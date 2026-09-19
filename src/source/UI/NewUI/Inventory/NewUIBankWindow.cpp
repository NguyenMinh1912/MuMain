//*****************************************************************************
// File: NewUIBankWindow.cpp
//*****************************************************************************

#include "stdafx.h"
#include "UI/NewUI/Inventory/NewUIBankWindow.h"
#include "I18N/All.h"

#include "Audio/DSPlaySound.h"
#include "Network/Server/BankStore.h"
#include "Network/Server/WSclient.h"
#include "UI/NewUI/Dialogs/NewUICustomMessageBox.h"
#include "UI/NewUI/NewUISystem.h"

#include <algorithm>
#include <limits>

using namespace SEASON3B;

namespace
{
/// <summary>
/// The names of the currencies, in the order of <see cref="Net::Bank::Currency"/>.
/// </summary>
/// <remarks>
/// Product names, not sentences: they are the same in every language of the client, the way the
/// item names of the jewels are, so they are kept here instead of in the translation tables.
/// </remarks>
const wchar_t* const CurrencyNames[] = {
    L"Zen",      L"WCoinC", L"WCoinP",   L"Goblin",  L"Bless", L"Soul",   L"Life",
    L"Creation", L"Chaos",  L"Guardian", L"Harmony", L"Lower", L"Higher",
};

/// <summary>The geometry of the window. Everything is relative to its top left corner.</summary>
constexpr int GridOffsetX = 15;
constexpr int GridOffsetY = 36;
constexpr int GridEndY = 426;
constexpr int SelectionLineY = 430;
constexpr int ButtonRowOneY = 452;
constexpr int ButtonRowTwoY = 480;
constexpr int ButtonRowThreeY = 508;
constexpr int ButtonWidth = 53;
constexpr int ButtonHeight = 23;
constexpr int ButtonSpacing = 58;
constexpr int FirstButtonX = 10;
constexpr int OfferLineHeight = 38;
constexpr int OfferListTop = 40;
constexpr int TextMargin = 12;

/// <summary>How many offers fit into the list of the market page.</summary>
constexpr int MaxVisibleOffers = (GridEndY - OfferListTop) / OfferLineHeight;

/// <summary>The value which means "no box is picked".</summary>
constexpr int NoSlot = -1;

/// <summary>
/// What a deposit asks for when the player wants to move everything: the server treats the amount
/// as an upper bound and moves what the character actually has.
/// </summary>
constexpr int64_t Everything = std::numeric_limits<int32_t>::max();
} // namespace

CNewUIBankWindow::CNewUIBankWindow()
    : m_pNewUIMng(nullptr), m_pInventoryCtrl(nullptr), m_page(Page::Storage), m_selectedCurrency(0), m_selectedOffer(0),
      m_selectedSlot(NoSlot), m_pendingInput(PendingInput::None), m_transferCarriesItem(false)
{
    m_Pos.x = m_Pos.y = 0;
}

CNewUIBankWindow::~CNewUIBankWindow()
{
    Release();
}

bool CNewUIBankWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (nullptr == pNewUIMng || nullptr == g_pNewUI3DRenderMng || nullptr == g_pNewItemMng)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(INTERFACE_BANK, this);

    m_pInventoryCtrl = new CNewUIInventoryCtrl;
    if (false == m_pInventoryCtrl->Create(STORAGE_TYPE::BANK, g_pNewUI3DRenderMng, g_pNewItemMng, this, x + GridOffsetX,
                                          y + GridOffsetY, BANK_COLUMNS, BANK_ROWS))
    {
        SAFE_DELETE(m_pInventoryCtrl);
        return false;
    }

    SetPos(x, y);
    LoadImages();

    InitButton(&m_abtn[BTN_PAGE], 0, 0, I18N::Game::BankMarket);
    InitButton(&m_abtn[BTN_PREV], 0, 0, I18N::Game::BankPreviousPage);
    InitButton(&m_abtn[BTN_NEXT], 0, 0, I18N::Game::BankNextPage);
    InitButton(&m_abtn[BTN_DEPOSIT], 0, 0, I18N::Game::BankDepositEverything);
    InitButton(&m_abtn[BTN_WITHDRAW], 0, 0, I18N::Game::BankWithdrawEverything);
    InitButton(&m_abtn[BTN_OFFER], 0, 0, I18N::Game::BankOffer);
    InitButton(&m_abtn[BTN_SEND_ITEM], 0, 0, I18N::Game::BankSendItem);
    InitButton(&m_abtn[BTN_SEND_VALUE], 0, 0, I18N::Game::BankSendValue);

    // The captions were set above; SetPos owns where they sit.
    SetPos(x, y);

    Show(false);

    return true;
}

