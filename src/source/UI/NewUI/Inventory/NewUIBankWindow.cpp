//*****************************************************************************
// File: NewUIBankWindow.cpp
//*****************************************************************************

#include "stdafx.h"

#include "UI/NewUI/Inventory/NewUIBankWindow.h"

#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzInventory.h"
#include "I18N/All.h"
#include "Network/Server/BankStore.h"
#include "Network/Server/ServerListManager.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Chat/Chat.h"
#include "UI/NewUI/Dialogs/NewUICustomMessageBox.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"

namespace
{
/// <summary>The tint of the box or the row the player picked.</summary>
constexpr unsigned int PICKED_COLOR = 0x80FFC83Cu;

/// <summary>The tint of the box or the row the cursor is over.</summary>
constexpr unsigned int HOVERED_COLOR = 0x33FFFFFFu;

/// <summary>The tint of an empty box, so that the tiles are visible before anything is in them.</summary>
constexpr unsigned int EMPTY_BOX_COLOR = 0x40202830u;

/// <summary>How many of the currencies are money; the rest are jewels.</summary>
constexpr int MONEY_CURRENCY_COUNT = 4;

/// <summary>Writes an amount with a separator every three digits, which is how balances are read.</summary>
void FormatAmount(int64_t amount, wchar_t* text, size_t textLength)
{
    wchar_t digits[32] = {0};
    mu_swprintf(digits, L"%lld", static_cast<long long>(amount < 0 ? -amount : amount));

    const size_t digitCount = wcslen(digits);
    size_t written = 0;
    if (amount < 0 && written + 1 < textLength)
    {
        text[written++] = L'-';
    }

    for (size_t i = 0; i < digitCount && written + 1 < textLength; ++i)
    {
        if (i > 0 && (digitCount - i) % 3 == 0)
        {
            text[written++] = L'.';
        }

        if (written + 1 < textLength)
        {
            text[written++] = digits[i];
        }
    }

    text[written] = L'\0';
}
} // namespace

SEASON3B::CNewUIBankWindow::CNewUIBankWindow()
    : m_pNewUIMng(nullptr), m_pNewUI3DRenderMng(nullptr), m_page(Page::Items), m_itemPage(0), m_selectedCurrency(0),
      m_selectedOffer(-1), m_selectedSlot(-1), m_depositSourceSlot(-1), m_takeSourceSlot(-1), m_ownOffersOnly(false),
      m_pendingInput(PendingInput::None), m_offerCarriesItem(true), m_offerAmount(0)
{
    m_Pos.x = 0;
    m_Pos.y = 0;
    m_boxes.fill(nullptr);
}

SEASON3B::CNewUIBankWindow::~CNewUIBankWindow()
{
    Release();
}

