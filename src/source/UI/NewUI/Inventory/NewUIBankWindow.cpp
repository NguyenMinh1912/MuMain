//*****************************************************************************
// File: NewUIBankWindow.cpp
//*****************************************************************************

#include "stdafx.h"
#include "UI/NewUI/Inventory/NewUIBankWindow.h"
#include "I18N/All.h"

#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzInventory.h"
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
    L"Zen",      L"WCoinC", L"WCoinP",   L"Goblin",  L"Bless", L"Soul",  L"Life",
    L"Creation", L"Chaos",  L"Guardian", L"Harmony", L"Lower", L"Higher",
};

/// <summary>The geometry of the window. Everything is relative to its top left corner.</summary>
constexpr int GridOffsetX = 15;
constexpr int GridOffsetY = 34;
constexpr int ItemPageLabelY = 170;

constexpr int BalanceTop = 188;
constexpr int BalanceLineHeight = 14;
constexpr int BalanceRows = 7;
constexpr int BalanceColumnWidth = 83;
constexpr int BalanceMarginX = 12;

constexpr int ButtonRowOneY = 300;
constexpr int ButtonRowTwoY = 328;
constexpr int ButtonRowThreeY = 356;
constexpr int ButtonWidth = 53;
constexpr int ButtonHeight = 23;
constexpr int ButtonSpacing = 58;
constexpr int FirstButtonX = 10;

constexpr int OfferListTop = 40;
constexpr int OfferLineHeight = 34;
constexpr int OfferListBottom = 380;
constexpr int MarketPageLabelY = 390;
constexpr int TextMargin = 12;

/// <summary>How many offers fit into the list of the market page.</summary>
constexpr int MaxVisibleOffers = (OfferListBottom - OfferListTop) / OfferLineHeight;

/// <summary>The value which means "no box is picked".</summary>
constexpr int NoSlot = -1;

/// <summary>
/// What a deposit asks for when the player wants to move everything: the server treats the amount
/// as an upper bound and moves what the character actually has.
/// </summary>
constexpr int64_t Everything = std::numeric_limits<int32_t>::max();

/// <summary>How many currencies there are, which is how many lines the balances have.</summary>
constexpr int CurrencyCount = static_cast<int>(Net::Bank::Currency::Count);
} // namespace

CNewUIBankWindow::CNewUIBankWindow()
    : m_pNewUIMng(nullptr), m_pInventoryCtrl(nullptr), m_page(Page::Storage), m_itemPage(0), m_selectedCurrency(0),
      m_selectedOffer(0), m_pendingSlot(NoSlot), m_autoMoveItem(nullptr), m_pendingInput(PendingInput::None),
      m_transferCarriesItem(false)
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
    if (false == m_pInventoryCtrl->Create(STORAGE_TYPE::BANK, g_pNewUI3DRenderMng, g_pNewItemMng, this,
                                          x + GridOffsetX, y + GridOffsetY, BANK_COLUMNS, BANK_PAGE_ROWS))
    {
        SAFE_DELETE(m_pInventoryCtrl);
        return false;
    }

    LoadImages();

    InitButton(&m_abtn[BTN_PAGE], I18N::Game::BankMarket);
    InitButton(&m_abtn[BTN_PREV], I18N::Game::BankPreviousPage);
    InitButton(&m_abtn[BTN_NEXT], I18N::Game::BankNextPage);
    InitButton(&m_abtn[BTN_DEPOSIT], I18N::Game::BankDepositEverything);
    InitButton(&m_abtn[BTN_WITHDRAW], I18N::Game::BankWithdrawEverything);
    InitButton(&m_abtn[BTN_SEND_VALUE], I18N::Game::BankSendValue);
    InitButton(&m_abtn[BTN_OFFER], I18N::Game::BankOffer);
    InitButton(&m_abtn[BTN_SEND_ITEM], I18N::Game::BankSendItem);

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
    m_abtn[BTN_SEND_VALUE].ChangeButtonInfo(x + FirstButtonX + 2 * ButtonSpacing, y + ButtonRowTwoY, ButtonWidth,
                                            ButtonHeight);

    m_abtn[BTN_OFFER].ChangeButtonInfo(x + FirstButtonX, y + ButtonRowThreeY, ButtonWidth, ButtonHeight);
    m_abtn[BTN_SEND_ITEM].ChangeButtonInfo(x + FirstButtonX + ButtonSpacing, y + ButtonRowThreeY, ButtonWidth,
                                           ButtonHeight);
}