void CNewUIBankWindow::Release()
{
    UnloadImages();

    SAFE_DELETE(m_pInventoryCtrl);

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void CNewUIBankWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;

    if (m_pInventoryCtrl)
    {
        m_pInventoryCtrl->SetPos(x + GridOffsetX, y + GridOffsetY);
    }

    m_abtn[BTN_PAGE].ChangeButtonInfo(x + FirstButtonX, y + ButtonRowOneY, ButtonWidth, ButtonHeight);
    m_abtn[BTN_PREV].ChangeButtonInfo(x + FirstButtonX + ButtonSpacing, y + ButtonRowOneY, ButtonWidth, ButtonHeight);
    m_abtn[BTN_NEXT].ChangeButtonInfo(x + FirstButtonX + 2 * ButtonSpacing, y + ButtonRowOneY, ButtonWidth,
                                      ButtonHeight);

    m_abtn[BTN_DEPOSIT].ChangeButtonInfo(x + FirstButtonX, y + ButtonRowTwoY, ButtonWidth, ButtonHeight);
    m_abtn[BTN_WITHDRAW].ChangeButtonInfo(x + FirstButtonX + ButtonSpacing, y + ButtonRowTwoY, ButtonWidth,
                                          ButtonHeight);

    m_abtn[BTN_OFFER].ChangeButtonInfo(x + FirstButtonX, y + ButtonRowThreeY, ButtonWidth, ButtonHeight);
    m_abtn[BTN_SEND_ITEM].ChangeButtonInfo(x + FirstButtonX + ButtonSpacing, y + ButtonRowThreeY, ButtonWidth,
                                           ButtonHeight);
    m_abtn[BTN_SEND_VALUE].ChangeButtonInfo(x + FirstButtonX + 2 * ButtonSpacing, y + ButtonRowThreeY, ButtonWidth,
                                            ButtonHeight);
}

void CNewUIBankWindow::InitButton(CNewUIButton* pButton, int x, int y, const wchar_t* caption)
{
    pButton->ChangeText(caption);
    pButton->ChangeTextBackColor(RGBA(255, 255, 255, 0));
    pButton->ChangeButtonImgState(true, IMAGE_BANK_BUTTON, true);
    pButton->ChangeButtonInfo(x, y, ButtonWidth, ButtonHeight);
    pButton->ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
    pButton->ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
}

bool CNewUIBankWindow::UpdateMouseEvent()
{
    // Before the item control gets the event: it may consume the click, and the box to sell or to
    // send is picked with exactly such a click.
    ProcessSelection();

    if (m_page == Page::Storage && m_pInventoryCtrl && false == m_pInventoryCtrl->UpdateMouseEvent())
    {
        return false;
    }

    if (ProcessButtons())
    {
        return false;
    }

    if (CheckMouseIn(m_Pos.x, m_Pos.y, BANK_WIDTH, BANK_HEIGHT))
    {
        if (g_pNewUISystem->HandleFrameCornerClose(m_Pos, INTERFACE_BANK))
        {
            return false;
        }

        if (IsPress(VK_RBUTTON))
        {
            MouseRButton = false;
            MouseRButtonPop = false;
            MouseRButtonPush = false;
            return false;
        }

        if (!IsNone(VK_LBUTTON))
        {
            return false;
        }
    }

    return true;
}