bool SEASON3B::CNewUIBankWindow::Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y)
{
    if (pNewUIMng == nullptr || pNewUI3DRenderMng == nullptr || g_pNewItemMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(INTERFACE_BANK, this);

    m_pNewUI3DRenderMng = pNewUI3DRenderMng;
    m_pNewUI3DRenderMng->Add3DRenderObj(this, INVENTORY_CAMERA_Z_ORDER);

    SetPos(x, y);

    InitButton(&m_abtn[BTN_TAB_ITEMS], &I18N::Game::BankStorage);
    InitButton(&m_abtn[BTN_TAB_VALUES], &I18N::Game::BankValues);
    InitButton(&m_abtn[BTN_TAB_MARKET], &I18N::Game::BankMarket);
    InitButton(&m_abtn[BTN_PREV], &I18N::Game::BankPreviousPage);
    InitButton(&m_abtn[BTN_NEXT], &I18N::Game::BankNextPage);
    InitButton(&m_abtn[BTN_TAKE_ITEM], &I18N::Game::BankTakeItem);
    InitButton(&m_abtn[BTN_OFFER_ITEM], &I18N::Game::BankOffer);
    InitButton(&m_abtn[BTN_DEPOSIT], &I18N::Game::BankDepositEverything);
    InitButton(&m_abtn[BTN_WITHDRAW], &I18N::Game::BankWithdrawEverything);
    InitButton(&m_abtn[BTN_OFFER_VALUE], &I18N::Game::BankOffer);
    InitButton(&m_abtn[BTN_BUY], &I18N::Game::BankBuyOffer);
    InitButton(&m_abtn[BTN_CANCEL_OFFER], &I18N::Game::BankCancelOffer);
    InitButton(&m_abtn[BTN_MINE], &I18N::Game::BankMyOffersOnly);

    LayoutButtons();

    Show(false);

    return true;
}

void SEASON3B::CNewUIBankWindow::Release()
{
    DeleteAllItems();

    if (m_pNewUI3DRenderMng)
    {
        m_pNewUI3DRenderMng->Remove3DRenderObj(this);
        m_pNewUI3DRenderMng = nullptr;
    }

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CNewUIBankWindow::InitButton(CNewUIButton* pButton, const wchar_t* const* captionSlot)
{
    pButton->ChangeText(captionSlot);
    pButton->ChangeTextBackColor(RGBA(255, 255, 255, 0));
    pButton->ChangeButtonImgState(true, IMAGE_BANK_BUTTON, true);
    pButton->ChangeImgColor(BUTTON_STATE_UP, RGBA(255, 255, 255, 255));
    pButton->ChangeImgColor(BUTTON_STATE_DOWN, RGBA(255, 255, 255, 255));
}

void SEASON3B::CNewUIBankWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    LayoutButtons();
}

void SEASON3B::CNewUIBankWindow::LayoutButtons()
{
    m_abtn[BTN_TAB_ITEMS].ChangeButtonInfo(m_Pos.x + 8, m_Pos.y + TAB_ROW_TOP, TAB_WIDTH, TAB_HEIGHT);
    m_abtn[BTN_TAB_VALUES].ChangeButtonInfo(m_Pos.x + 132, m_Pos.y + TAB_ROW_TOP, TAB_WIDTH, TAB_HEIGHT);
    m_abtn[BTN_TAB_MARKET].ChangeButtonInfo(m_Pos.x + 256, m_Pos.y + TAB_ROW_TOP, TAB_WIDTH, TAB_HEIGHT);

    m_abtn[BTN_PREV].ChangeButtonInfo(m_Pos.x + 20, m_Pos.y + PAGE_ROW_TOP, 44, 20);
    m_abtn[BTN_NEXT].ChangeButtonInfo(m_Pos.x + 316, m_Pos.y + PAGE_ROW_TOP, 44, 20);

    // Two buttons stand in the middle, three fill the row.
    m_abtn[BTN_TAKE_ITEM].ChangeButtonInfo(m_Pos.x + 86, m_Pos.y + BUTTON_ROW_TOP, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_abtn[BTN_OFFER_ITEM].ChangeButtonInfo(m_Pos.x + 198, m_Pos.y + BUTTON_ROW_TOP, BUTTON_WIDTH, BUTTON_HEIGHT);

    m_abtn[BTN_DEPOSIT].ChangeButtonInfo(m_Pos.x + 32, m_Pos.y + BUTTON_ROW_TOP, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_abtn[BTN_WITHDRAW].ChangeButtonInfo(m_Pos.x + 142, m_Pos.y + BUTTON_ROW_TOP, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_abtn[BTN_OFFER_VALUE].ChangeButtonInfo(m_Pos.x + 252, m_Pos.y + BUTTON_ROW_TOP, BUTTON_WIDTH, BUTTON_HEIGHT);

    m_abtn[BTN_BUY].ChangeButtonInfo(m_Pos.x + 32, m_Pos.y + BUTTON_ROW_TOP, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_abtn[BTN_CANCEL_OFFER].ChangeButtonInfo(m_Pos.x + 142, m_Pos.y + BUTTON_ROW_TOP, BUTTON_WIDTH, BUTTON_HEIGHT);
    m_abtn[BTN_MINE].ChangeButtonInfo(m_Pos.x + 252, m_Pos.y + BUTTON_ROW_TOP, BUTTON_WIDTH, BUTTON_HEIGHT);
}

float SEASON3B::CNewUIBankWindow::GetLayerDepth()
{
    return 2.2f;
}

bool SEASON3B::CNewUIBankWindow::IsVisible() const
{
    return CNewUIObj::IsVisible();
}

void SEASON3B::CNewUIBankWindow::OpeningProcess()
{
    m_page = Page::Items;
    m_itemPage = 0;
    m_selectedSlot = -1;
    m_selectedOffer = -1;
    m_depositSourceSlot = -1;
    m_takeSourceSlot = -1;
    m_pendingInput = PendingInput::None;

    SocketClient->ToGameServer()->SendBankDialog(true);
}

void SEASON3B::CNewUIBankWindow::ClosingProcess()
{
    SocketClient->ToGameServer()->SendBankDialog(false);

    DeleteAllItems();
    m_selectedSlot = -1;
    m_selectedOffer = -1;
    m_depositSourceSlot = -1;
    m_takeSourceSlot = -1;
    m_pendingInput = PendingInput::None;
}

bool SEASON3B::CNewUIBankWindow::InsertItem(int iIndex, std::span<const BYTE> pbyItemPacket)
{
    if (iIndex < 0 || iIndex >= BANK_TOTAL_SLOTS)
    {
        return false;
    }

    ClearBox(iIndex);

    ITEM* pNewItem = g_pNewItemMng->CreateItem(pbyItemPacket);
    if (pNewItem == nullptr)
    {
        return false;
    }

    m_boxes[iIndex] = pNewItem;
    return true;
}

void SEASON3B::CNewUIBankWindow::ProcessToReceiveBankItems(int nIndex, std::span<const BYTE> pbyItemPacket)
{
    InsertItem(nIndex, pbyItemPacket);

    // The server confirmed the move, so the item may leave the box of the inventory it came from.
    if (m_depositSourceSlot >= MAX_EQUIPMENT_INDEX && m_depositSourceSlot < MAX_MY_INVENTORY_INDEX)
    {
        g_pMyInventory->DeleteItem(m_depositSourceSlot);
    }
    else if (m_depositSourceSlot >= MAX_MY_INVENTORY_INDEX && m_depositSourceSlot < MAX_MY_INVENTORY_EX_INDEX)
    {
        g_pMyInventoryExt->DeleteItem(m_depositSourceSlot);
    }

    m_depositSourceSlot = -1;
}

void SEASON3B::CNewUIBankWindow::DeleteAllItems()
{
    for (int slot = 0; slot < BANK_TOTAL_SLOTS; ++slot)
    {
        ClearBox(slot);
    }

    m_selectedSlot = -1;
}

void SEASON3B::CNewUIBankWindow::ClearBox(int slot)
{
    if (slot < 0 || slot >= BANK_TOTAL_SLOTS)
    {
        return;
    }

    if (m_boxes[slot] != nullptr)
    {
        if (g_pNewItemMng)
        {
            g_pNewItemMng->DeleteItem(m_boxes[slot]);
        }

        m_boxes[slot] = nullptr;
    }
}

void SEASON3B::CNewUIBankWindow::ProcessAutoMoveSuccess()
{
    if (m_takeSourceSlot < 0)
    {
        return;
    }

    ClearBox(m_takeSourceSlot);
    if (m_selectedSlot == m_takeSourceSlot)
    {
        m_selectedSlot = -1;
    }

    m_takeSourceSlot = -1;
}

bool SEASON3B::CNewUIBankWindow::ProcessMyInvenItemAutoMove(CNewUIInventoryCtrl* sourceCtrl)
{
    if (!IsVisible())
    {
        return false;
    }

    if (g_pPickedItem && g_pPickedItem->GetItem())
    {
        return false;
    }

    if (m_depositSourceSlot >= 0)
    {
        // A move is still on its way; a second one would take the same box of the bank.
        return false;
    }

    if (sourceCtrl == nullptr)
    {
        sourceCtrl = g_pMyInventory->GetInventoryCtrl();
    }

    if (sourceCtrl == nullptr)
    {
        return false;
    }

    ITEM* pItemObj = sourceCtrl->FindItemAtPt(MouseX, MouseY);
    if (pItemObj == nullptr)
    {
        return false;
    }

    if (pItemObj->Type == ITEM_WIZARDS_RING)
    {
        return false;
    }

    int freeBox = -1;
    for (int slot = 0; slot < BANK_TOTAL_SLOTS; ++slot)
    {
        if (m_boxes[slot] == nullptr)
        {
            freeBox = slot;
            break;
        }
    }

    if (freeBox < 0)
    {
        g_pChatListBox->AddText(L"", I18N::Game::BankIsFull, SEASON3B::TYPE_ERROR_MESSAGE);
        return false;
    }

    const int nSrcIndex = sourceCtrl->GetIndexByItem(pItemObj);
    if (nSrcIndex < 0)
    {
        return false;
    }

    m_depositSourceSlot = nSrcIndex;
    SendRequestEquipmentItem(sourceCtrl->GetStorageType(), nSrcIndex, pItemObj, STORAGE_TYPE::BANK, freeBox);
    PlayBuffer(SOUND_GET_ITEM01);
    return true;
}

void SEASON3B::CNewUIBankWindow::TakeSelectedItem()
{
    if (!HasSelectedItem())
    {
        return;
    }

    ITEM* pItem = m_boxes[m_selectedSlot];
    CNewUIInventoryCtrl* pInventory = g_pMyInventory->GetInventoryCtrl();
    if (pInventory == nullptr)
    {
        return;
    }

    const ITEM_ATTRIBUTE* pItemAttr = &ItemAttribute[pItem->Type];
    const int freeSlot = pInventory->FindEmptySlot(pItemAttr->Width, pItemAttr->Height);
    if (freeSlot < 0)
    {
        g_pChatListBox->AddText(L"", I18N::Game::InventorySpaceIsInsufficient, SEASON3B::TYPE_ERROR_MESSAGE);
        return;
    }

    m_takeSourceSlot = m_selectedSlot;
    SendRequestEquipmentItem(STORAGE_TYPE::BANK, m_selectedSlot, pItem, STORAGE_TYPE::INVENTORY, freeSlot);
    PlayBuffer(SOUND_GET_ITEM01);
}

bool SEASON3B::CNewUIBankWindow::HasSelectedItem()
{
    if (m_selectedSlot >= 0 && m_selectedSlot < BANK_TOTAL_SLOTS && m_boxes[m_selectedSlot] != nullptr)
    {
        return true;
    }

    g_pChatListBox->AddText(L"", I18N::Game::BankSelectAnItemFirst, SEASON3B::TYPE_ERROR_MESSAGE);
    return false;
}

void SEASON3B::CNewUIBankWindow::RequestMarketPage(BYTE page)
{
    SocketClient->ToGameServer()->SendMarketList(page, Net::Bank::AnyCurrency, m_ownOffersOnly, L"");
}

void SEASON3B::CNewUIBankWindow::FinishOffer(int64_t price)
{
    const auto priceCurrency = static_cast<Net::Bank::Currency>(m_selectedCurrency);

    if (m_offerCarriesItem)
    {
        if (m_selectedSlot < 0 || m_boxes[m_selectedSlot] == nullptr)
        {
            return;
        }

        SocketClient->ToGameServer()->SendMarketRegisterItem(static_cast<BYTE>(m_selectedSlot), priceCurrency, price);
    }
    else
    {
        // What is offered cannot also be what is asked for, so a pile of zen is sold for cash and
        // everything else for zen.
        const auto offeredCurrency = static_cast<Net::Bank::Currency>(m_selectedCurrency);
        const Net::Bank::Currency askedCurrency =
            offeredCurrency == Net::Bank::Currency::Zen ? Net::Bank::Currency::WCoinC : Net::Bank::Currency::Zen;
        SocketClient->ToGameServer()->SendMarketRegisterCurrency(offeredCurrency, m_offerAmount, askedCurrency, price);
    }

    m_offerAmount = 0;
}

void SEASON3B::CNewUIBankWindow::FinishValueAmount(int64_t amount)
{
    const auto currency = static_cast<Net::Bank::Currency>(m_selectedCurrency);

    switch (m_pendingInput)
    {
    case PendingInput::DepositAmount:
        SocketClient->ToGameServer()->SendBankMoveValue(true, currency, amount);
        m_pendingInput = PendingInput::None;
        break;
    case PendingInput::WithdrawAmount:
        SocketClient->ToGameServer()->SendBankMoveValue(false, currency, amount);
        m_pendingInput = PendingInput::None;
        break;
    case PendingInput::OfferAmount:
        // How much is offered is known now, so the next dialog asks what it costs.
        m_offerAmount = amount;
        m_pendingInput = PendingInput::Price;
        break;
    default:
        m_pendingInput = PendingInput::None;
        break;
    }
}

void SEASON3B::CNewUIBankWindow::CancelPendingInput()
{
    m_pendingInput = PendingInput::None;
    m_offerAmount = 0;
}

void SEASON3B::CNewUIBankWindow::OpenPendingInput()
{
    switch (m_pendingInput)
    {
    case PendingInput::Price:
    {
        CNewUITextInputMsgBox* pMsgBox = nullptr;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CBankPriceMsgBoxLayout), &pMsgBox);
        m_pendingInput = PendingInput::None;
        break;
    }
    case PendingInput::DepositAmount:
    case PendingInput::WithdrawAmount:
    case PendingInput::OfferAmount:
    {
        CNewUITextInputMsgBox* pMsgBox = nullptr;
        CreateMessageBox(MSGBOX_LAYOUT_CLASS(CBankAmountMsgBoxLayout), &pMsgBox);
        // The kind stays until the answer arrives, because it decides what the amount is for.
        break;
    }
    default:
        break;
    }
}

void SEASON3B::CNewUIBankWindow::ShowRefusedRequest()
{
    g_pChatListBox->AddText(L"", I18N::Game::BankRefusedTheRequest, SEASON3B::TYPE_ERROR_MESSAGE);
}

bool SEASON3B::CNewUIBankWindow::Update()
{
    if (!IsVisible())
    {
        return true;
    }

    Net::Bank::Operation operation = Net::Bank::Operation::Deposit;
    Net::Bank::ResultCode result = Net::Bank::ResultCode::Success;
    if (Net::Bank::Store::Instance().TakeNewResult(operation, result))
    {
        if (result != Net::Bank::ResultCode::Success)
        {
            ShowRefusedRequest();
        }
        else if (operation == Net::Bank::Operation::MarketRegister || operation == Net::Bank::Operation::MarketBuy ||
                 operation == Net::Bank::Operation::MarketCancel)
        {
            m_selectedOffer = -1;
            RequestMarketPage(Net::Bank::Store::Instance().GetOfferPage());
        }
    }

    // A dialog is opened from here and never from the callback of another one, so a message box is
    // never created while the box which asked for it is being destroyed.
    if (m_pendingInput != PendingInput::None && g_MessageBox && !g_MessageBox->IsVisible())
    {
        OpenPendingInput();
    }

    return true;
}

bool SEASON3B::CNewUIBankWindow::UpdateKeyEvent()
{
    if (IsVisible() && SEASON3B::IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(INTERFACE_BANK);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIBankWindow::UpdateMouseEvent()
{
    if (!IsVisible())
    {
        return true;
    }

    if (ProcessTabs())
    {
        return false;
    }

    if (ProcessButtons())
    {
        return false;
    }

    if (m_page == Page::Items && ProcessTileSelection())
    {
        return false;
    }

    if (m_page == Page::Values && ProcessValueSelection())
    {
        return false;
    }

    if (m_page == Page::Market && ProcessOfferSelection())
    {
        return false;
    }

    // The window swallows what happens over it, so a click next to a box does not walk the
    // character to the other side of the map.
    if (SEASON3B::CheckMouseIn(m_Pos.x, m_Pos.y, static_cast<int>(BANK_WIDTH), static_cast<int>(BANK_HEIGHT)))
    {
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIBankWindow::ProcessTabs()
{
    if (m_abtn[BTN_TAB_ITEMS].UpdateMouseEvent())
    {
        m_page = Page::Items;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_TAB_VALUES].UpdateMouseEvent())
    {
        m_page = Page::Values;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_TAB_MARKET].UpdateMouseEvent())
    {
        m_page = Page::Market;
        m_selectedOffer = -1;
        RequestMarketPage(0);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool SEASON3B::CNewUIBankWindow::ProcessButtons()
{
    if (m_page != Page::Values)
    {
        if (m_abtn[BTN_PREV].UpdateMouseEvent())
        {
            if (m_page == Page::Items)
            {
                m_itemPage = m_itemPage > 0 ? m_itemPage - 1 : ITEM_PAGE_COUNT - 1;
            }
            else
            {
                const BYTE page = Net::Bank::Store::Instance().GetOfferPage();
                RequestMarketPage(page > 0 ? static_cast<BYTE>(page - 1) : 0);
            }

            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        if (m_abtn[BTN_NEXT].UpdateMouseEvent())
        {
            if (m_page == Page::Items)
            {
                m_itemPage = (m_itemPage + 1) % ITEM_PAGE_COUNT;
            }
            else
            {
                const auto& store = Net::Bank::Store::Instance();
                const BYTE page = store.GetOfferPage();
                if (page + 1 < store.GetOfferPageCount())
                {
                    RequestMarketPage(static_cast<BYTE>(page + 1));
                }
            }

            PlayBuffer(SOUND_CLICK01);
            return true;
        }
    }

    switch (m_page)
    {
    case Page::Items:
        return ProcessItemsPageButtons();
    case Page::Values:
        return ProcessValuesPageButtons();
    case Page::Market:
        return ProcessMarketPageButtons();
    }

    return false;
}

bool SEASON3B::CNewUIBankWindow::ProcessItemsPageButtons()
{
    if (m_abtn[BTN_TAKE_ITEM].UpdateMouseEvent())
    {
        TakeSelectedItem();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_OFFER_ITEM].UpdateMouseEvent())
    {
        if (HasSelectedItem())
        {
            m_offerCarriesItem = true;
            m_pendingInput = PendingInput::Price;
        }

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool SEASON3B::CNewUIBankWindow::ProcessValuesPageButtons()
{
    if (m_abtn[BTN_DEPOSIT].UpdateMouseEvent())
    {
        m_pendingInput = PendingInput::DepositAmount;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_WITHDRAW].UpdateMouseEvent())
    {
        m_pendingInput = PendingInput::WithdrawAmount;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_OFFER_VALUE].UpdateMouseEvent())
    {
        m_offerCarriesItem = false;
        m_pendingInput = PendingInput::OfferAmount;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool SEASON3B::CNewUIBankWindow::ProcessMarketPageButtons()
{
    if (m_abtn[BTN_BUY].UpdateMouseEvent())
    {
        BuySelectedOffer();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_CANCEL_OFFER].UpdateMouseEvent())
    {
        CancelSelectedOffer();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_MINE].UpdateMouseEvent())
    {
        m_ownOffersOnly = !m_ownOffersOnly;
        m_selectedOffer = -1;
        RequestMarketPage(0);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

void SEASON3B::CNewUIBankWindow::BuySelectedOffer()
{
    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    if (m_selectedOffer < 0 || m_selectedOffer >= static_cast<int>(offers.size()))
    {
        return;
    }

    SocketClient->ToGameServer()->SendMarketBuy(offers[m_selectedOffer].ListingId.data());
}

void SEASON3B::CNewUIBankWindow::CancelSelectedOffer()
{
    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    if (m_selectedOffer < 0 || m_selectedOffer >= static_cast<int>(offers.size()))
    {
        return;
    }

    SocketClient->ToGameServer()->SendMarketCancel(offers[m_selectedOffer].ListingId.data());
}

void SEASON3B::CNewUIBankWindow::GetTileRect(int slotOnPage, RECT& rect) const
{
    const int column = slotOnPage % TILE_COLUMNS;
    const int row = slotOnPage / TILE_COLUMNS;

    rect.left = m_Pos.x + TILE_ORIGIN_X + column * TILE_SIZE;
    rect.top = m_Pos.y + CONTENT_TOP + row * TILE_SIZE;
    rect.right = rect.left + TILE_SIZE;
    rect.bottom = rect.top + TILE_SIZE;
}

int SEASON3B::CNewUIBankWindow::GetCurrencyRowTop(int currency) const
{
    const bool isMoney = currency < MONEY_CURRENCY_COUNT;
    const int rowInGroup = isMoney ? currency : currency - MONEY_CURRENCY_COUNT;
    return m_Pos.y + CONTENT_TOP + (isMoney ? MONEY_ROWS_TOP : JEWEL_ROWS_TOP) + rowInGroup * LIST_LINE_HEIGHT;
}

int SEASON3B::CNewUIBankWindow::GetOfferRowTop(int row) const
{
    return m_Pos.y + CONTENT_TOP + MONEY_ROWS_TOP + row * LIST_LINE_HEIGHT;
}

int SEASON3B::CNewUIBankWindow::GetTileAtCursor() const
{
    for (int slotOnPage = 0; slotOnPage < ITEMS_PER_PAGE; ++slotOnPage)
    {
        RECT rect;
        GetTileRect(slotOnPage, rect);
        if (SEASON3B::CheckMouseIn(rect.left, rect.top, TILE_SIZE, TILE_SIZE))
        {
            return m_itemPage * ITEMS_PER_PAGE + slotOnPage;
        }
    }

    return -1;
}

bool SEASON3B::CNewUIBankWindow::ProcessTileSelection()
{
    const int slot = GetTileAtCursor();
    if (slot < 0)
    {
        return false;
    }

    if (SEASON3B::IsRelease(VK_LBUTTON))
    {
        m_selectedSlot = m_boxes[slot] != nullptr ? slot : -1;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    // The cursor is over a box, so nothing behind the window may take this frame.
    return true;
}

bool SEASON3B::CNewUIBankWindow::ProcessValueSelection()
{
    for (int currency = 0; currency < static_cast<int>(Net::Bank::Currency::Count); ++currency)
    {
        const int top = GetCurrencyRowTop(currency);

        if (SEASON3B::CheckMouseIn(m_Pos.x + 16, top, static_cast<int>(BANK_WIDTH) - 32, LIST_LINE_HEIGHT))
        {
            if (SEASON3B::IsRelease(VK_LBUTTON))
            {
                m_selectedCurrency = currency;
                PlayBuffer(SOUND_CLICK01);
            }

            return true;
        }
    }

    return false;
}

bool SEASON3B::CNewUIBankWindow::ProcessOfferSelection()
{
    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    const int rows = static_cast<int>(offers.size()) < MARKET_ROWS ? static_cast<int>(offers.size()) : MARKET_ROWS;

    for (int row = 0; row < rows; ++row)
    {
        const int top = GetOfferRowTop(row);
        if (SEASON3B::CheckMouseIn(m_Pos.x + 16, top, static_cast<int>(BANK_WIDTH) - 32, LIST_LINE_HEIGHT))
        {
            if (SEASON3B::IsRelease(VK_LBUTTON))
            {
                m_selectedOffer = row;
                PlayBuffer(SOUND_CLICK01);
            }

            return true;
        }
    }

    return false;
}

bool SEASON3B::CNewUIBankWindow::Render()
{
    if (!IsVisible())
    {
        return true;
    }

    EnableAlphaTest();

    RenderFrame();
    RenderTabs();

    switch (m_page)
    {
    case Page::Items:
        RenderItemsPage();
        break;
    case Page::Values:
        RenderValuesPage();
        break;
    case Page::Market:
        RenderMarketPage();
        break;
    }

    DisableAlphaBlend();
    return true;
}

void SEASON3B::CNewUIBankWindow::RenderFrame()
{
    RenderImage(IMAGE_BANK_BACK, m_Pos.x, m_Pos.y, BANK_WIDTH, BANK_HEIGHT);
    RenderImage(IMAGE_BANK_TOP, m_Pos.x, m_Pos.y, BANK_WIDTH, 64.f);
    RenderImage(IMAGE_BANK_LEFT, m_Pos.x, m_Pos.y + 64, 21.f, BANK_HEIGHT - 109.f);
    RenderImage(IMAGE_BANK_RIGHT, m_Pos.x + BANK_WIDTH - 21, m_Pos.y + 64, 21.f, BANK_HEIGHT - 109.f);
    RenderImage(IMAGE_BANK_BOTTOM, m_Pos.x, m_Pos.y + BANK_HEIGHT - 45, BANK_WIDTH, 45.f);

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(230, 230, 230, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + 10, I18N::Game::Bank, static_cast<int>(BANK_WIDTH), 0,
                              RT3_SORT_CENTER);
}

void SEASON3B::CNewUIBankWindow::RenderTabs()
{
    // The tab which is shown is the one which is not drawn as a button the player can press.
    m_abtn[BTN_TAB_ITEMS].ChangeTextColor(m_page == Page::Items ? RGBA(255, 210, 80, 255) : RGBA(220, 220, 220, 255));
    m_abtn[BTN_TAB_VALUES].ChangeTextColor(m_page == Page::Values ? RGBA(255, 210, 80, 255) : RGBA(220, 220, 220, 255));
    m_abtn[BTN_TAB_MARKET].ChangeTextColor(m_page == Page::Market ? RGBA(255, 210, 80, 255) : RGBA(220, 220, 220, 255));

    m_abtn[BTN_TAB_ITEMS].Render();
    m_abtn[BTN_TAB_VALUES].Render();
    m_abtn[BTN_TAB_MARKET].Render();
}

void SEASON3B::CNewUIBankWindow::RenderItemsPage()
{
    const int hovered = GetTileAtCursor();

    for (int slotOnPage = 0; slotOnPage < ITEMS_PER_PAGE; ++slotOnPage)
    {
        const int slot = m_itemPage * ITEMS_PER_PAGE + slotOnPage;
        RECT rect;
        GetTileRect(slotOnPage, rect);

        unsigned int color = EMPTY_BOX_COLOR;
        if (slot == m_selectedSlot && m_boxes[slot] != nullptr)
        {
            color = PICKED_COLOR;
        }
        else if (slot == hovered)
        {
            color = HOVERED_COLOR;
        }

        RenderColorQuadARGB(static_cast<float>(rect.left + 1), static_cast<float>(rect.top + 1),
                            static_cast<float>(TILE_SIZE - 2), static_cast<float>(TILE_SIZE - 2), color);
        EndRenderColor();
    }

    wchar_t szText[256] = {0};
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);

    // What is picked, and what a price would be asked in, because that is what the market needs.
    g_pRenderText->SetTextColor(255, 210, 80, 255);
    if (m_selectedSlot >= 0 && m_boxes[m_selectedSlot] != nullptr)
    {
        GetItemName(m_boxes[m_selectedSlot]->Type, m_boxes[m_selectedSlot]->Level, szText);
    }
    else
    {
        mu_swprintf(szText, L"%ls", GetCurrencyName(static_cast<Net::Bank::Currency>(m_selectedCurrency)));
    }

    g_pRenderText->RenderText(m_Pos.x + 20, m_Pos.y + INFO_ROW_TOP, szText, static_cast<int>(BANK_WIDTH) - 40, 0,
                              RT3_SORT_CENTER);

    int used = 0;
    for (int slot = 0; slot < BANK_TOTAL_SLOTS; ++slot)
    {
        if (m_boxes[slot] != nullptr)
        {
            ++used;
        }
    }

    g_pRenderText->SetTextColor(200, 200, 200, 255);
    mu_swprintf(szText, I18N::Game::BankBoxCount, used, BANK_TOTAL_SLOTS);
    g_pRenderText->RenderText(m_Pos.x + 20, m_Pos.y + INFO_ROW_TOP - 18, szText, 140, 0);

    mu_swprintf(szText, I18N::Game::BankPageOf, m_itemPage + 1, ITEM_PAGE_COUNT);
    g_pRenderText->RenderText(m_Pos.x + 70, m_Pos.y + PAGE_ROW_TOP + 3, szText, 240, 0, RT3_SORT_CENTER);

    m_abtn[BTN_PREV].Render();
    m_abtn[BTN_NEXT].Render();
    m_abtn[BTN_TAKE_ITEM].Render();
    m_abtn[BTN_OFFER_ITEM].Render();

    RenderHoveredItemInfo();
}

void SEASON3B::CNewUIBankWindow::RenderValuesPage()
{
    wchar_t szText[256] = {0};
    wchar_t szAmount[64] = {0};

    g_pRenderText->SetBgColor(0);

    for (int group = 0; group < 2; ++group)
    {
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(160, 200, 255, 255);
        g_pRenderText->RenderText(m_Pos.x + 20, m_Pos.y + CONTENT_TOP + (group == 0 ? 0 : JEWEL_HEADER_TOP),
                                  group == 0 ? I18N::Game::BankMoneyGroup : I18N::Game::BankJewelGroup, 240, 0);

        const int first = group == 0 ? 0 : MONEY_CURRENCY_COUNT;
        const int last = group == 0 ? MONEY_CURRENCY_COUNT : static_cast<int>(Net::Bank::Currency::Count);

        g_pRenderText->SetFont(g_hFont);
        for (int currency = first; currency < last; ++currency)
        {
            const int top = GetCurrencyRowTop(currency);

            if (currency == m_selectedCurrency)
            {
                RenderColorQuadARGB(static_cast<float>(m_Pos.x + 16), static_cast<float>(top), BANK_WIDTH - 32.f,
                                    static_cast<float>(LIST_LINE_HEIGHT), PICKED_COLOR);
                EndRenderColor();
                g_pRenderText->SetTextColor(255, 230, 150, 255);
            }
            else
            {
                g_pRenderText->SetTextColor(220, 220, 220, 255);
            }

            g_pRenderText->RenderText(m_Pos.x + 26, top + 2,
                                      GetCurrencyName(static_cast<Net::Bank::Currency>(currency)), 180, 0);

            FormatAmount(Net::Bank::Store::Instance().GetBalance(static_cast<Net::Bank::Currency>(currency)), szAmount,
                         std::size(szAmount));
            mu_swprintf(szText, L"%ls", szAmount);
            g_pRenderText->RenderText(m_Pos.x + 206, top + 2, szText, 148, 0, RT3_SORT_RIGHT);
        }
    }

    m_abtn[BTN_DEPOSIT].Render();
    m_abtn[BTN_WITHDRAW].Render();
    m_abtn[BTN_OFFER_VALUE].Render();
}

void SEASON3B::CNewUIBankWindow::RenderMarketPage()
{
    wchar_t szText[256] = {0};
    wchar_t szAmount[64] = {0};

    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(160, 200, 255, 255);
    g_pRenderText->RenderText(m_Pos.x + 26, m_Pos.y + CONTENT_TOP, I18N::Game::BankSellerColumn, 90, 0);
    g_pRenderText->RenderText(m_Pos.x + 116, m_Pos.y + CONTENT_TOP, I18N::Game::BankItemColumn, 150, 0);
    g_pRenderText->RenderText(m_Pos.x + 236, m_Pos.y + CONTENT_TOP, I18N::Game::BankPriceColumn, 118, 0,
                              RT3_SORT_RIGHT);

    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    g_pRenderText->SetFont(g_hFont);

    if (offers.empty())
    {
        g_pRenderText->SetTextColor(180, 180, 180, 255);
        g_pRenderText->RenderText(m_Pos.x + 20, m_Pos.y + CONTENT_TOP + 80, I18N::Game::BankHasNoOffers,
                                  static_cast<int>(BANK_WIDTH) - 40, 0, RT3_SORT_CENTER);
    }

    const int rows = static_cast<int>(offers.size()) < MARKET_ROWS ? static_cast<int>(offers.size()) : MARKET_ROWS;
    for (int row = 0; row < rows; ++row)
    {
        const Net::Bank::Offer& offer = offers[row];
        const int top = GetOfferRowTop(row);

        if (row == m_selectedOffer)
        {
            RenderColorQuadARGB(static_cast<float>(m_Pos.x + 16), static_cast<float>(top), BANK_WIDTH - 32.f,
                                static_cast<float>(LIST_LINE_HEIGHT), PICKED_COLOR);
            EndRenderColor();
            g_pRenderText->SetTextColor(255, 230, 150, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(220, 220, 220, 255);
        }

        g_pRenderText->RenderText(m_Pos.x + 26, top + 2, offer.SellerName.c_str(), 90, 0);
        g_pRenderText->RenderText(m_Pos.x + 116, top + 2, offer.OfferName.c_str(), 150, 0);

        FormatAmount(offer.PriceAmount, szAmount, std::size(szAmount));
        mu_swprintf(szText, L"%ls %ls", szAmount, GetCurrencyName(offer.PriceCurrency));
        g_pRenderText->RenderText(m_Pos.x + 236, top + 2, szText, 118, 0, RT3_SORT_RIGHT);
    }

    g_pRenderText->SetTextColor(200, 200, 200, 255);
    mu_swprintf(szText, I18N::Game::BankPageOf, Net::Bank::Store::Instance().GetOfferPage() + 1,
                Net::Bank::Store::Instance().GetOfferPageCount());
    g_pRenderText->RenderText(m_Pos.x + 70, m_Pos.y + PAGE_ROW_TOP + 3, szText, 240, 0, RT3_SORT_CENTER);

    m_abtn[BTN_MINE].ChangeTextColor(m_ownOffersOnly ? RGBA(255, 210, 80, 255) : RGBA(220, 220, 220, 255));

    m_abtn[BTN_PREV].Render();
    m_abtn[BTN_NEXT].Render();
    m_abtn[BTN_BUY].Render();
    m_abtn[BTN_CANCEL_OFFER].Render();
    m_abtn[BTN_MINE].Render();
}

void SEASON3B::CNewUIBankWindow::RenderHoveredItemInfo()
{
    if (m_pNewUI3DRenderMng == nullptr || (g_pPickedItem && g_pPickedItem->GetItem()))
    {
        return;
    }

    const int slot = GetTileAtCursor();
    if (slot < 0 || m_boxes[slot] == nullptr)
    {
        return;
    }

    m_pNewUI3DRenderMng->RenderUI2DEffect(INVENTORY_CAMERA_Z_ORDER, UI2DEffectCallback, this, RENDER_ITEM_TOOLTIP, 0);
}

void SEASON3B::CNewUIBankWindow::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB)
{
    auto* pWindow = static_cast<CNewUIBankWindow*>(pClass);
    if (pWindow == nullptr || dwParamA != RENDER_ITEM_TOOLTIP)
    {
        return;
    }

    const int slot = pWindow->GetTileAtCursor();
    if (slot < 0 || pWindow->m_boxes[slot] == nullptr)
    {
        return;
    }

    RECT rect;
    pWindow->GetTileRect(slot - pWindow->m_itemPage * ITEMS_PER_PAGE, rect);
    RenderItemInfo(rect.left + TILE_SIZE / 2, rect.top, pWindow->m_boxes[slot], false);
}

void SEASON3B::CNewUIBankWindow::Render3D()
{
    if (!IsVisible() || m_page != Page::Items)
    {
        return;
    }

    for (int slotOnPage = 0; slotOnPage < ITEMS_PER_PAGE; ++slotOnPage)
    {
        const int slot = m_itemPage * ITEMS_PER_PAGE + slotOnPage;
        const ITEM* pItem = m_boxes[slot];
        if (pItem == nullptr)
        {
            continue;
        }

        RECT rect;
        GetTileRect(slotOnPage, rect);

        // Every box is the same size, so the picture keeps the proportions of the item and is
        // drawn as large as it can be inside its box.
        const ITEM_ATTRIBUTE* pItemAttr = &ItemAttribute[pItem->Type];
        const int columns = pItemAttr->Width > 0 ? pItemAttr->Width : 1;
        const int rows = pItemAttr->Height > 0 ? pItemAttr->Height : 1;
        const float scale = static_cast<float>(TILE_SIZE - 8) / static_cast<float>(columns > rows ? columns : rows);
        const float width = columns * scale;
        const float height = rows * scale;
        const float x = rect.left + (TILE_SIZE - width) / 2.f;
        const float y = rect.top + (TILE_SIZE - height) / 2.f;

        RenderItem3D(x, y, width, height, pItem->Type, pItem->Level, pItem->ExcellentFlags, pItem->AncientDiscriminator,
                     false);
    }
}

int SEASON3B::CNewUIBankWindow::GetMoneyCurrencyCount()
{
    return MONEY_CURRENCY_COUNT;
}

const wchar_t* SEASON3B::CNewUIBankWindow::GetCurrencyName(Net::Bank::Currency currency)
{
    switch (currency)
    {
    case Net::Bank::Currency::Zen:
        return I18N::Game::BankCurrencyZen;
    case Net::Bank::Currency::WCoinC:
        return I18N::Game::BankCurrencyWcoinC;
    case Net::Bank::Currency::WCoinP:
        return I18N::Game::BankCurrencyWcoinP;
    case Net::Bank::Currency::GoblinPoints:
        return I18N::Game::BankCurrencyGoblinPoints;
    case Net::Bank::Currency::JewelOfBless:
        return I18N::Game::BankCurrencyJewelOfBless;
    case Net::Bank::Currency::JewelOfSoul:
        return I18N::Game::BankCurrencyJewelOfSoul;
    case Net::Bank::Currency::JewelOfLife:
        return I18N::Game::BankCurrencyJewelOfLife;
    case Net::Bank::Currency::JewelOfCreation:
        return I18N::Game::BankCurrencyJewelOfCreation;
    case Net::Bank::Currency::JewelOfChaos:
        return I18N::Game::BankCurrencyJewelOfChaos;
    default:
        return L"";
    }
}