void CNewUIBankWindow::InitButton(CNewUIButton* pButton, const wchar_t* caption)
{
    pButton->ChangeText(caption);
    pButton->ChangeTextBackColor(RGBA(255, 255, 255, 0));
    pButton->ChangeButtonImgState(true, IMAGE_BANK_BUTTON, true);
    pButton->ChangeButtonInfo(0, 0, ButtonWidth, ButtonHeight);
    pButton->ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
    pButton->ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
}

void CNewUIBankWindow::OpeningProcess()
{
    m_page = Page::Storage;
    m_itemPage = 0;
    m_selectedOffer = 0;
    CancelPendingInput();

    m_abtn[BTN_PAGE].ChangeText(I18N::Game::BankMarket);
    m_abtn[BTN_DEPOSIT].ChangeText(I18N::Game::BankDepositEverything);
    m_abtn[BTN_WITHDRAW].ChangeText(I18N::Game::BankWithdrawEverything);

    if (SocketClient)
    {
        SocketClient->ToGameServer()->SendBankDialog(true);
    }
}

void CNewUIBankWindow::ClosingProcess()
{
    if (SocketClient)
    {
        SocketClient->ToGameServer()->SendBankDialog(false);
    }

    CancelPendingInput();
    m_autoMoveItem = nullptr;
}