bool CNewUIBankWindow::UpdateKeyEvent()
{
    if (!g_pNewUISystem->IsVisible(INTERFACE_BANK))
    {
        return true;
    }

    if (IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(INTERFACE_BANK);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool CNewUIBankWindow::Update()
{
    ShowRefusedRequest();
    OpenPendingInput();

    if (m_page == Page::Storage && m_pInventoryCtrl && !m_pInventoryCtrl->Update())
    {
        return false;
    }

    return true;
}

void CNewUIBankWindow::ShowRefusedRequest()
{
    Net::Bank::Operation operation = Net::Bank::Operation::Deposit;
    Net::Bank::ResultCode result = Net::Bank::ResultCode::Success;
    if (!Net::Bank::Store::Instance().TakeNewResult(operation, result))
    {
        return;
    }

    if (result == Net::Bank::ResultCode::Success)
    {
        // What succeeded is visible: the balances and the boxes were sent with the answer.
        m_selectedSlot = NoSlot;
        return;
    }

    // Which of the twenty reasons it was is in the answer, but the player only needs to know that
    // nothing happened; the balances on screen say the rest.
    g_pSystemLogBox->AddText(I18N::Game::BankRefusedTheRequest, SEASON3B::TYPE_ERROR_MESSAGE);
}

void CNewUIBankWindow::OpenPendingInput()
{
    const PendingInput pending = m_pendingInput;
    m_pendingInput = PendingInput::None;

    switch (pending)
    {
    case PendingInput::Price:
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CBankPriceMsgBoxLayout));
        break;
    case PendingInput::Receiver:
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CBankReceiverMsgBoxLayout));
        break;
    case PendingInput::Amount:
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CBankAmountMsgBoxLayout));
        break;
    default:
        break;
    }
}

bool CNewUIBankWindow::Render()
{
    EnableAlphaTest();

    RenderFrame();

    if (m_page == Page::Storage)
    {
        RenderStoragePage();
    }
    else
    {
        RenderMarketPage();
    }

    const int lastButton = m_page == Page::Storage ? MAX_BTN : BTN_OFFER;
    for (int i = 0; i < lastButton; ++i)
    {
        m_abtn[i].Render();
    }

    DisableAlphaBlend();

    return true;
}

void CNewUIBankWindow::RenderFrame()
{
    const auto x = static_cast<float>(m_Pos.x);
    const auto y = static_cast<float>(m_Pos.y);
    constexpr float topHeight = 64.f;
    constexpr float bottomHeight = 45.f;
    constexpr float sideWidth = 21.f;

    RenderImage(IMAGE_BANK_BACK, x, y, BANK_WIDTH, BANK_HEIGHT);
    RenderImage(IMAGE_BANK_TOP, x, y, BANK_WIDTH, topHeight);
    RenderImage(IMAGE_BANK_LEFT, x, y + topHeight, sideWidth, BANK_HEIGHT - topHeight - bottomHeight);
    RenderImage(IMAGE_BANK_RIGHT, x + BANK_WIDTH - sideWidth, y + topHeight, sideWidth,
                BANK_HEIGHT - topHeight - bottomHeight);
    RenderImage(IMAGE_BANK_BOTTOM, x, y + BANK_HEIGHT - bottomHeight, BANK_WIDTH, bottomHeight);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(216, 216, 216, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + 11, I18N::Game::Bank, BANK_WIDTH, 0, RT3_SORT_CENTER);
}

void CNewUIBankWindow::RenderStoragePage()
{
    if (m_pInventoryCtrl)
    {
        m_pInventoryCtrl->Render();
    }

    const auto currency = static_cast<Net::Bank::Currency>(m_selectedCurrency);
    const int64_t balance = Net::Bank::Store::Instance().GetBalance(currency);

    wchar_t line[128];
    if (m_selectedSlot == NoSlot)
    {
        mu_swprintf_s(line, _countof(line), L"%ls: %lld", GetCurrencyName(currency), static_cast<long long>(balance));
    }
    else
    {
        mu_swprintf_s(line, _countof(line), L"%ls: %lld   [%d]", GetCurrencyName(currency),
                      static_cast<long long>(balance), m_selectedSlot);
    }

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(255, 220, 150, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + SelectionLineY, line, BANK_WIDTH, 0, RT3_SORT_CENTER);
}

void CNewUIBankWindow::RenderMarketPage()
{
    const auto& store = Net::Bank::Store::Instance();
    const auto& offers = store.GetOffers();

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);

    if (offers.empty())
    {
        g_pRenderText->SetTextColor(200, 200, 200, 255);
        g_pRenderText->RenderText(m_Pos.x, m_Pos.y + OfferListTop, I18N::Game::BankHasNoOffers, BANK_WIDTH, 0,
                                  RT3_SORT_CENTER);
        return;
    }

    const int visible = std::min<int>(MaxVisibleOffers, static_cast<int>(offers.size()));
    for (int i = 0; i < visible; ++i)
    {
        const auto& offer = offers[i];
        const int lineY = m_Pos.y + OfferListTop + i * OfferLineHeight;

        if (i == m_selectedOffer)
        {
            g_pRenderText->SetTextColor(255, 220, 150, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(200, 200, 200, 255);
        }

        g_pRenderText->RenderText(m_Pos.x + TextMargin, lineY, offer.OfferName.c_str());

        wchar_t price[128];
        mu_swprintf_s(price, _countof(price), L"%lld %ls - %ls", static_cast<long long>(offer.PriceAmount),
                      GetCurrencyName(offer.PriceCurrency), offer.SellerName.c_str());
        g_pRenderText->SetTextColor(180, 180, 180, 255);
        g_pRenderText->RenderText(m_Pos.x + TextMargin, lineY + 16, price);
    }

    wchar_t pageText[64];
    mu_swprintf_s(pageText, _countof(pageText), I18N::Game::BankPageOf, store.GetOfferPage() + 1,
                  store.GetOfferPageCount());
    g_pRenderText->SetTextColor(200, 200, 200, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + SelectionLineY, pageText, BANK_WIDTH, 0, RT3_SORT_CENTER);
}