bool CNewUIBankWindow::UpdateMouseEvent()
{
    if (m_page == Page::Storage && m_pInventoryCtrl && false == m_pInventoryCtrl->UpdateMouseEvent())
    {
        return false;
    }

    if (m_page == Page::Storage)
    {
        ProcessInventoryCtrl();
    }
    else
    {
        ProcessOfferSelection();
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

        if (m_page == Page::Storage)
        {
            ProcessBalanceSelection();
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
        m_pendingSlot = NoSlot;
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

    const int lastButton = m_page == Page::Storage ? MAX_BTN : BTN_SEND_VALUE;
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

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);

    wchar_t pageText[64];
    mu_swprintf_s(pageText, _countof(pageText), I18N::Game::BankPageOf, m_itemPage + 1, ITEM_PAGE_COUNT);
    g_pRenderText->SetTextColor(200, 200, 200, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + ItemPageLabelY, pageText, BANK_WIDTH, 0, RT3_SORT_CENTER);

    RenderBalances();
}

void CNewUIBankWindow::RenderBalances()
{
    const auto& store = Net::Bank::Store::Instance();

    for (int i = 0; i < CurrencyCount; ++i)
    {
        const int column = i / BalanceRows;
        const int row = i % BalanceRows;
        const int lineX = m_Pos.x + BalanceMarginX + column * BalanceColumnWidth;
        const int lineY = m_Pos.y + BalanceTop + row * BalanceLineHeight;

        if (i == m_selectedCurrency)
        {
            g_pRenderText->SetTextColor(255, 220, 150, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(190, 190, 190, 255);
        }

        const auto currency = static_cast<Net::Bank::Currency>(i);
        wchar_t line[64];
        mu_swprintf_s(line, _countof(line), L"%ls %lld", GetCurrencyName(currency),
                      static_cast<long long>(store.GetBalance(currency)));
        g_pRenderText->RenderText(lineX, lineY, line);
    }
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
        g_pRenderText->RenderText(m_Pos.x + TextMargin, lineY + 15, price);
    }

    wchar_t pageText[64];
    mu_swprintf_s(pageText, _countof(pageText), I18N::Game::BankPageOf, store.GetOfferPage() + 1,
                  store.GetOfferPageCount());
    g_pRenderText->SetTextColor(200, 200, 200, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + MarketPageLabelY, pageText, BANK_WIDTH, 0, RT3_SORT_CENTER);
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
    if (m_abtn[BTN_PREV].UpdateMouseEvent())
    {
        ShowItemPage(m_itemPage - 1);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_NEXT].UpdateMouseEvent())
    {
        ShowItemPage(m_itemPage + 1);
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

    if (m_abtn[BTN_SEND_VALUE].UpdateMouseEvent())
    {
        m_transferCarriesItem = false;
        m_pendingInput = PendingInput::Receiver;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_OFFER].UpdateMouseEvent())
    {
        if (TakePickedBankSlot(m_pendingSlot))
        {
            m_pendingInput = PendingInput::Price;
        }

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_SEND_ITEM].UpdateMouseEvent())
    {
        if (TakePickedBankSlot(m_pendingSlot))
        {
            m_transferCarriesItem = true;
            m_pendingInput = PendingInput::Receiver;
        }

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

void CNewUIBankWindow::ProcessInventoryCtrl()
{
    if (m_pInventoryCtrl == nullptr)
    {
        return;
    }

    CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
    if (pPickedItem == nullptr)
    {
        // Nothing on the cursor, so the right button means "move this one over by itself".
        if (IsPress(VK_RBUTTON))
        {
            ProcessAutoMove();
        }

        return;
    }

    ITEM* pItemObj = pPickedItem->GetItem();
    if (pItemObj == nullptr)
    {
        return;
    }

    if (IsPress(VK_LBUTTON) || IsRelease(VK_LBUTTON))
    {
        const int targetSlot = pPickedItem->GetTargetLinealPos(m_pInventoryCtrl);
        if (targetSlot >= 0 && m_pInventoryCtrl->CanMove(targetSlot, pItemObj))
        {
            SendRequestEquipmentItem(pPickedItem->GetSourceStorageType(), pPickedItem->GetSourceLinealPos(), pItemObj,
                                     m_pInventoryCtrl->GetStorageType(), targetSlot);
        }
    }
    else
    {
        m_pInventoryCtrl->SetSquareColorNormal(0.1f, 0.4f, 0.8f);
    }
}

void CNewUIBankWindow::ProcessAutoMove()
{
    if (m_pInventoryCtrl == nullptr || m_autoMoveItem != nullptr)
    {
        return;
    }

    ITEM* pItemObj = m_pInventoryCtrl->FindItemAtPt(MouseX, MouseY);
    if (pItemObj == nullptr)
    {
        return;
    }

    const int targetSlot = g_pMyInventory->FindEmptySlotIncludingExtensions(pItemObj);
    if (targetSlot == -1)
    {
        return;
    }

    const int sourceSlot =
        pItemObj->y * m_pInventoryCtrl->GetNumberOfColumn() + pItemObj->x + m_pInventoryCtrl->GetIndexOffset();

    // Remembered, not removed: the box only empties once the server confirmed the move.
    m_autoMoveItem = pItemObj;
    SendRequestEquipmentItem(STORAGE_TYPE::BANK, sourceSlot, pItemObj, STORAGE_TYPE::INVENTORY, targetSlot);
    PlayBuffer(SOUND_GET_ITEM01);
}

void CNewUIBankWindow::ProcessAutoMoveSuccess()
{
    if (m_autoMoveItem == nullptr || m_pInventoryCtrl == nullptr)
    {
        return;
    }

    m_pInventoryCtrl->RemoveItem(m_autoMoveItem);
    m_autoMoveItem = nullptr;
}

void CNewUIBankWindow::ProcessOfferSelection()
{
    if (!CheckMouseIn(m_Pos.x, m_Pos.y, BANK_WIDTH, BANK_HEIGHT) || !IsPress(VK_LBUTTON))
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
}

void CNewUIBankWindow::ProcessBalanceSelection()
{
    if (!IsPress(VK_LBUTTON))
    {
        return;
    }

    const int relativeX = MouseX - (m_Pos.x + BalanceMarginX);
    const int relativeY = MouseY - (m_Pos.y + BalanceTop);
    if (relativeX < 0 || relativeY < 0 || relativeY >= BalanceRows * BalanceLineHeight)
    {
        return;
    }

    const int index = (relativeX / BalanceColumnWidth) * BalanceRows + relativeY / BalanceLineHeight;
    if (index >= 0 && index < CurrencyCount)
    {
        m_selectedCurrency = index;
        PlayBuffer(SOUND_CLICK01);
    }
}

void CNewUIBankWindow::ShowItemPage(int page)
{
    if (m_pInventoryCtrl == nullptr || page < 0 || page >= ITEM_PAGE_COUNT)
    {
        return;
    }

    m_itemPage = page;
    m_pInventoryCtrl->RemoveAllItems();
    m_pInventoryCtrl->SetIndexOffset(page * ITEMS_PER_PAGE);

    const int first = page * ITEMS_PER_PAGE;
    const int last = first + ITEMS_PER_PAGE;
    for (const auto& stored : m_storedItems)
    {
        if (stored.first >= first && stored.first < last)
        {
            m_pInventoryCtrl->AddItem(stored.first, stored.second);
        }
    }
}

bool CNewUIBankWindow::TakePickedBankSlot(int& slot)
{
    slot = NoSlot;

    CNewUIPickedItem* pPickedItem = CNewUIInventoryCtrl::GetPickedItem();
    if (pPickedItem != nullptr && pPickedItem->GetItem() != nullptr
        && pPickedItem->GetSourceStorageType() == STORAGE_TYPE::BANK)
    {
        slot = pPickedItem->GetSourceLinealPos();

        // The item goes back into its box while the dialog asks; the server is told the box, and
        // an item left hanging on the cursor would only be in the way.
        CNewUIInventoryCtrl::BackupPickedItem();
        return true;
    }

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
    if (!SocketClient || m_pendingSlot == NoSlot)
    {
        return;
    }

    SocketClient->ToGameServer()->SendMarketRegisterItem(static_cast<BYTE>(m_pendingSlot),
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

    if (SocketClient && m_pendingSlot != NoSlot)
    {
        SocketClient->ToGameServer()->SendBankTransferItem(m_transferReceiver.c_str(),
                                                          static_cast<BYTE>(m_pendingSlot), L"");
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
    m_pendingSlot = NoSlot;
    m_transferReceiver.clear();
}

void CNewUIBankWindow::RequestMarketPage(BYTE page)
{
    if (SocketClient)
    {
        SocketClient->ToGameServer()->SendMarketList(page, Net::Bank::AnyCurrency, false, L"");
    }
}

bool CNewUIBankWindow::InsertItem(int iIndex, std::span<const BYTE> pbyItemPacket)
{
    if (iIndex < 0 || iIndex >= BANK_TOTAL_SLOTS)
    {
        return false;
    }

    m_storedItems[iIndex] = std::vector<BYTE>(pbyItemPacket.begin(), pbyItemPacket.end());

    const int first = m_itemPage * ITEMS_PER_PAGE;
    if (iIndex < first || iIndex >= first + ITEMS_PER_PAGE)
    {
        // The box belongs to a page which is not on screen; it is drawn when that page is shown.
        return true;
    }

    return m_pInventoryCtrl != nullptr && m_pInventoryCtrl->AddItem(iIndex, pbyItemPacket);
}

void CNewUIBankWindow::ProcessToReceiveBankItems(int nIndex, std::span<const BYTE> pbyItemPacket)
{
    if (m_pInventoryCtrl == nullptr || nIndex < 0 || nIndex >= BANK_TOTAL_SLOTS)
    {
        return;
    }

    CNewUIInventoryCtrl::DeletePickedItem();

    m_storedItems[nIndex] = std::vector<BYTE>(pbyItemPacket.begin(), pbyItemPacket.end());

    const int first = m_itemPage * ITEMS_PER_PAGE;
    if (nIndex >= first && nIndex < first + ITEMS_PER_PAGE)
    {
        m_pInventoryCtrl->RemoveItemAt(nIndex);
        m_pInventoryCtrl->AddItem(nIndex, pbyItemPacket);
    }
}

void CNewUIBankWindow::DeleteAllItems()
{
    m_pendingSlot = NoSlot;
    m_autoMoveItem = nullptr;
    m_storedItems.clear();

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