bool CNewUIBankWindow::ProcessButtons()
{
    if (m_abtn[BTN_PAGE].UpdateMouseEvent())
    {
        if (m_page == Page::Storage)
        {
            m_page = Page::Market;
            m_selectedOffer = 0;
            m_abtn[BTN_PAGE].ChangeText(I18N::Game::BankStorage);
            m_abtn[BTN_DEPOSIT].ChangeText(I18N::Game::BankBuyOffer);
            m_abtn[BTN_WITHDRAW].ChangeText(I18N::Game::BankCancelOffer);
            RequestMarketPage(0);
        }
        else
        {
            m_page = Page::Storage;
            m_abtn[BTN_PAGE].ChangeText(I18N::Game::BankMarket);
            m_abtn[BTN_DEPOSIT].ChangeText(I18N::Game::BankDepositEverything);
            m_abtn[BTN_WITHDRAW].ChangeText(I18N::Game::BankWithdrawEverything);
        }

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return m_page == Page::Storage ? ProcessStorageButtons() : ProcessMarketButtons();
}

bool CNewUIBankWindow::ProcessStorageButtons()
{
    const int currencyCount = static_cast<int>(Net::Bank::Currency::Count);

    if (m_abtn[BTN_PREV].UpdateMouseEvent())
    {
        m_selectedCurrency = (m_selectedCurrency + currencyCount - 1) % currencyCount;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_NEXT].UpdateMouseEvent())
    {
        m_selectedCurrency = (m_selectedCurrency + 1) % currencyCount;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_DEPOSIT].UpdateMouseEvent())
    {
        MoveSelectedCurrency(true);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_WITHDRAW].UpdateMouseEvent())
    {
        MoveSelectedCurrency(false);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_OFFER].UpdateMouseEvent())
    {
        if (RequireSelectedItem())
        {
            m_pendingInput = PendingInput::Price;
        }

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_SEND_ITEM].UpdateMouseEvent())
    {
        if (RequireSelectedItem())
        {
            m_transferCarriesItem = true;
            m_pendingInput = PendingInput::Receiver;
        }

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_SEND_VALUE].UpdateMouseEvent())
    {
        m_transferCarriesItem = false;
        m_pendingInput = PendingInput::Receiver;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool CNewUIBankWindow::ProcessMarketButtons()
{
    const auto& store = Net::Bank::Store::Instance();

    if (m_abtn[BTN_PREV].UpdateMouseEvent())
    {
        if (store.GetOfferPage() > 0)
        {
            RequestMarketPage(static_cast<BYTE>(store.GetOfferPage() - 1));
        }

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_NEXT].UpdateMouseEvent())
    {
        if (store.GetOfferPage() + 1 < store.GetOfferPageCount())
        {
            RequestMarketPage(static_cast<BYTE>(store.GetOfferPage() + 1));
        }

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_DEPOSIT].UpdateMouseEvent())
    {
        BuySelectedOffer();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_WITHDRAW].UpdateMouseEvent())
    {
        CancelSelectedOffer();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

void CNewUIBankWindow::ProcessSelection()
{
    if (!CheckMouseIn(m_Pos.x, m_Pos.y, BANK_WIDTH, BANK_HEIGHT))
    {
        return;
    }

    if (m_page == Page::Market)
    {
        if (!IsPress(VK_LBUTTON))
        {
            return;
        }

        const int relativeY = MouseY - (m_Pos.y + OfferListTop);
        if (relativeY < 0)
        {
            return;
        }

        const int index = relativeY / OfferLineHeight;
        const auto& offers = Net::Bank::Store::Instance().GetOffers();
        if (index < static_cast<int>(offers.size()) && index < MaxVisibleOffers)
        {
            m_selectedOffer = index;
        }

        return;
    }

    // The left button drags items, so the box to sell or to send is picked with the right one.
    if (!IsPress(VK_RBUTTON) || m_pInventoryCtrl == nullptr)
    {
        return;
    }

    const int pointed = m_pInventoryCtrl->GetPointedSquareIndex();
    if (pointed >= 0 && m_pInventoryCtrl->FindItem(pointed) != nullptr)
    {
        m_selectedSlot = pointed;
        PlayBuffer(SOUND_CLICK01);
    }
}

bool CNewUIBankWindow::RequireSelectedItem()
{
    if (m_selectedSlot != NoSlot && m_pInventoryCtrl && m_pInventoryCtrl->FindItem(m_selectedSlot) != nullptr)
    {
        return true;
    }

    m_selectedSlot = NoSlot;
    g_pSystemLogBox->AddText(I18N::Game::BankSelectAnItemFirst, SEASON3B::TYPE_SYSTEM_MESSAGE);
    return false;
}

void CNewUIBankWindow::MoveSelectedCurrency(bool deposit)
{
    if (!SocketClient)
    {
        return;
    }

    const auto currency = static_cast<Net::Bank::Currency>(m_selectedCurrency);
    const int64_t amount = deposit ? Everything : Net::Bank::Store::Instance().GetBalance(currency);
    if (amount <= 0)
    {
        return;
    }

    SocketClient->ToGameServer()->SendBankMoveValue(deposit, currency, amount);
}

void CNewUIBankWindow::BuySelectedOffer()
{
    if (!SocketClient)
    {
        return;
    }

    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    if (m_selectedOffer < static_cast<int>(offers.size()))
    {
        SocketClient->ToGameServer()->SendMarketBuy(offers[m_selectedOffer].ListingId.data());
    }
}

void CNewUIBankWindow::CancelSelectedOffer()
{
    if (!SocketClient)
    {
        return;
    }

    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    if (m_selectedOffer < static_cast<int>(offers.size()))
    {
        SocketClient->ToGameServer()->SendMarketCancel(offers[m_selectedOffer].ListingId.data());
    }
}

void CNewUIBankWindow::FinishOffer(int64_t price)
{
    if (!SocketClient || !RequireSelectedItem())
    {
        return;
    }

    SocketClient->ToGameServer()->SendMarketRegisterItem(static_cast<BYTE>(m_selectedSlot),
                                                         static_cast<Net::Bank::Currency>(m_selectedCurrency), price);
}

void CNewUIBankWindow::SetTransferReceiver(const wchar_t* receiverName)
{
    if (receiverName == nullptr)
    {
        return;
    }

    m_transferReceiver = receiverName;

    if (!m_transferCarriesItem)
    {
        // A currency still needs an amount; the dialog for it opens on the next frame.
        m_pendingInput = PendingInput::Amount;
        return;
    }

    if (SocketClient && RequireSelectedItem())
    {
        SocketClient->ToGameServer()->SendBankTransferItem(m_transferReceiver.c_str(),
                                                           static_cast<BYTE>(m_selectedSlot), L"");
    }
}

void CNewUIBankWindow::FinishValueTransfer(int64_t amount)
{
    if (!SocketClient || m_transferReceiver.empty())
    {
        return;
    }

    SocketClient->ToGameServer()->SendBankTransferValue(
        m_transferReceiver.c_str(), static_cast<Net::Bank::Currency>(m_selectedCurrency), amount, L"");
    m_transferReceiver.clear();
}

void CNewUIBankWindow::CancelPendingInput()
{
    m_pendingInput = PendingInput::None;
    m_transferReceiver.clear();
}

void CNewUIBankWindow::RequestMarketPage(BYTE page)
{
    if (SocketClient)
    {
        SocketClient->ToGameServer()->SendMarketList(page, Net::Bank::AnyCurrency, false, L"");
    }
}

void CNewUIBankWindow::ClosingProcess()
{
    if (SocketClient)
    {
        SocketClient->ToGameServer()->SendCloseNpcRequest();
    }

    m_page = Page::Storage;
    m_selectedOffer = 0;
    m_selectedSlot = NoSlot;
    CancelPendingInput();
    m_abtn[BTN_PAGE].ChangeText(I18N::Game::BankMarket);
    m_abtn[BTN_DEPOSIT].ChangeText(I18N::Game::BankDepositEverything);
    m_abtn[BTN_WITHDRAW].ChangeText(I18N::Game::BankWithdrawEverything);
}

bool CNewUIBankWindow::InsertItem(int iIndex, std::span<const BYTE> pbyItemPacket)
{
    return m_pInventoryCtrl != nullptr && m_pInventoryCtrl->AddItem(iIndex, pbyItemPacket);
}

void CNewUIBankWindow::ProcessToReceiveBankItems(int nIndex, std::span<const BYTE> pbyItemPacket)
{
    if (m_pInventoryCtrl == nullptr)
    {
        return;
    }

    CNewUIInventoryCtrl::DeletePickedItem();

    const int boxes = m_pInventoryCtrl->GetNumberOfColumn() * m_pInventoryCtrl->GetNumberOfRow();
    if (nIndex >= 0 && nIndex < boxes)
    {
        m_pInventoryCtrl->RemoveItemAt(nIndex);
        m_pInventoryCtrl->AddItem(nIndex, pbyItemPacket);
    }
}

void CNewUIBankWindow::DeleteAllItems()
{
    m_selectedSlot = NoSlot;

    if (m_pInventoryCtrl)
    {
        m_pInventoryCtrl->RemoveAllItems();
    }
}

float CNewUIBankWindow::GetLayerDepth()
{
    return 2.2f;
}

const wchar_t* CNewUIBankWindow::GetCurrencyName(Net::Bank::Currency currency)
{
    const auto index = static_cast<size_t>(currency);
    return index < _countof(CurrencyNames) ? CurrencyNames[index] : L"?";
}

void CNewUIBankWindow::LoadImages()
{
    LoadBitmap(L"Interface\\newui_msgbox_back.jpg", IMAGE_BANK_BACK, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back01.tga", IMAGE_BANK_TOP, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-L.tga", IMAGE_BANK_LEFT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back02-R.tga", IMAGE_BANK_RIGHT, GL_LINEAR);
    LoadBitmap(L"Interface\\newui_item_back03.tga", IMAGE_BANK_BOTTOM, GL_LINEAR);
}

void CNewUIBankWindow::UnloadImages()
{
    DeleteBitmap(IMAGE_BANK_BOTTOM);
    DeleteBitmap(IMAGE_BANK_RIGHT);
    DeleteBitmap(IMAGE_BANK_LEFT);
    DeleteBitmap(IMAGE_BANK_TOP);
    DeleteBitmap(IMAGE_BANK_BACK);
}
