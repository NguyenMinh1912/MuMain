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

#include <algorithm>

namespace
{
/// <summary>The body of the window, dark enough for the world behind it to stay behind it.</summary>
constexpr unsigned int PANEL_COLOR = 0xEE0E0F12u;

/// <summary>The bronze edge of the window and the darker line just inside it.</summary>
constexpr unsigned int PANEL_EDGE_COLOR = 0xFF6B5A3Cu;
constexpr unsigned int PANEL_INNER_COLOR = 0xFF241F16u;

/// <summary>The area the boxes stand in, and the lines between them.</summary>
constexpr unsigned int GRID_BACK_COLOR = 0x99000000u;
constexpr unsigned int GRID_FRAME_COLOR = 0xFF4A4235u;
constexpr unsigned int TILE_COLOR = 0xFF161A20u;
constexpr unsigned int TILE_LINE_COLOR = 0xFF2F3640u;

/// <summary>What is picked, and what the cursor is over.</summary>
constexpr unsigned int PICKED_FILL_COLOR = 0x40FFC83Cu;
constexpr unsigned int PICKED_EDGE_COLOR = 0xFFFFC83Cu;
constexpr unsigned int HOVERED_FILL_COLOR = 0x22FFFFFFu;

/// <summary>The plate of a button, in its three states, and its edge.</summary>
constexpr unsigned int BUTTON_UP_COLOR = 0xFF433D31u;
constexpr unsigned int BUTTON_OVER_COLOR = 0xFF564E3Eu;
constexpr unsigned int BUTTON_DOWN_COLOR = 0xFF2B261Du;
constexpr unsigned int BUTTON_EDGE_COLOR = 0xFF8A7444u;
constexpr unsigned int BUTTON_EDGE_ON_COLOR = 0xFFFFC83Cu;

/// <summary>The line which separates a heading from the rows under it.</summary>
constexpr unsigned int HEADING_LINE_COLOR = 0x804A5566u;

/// <summary>How many of the currencies are money; the rest are jewels.</summary>
constexpr int MONEY_CURRENCY_COUNT = 4;

/// <summary>Draws the four edges of a rectangle, which is the only border this window needs.</summary>
void RenderBorder(int x, int y, int width, int height, unsigned int color, int thickness = 1)
{
    const float fx = static_cast<float>(x);
    const float fy = static_cast<float>(y);
    const float fw = static_cast<float>(width);
    const float fh = static_cast<float>(height);
    const float ft = static_cast<float>(thickness);

    RenderColorQuadARGB(fx, fy, fw, ft, color);
    RenderColorQuadARGB(fx, fy + fh - ft, fw, ft, color);
    RenderColorQuadARGB(fx, fy + ft, ft, fh - 2.f * ft, color);
    RenderColorQuadARGB(fx + fw - ft, fy + ft, ft, fh - 2.f * ft, color);
}

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
    : m_pNewUIMng(nullptr), m_pNewUI3DRenderMng(nullptr), m_layout{}, m_page(Page::Items), m_itemPage(0),
      m_selectedCurrency(0), m_selectedOffer(-1), m_selectedSlot(-1), m_depositSourceSlot(-1), m_takeSourceSlot(-1),
      m_ownOffersOnly(false), m_pendingInput(PendingInput::None), m_offerCarriesItem(true), m_offerAmount(0)
{
    m_Pos.x = 0;
    m_Pos.y = 0;
    m_boxes.fill(nullptr);
    BuildLayout();
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

    SetPos(x, y);

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
    // The button gets no image: this window draws the plate under every caption itself, because the
    // images of the original dialogs are cut for one size and nothing else.
    pButton->ChangeText(captionSlot);
    pButton->ChangeTextBackColor(RGBA(255, 255, 255, 0));
    pButton->ChangeTextColor(RGBA(240, 228, 200, 255));
}

void SEASON3B::CNewUIBankWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    BuildLayout();
    ApplyLayoutToButtons();
}

void SEASON3B::CNewUIBankWindow::BuildLayout()
{
    Layout& layout = m_layout;
    layout.width = BANK_WIDTH;
    layout.height = BANK_HEIGHT;

    // What the frame keeps for itself, and what stands between two things side by side.
    constexpr int edge = 10;
    constexpr int gap = 6;
    constexpr int titleHeight = 28;
    constexpr int tabHeight = 24;
    constexpr int buttonHeight = 26;
    constexpr int pageButtonWidth = 44;
    constexpr int pageButtonHeight = 20;

    const int tabWidth = std::max(1, (layout.width - 2 * edge - 2 * gap) / 3);
    for (int index = 0; index < 3; ++index)
    {
        layout.tab[index].left = edge + index * (tabWidth + gap);
        layout.tab[index].top = titleHeight;
        layout.tab[index].right = layout.tab[index].left + tabWidth;
        layout.tab[index].bottom = titleHeight + tabHeight;
    }

    // The row of buttons stands at the bottom; everything else fills what is left above it.
    const int buttonRowTop = layout.height - buttonHeight - 16;
    const int buttonWidth = tabWidth;
    for (int index = 0; index < 3; ++index)
    {
        layout.button[index].left = edge + index * (buttonWidth + gap);
        layout.button[index].top = buttonRowTop;
        layout.button[index].right = layout.button[index].left + buttonWidth;
        layout.button[index].bottom = buttonRowTop + buttonHeight;
    }

    layout.pageRowTop = buttonRowTop - 26;
    layout.prevButton = {edge, layout.pageRowTop, edge + pageButtonWidth, layout.pageRowTop + pageButtonHeight};
    layout.nextButton = {layout.width - edge - pageButtonWidth, layout.pageRowTop, layout.width - edge,
                         layout.pageRowTop + pageButtonHeight};
    layout.infoRowTop = layout.pageRowTop - 20;

    layout.contentTop = titleHeight + tabHeight + 8;
    layout.contentBottom = layout.infoRowTop - 4;

    // The boxes: as many as the room holds, and how many pages there are follows from that.
    layout.tileSize = 68;
    layout.tileColumns = std::max(1, (layout.width - 2 * edge - 8) / layout.tileSize);
    layout.tileRows = std::max(1, (layout.contentBottom - layout.contentTop) / layout.tileSize);
    layout.itemsPerPage = layout.tileColumns * layout.tileRows;
    layout.itemPageCount = (BANK_TOTAL_SLOTS + layout.itemsPerPage - 1) / layout.itemsPerPage;
    layout.tileOriginX = (layout.width - layout.tileColumns * layout.tileSize) / 2;
    layout.tileOriginY = layout.contentTop;

    // The two lists share a line height; each starts under its own heading.
    layout.listLineHeight = 22;
    layout.moneyHeaderTop = layout.contentTop;
    layout.moneyRowsTop = layout.moneyHeaderTop + 24;
    layout.jewelHeaderTop = layout.moneyRowsTop + MONEY_CURRENCY_COUNT * layout.listLineHeight + 14;
    layout.jewelRowsTop = layout.jewelHeaderTop + 24;

    layout.marketHeaderTop = layout.contentTop;
    layout.marketRowsTop = layout.marketHeaderTop + 24;
    layout.marketRows = std::max(1, (layout.contentBottom - layout.marketRowsTop) / layout.listLineHeight);
}

void SEASON3B::CNewUIBankWindow::ApplyLayoutToButtons()
{
    const auto place = [this](BANK_BUTTON button, const RECT& rect)
    {
        m_abtn[button].ChangeButtonInfo(m_Pos.x + rect.left, m_Pos.y + rect.top, rect.right - rect.left,
                                        rect.bottom - rect.top);
    };

    place(BTN_TAB_ITEMS, m_layout.tab[0]);
    place(BTN_TAB_VALUES, m_layout.tab[1]);
    place(BTN_TAB_MARKET, m_layout.tab[2]);

    place(BTN_PREV, m_layout.prevButton);
    place(BTN_NEXT, m_layout.nextButton);

    // Two buttons stand in the middle of the row, three fill it.
    RECT left = m_layout.button[0];
    RECT right = m_layout.button[2];
    const int half = (m_layout.button[1].left - m_layout.button[0].left) / 2;
    left.left += half;
    left.right += half;
    right.left -= half;
    right.right -= half;

    place(BTN_TAKE_ITEM, left);
    place(BTN_OFFER_ITEM, right);

    place(BTN_DEPOSIT, m_layout.button[0]);
    place(BTN_WITHDRAW, m_layout.button[1]);
    place(BTN_OFFER_VALUE, m_layout.button[2]);

    place(BTN_BUY, m_layout.button[0]);
    place(BTN_CANCEL_OFFER, m_layout.button[1]);
    place(BTN_MINE, m_layout.button[2]);
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
    if (SEASON3B::CheckMouseIn(m_Pos.x, m_Pos.y, m_layout.width, m_layout.height))
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
                m_itemPage = m_itemPage > 0 ? m_itemPage - 1 : m_layout.itemPageCount - 1;
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
                m_itemPage = (m_itemPage + 1) % m_layout.itemPageCount;
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
    const int column = slotOnPage % m_layout.tileColumns;
    const int row = slotOnPage / m_layout.tileColumns;

    rect.left = m_Pos.x + m_layout.tileOriginX + column * m_layout.tileSize;
    rect.top = m_Pos.y + m_layout.tileOriginY + row * m_layout.tileSize;
    rect.right = rect.left + m_layout.tileSize;
    rect.bottom = rect.top + m_layout.tileSize;
}

int SEASON3B::CNewUIBankWindow::GetCurrencyRowTop(int currency) const
{
    const bool isMoney = currency < MONEY_CURRENCY_COUNT;
    const int rowInGroup = isMoney ? currency : currency - MONEY_CURRENCY_COUNT;
    return m_Pos.y + (isMoney ? m_layout.moneyRowsTop : m_layout.jewelRowsTop) + rowInGroup * m_layout.listLineHeight;
}

int SEASON3B::CNewUIBankWindow::GetOfferRowTop(int row) const
{
    return m_Pos.y + m_layout.marketRowsTop + row * m_layout.listLineHeight;
}

int SEASON3B::CNewUIBankWindow::GetTileAtCursor() const
{
    for (int slotOnPage = 0; slotOnPage < m_layout.itemsPerPage; ++slotOnPage)
    {
        const int slot = m_itemPage * m_layout.itemsPerPage + slotOnPage;
        if (slot >= BANK_TOTAL_SLOTS)
        {
            break;
        }

        RECT rect;
        GetTileRect(slotOnPage, rect);
        if (SEASON3B::CheckMouseIn(rect.left, rect.top, m_layout.tileSize, m_layout.tileSize))
        {
            return slot;
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
        if (SEASON3B::CheckMouseIn(m_Pos.x + 16, top, m_layout.width - 32, m_layout.listLineHeight))
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
    const int rows = std::min(static_cast<int>(offers.size()), m_layout.marketRows);

    for (int row = 0; row < rows; ++row)
    {
        const int top = GetOfferRowTop(row);
        if (SEASON3B::CheckMouseIn(m_Pos.x + 16, top, m_layout.width - 32, m_layout.listLineHeight))
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
    const float x = static_cast<float>(m_Pos.x);
    const float y = static_cast<float>(m_Pos.y);
    const float width = static_cast<float>(m_layout.width);
    const float height = static_cast<float>(m_layout.height);

    RenderColorQuadARGB(x, y, width, height, PANEL_COLOR);
    RenderBorder(m_Pos.x, m_Pos.y, m_layout.width, m_layout.height, PANEL_EDGE_COLOR, 2);
    RenderBorder(m_Pos.x + 2, m_Pos.y + 2, m_layout.width - 4, m_layout.height - 4, PANEL_INNER_COLOR, 1);
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(240, 220, 164, 255);
    g_pRenderText->RenderText(m_Pos.x, m_Pos.y + 8, I18N::Game::Bank, m_layout.width, 0, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIBankWindow::RenderButton(CNewUIButton& button, bool highlighted)
{
    const POINT& pos = button.GetPos();
    const POINT& size = button.GetSize();
    const BUTTON_STATE state = button.GetBTState();

    unsigned int plate = BUTTON_UP_COLOR;
    if (state == BUTTON_STATE_DOWN)
    {
        plate = BUTTON_DOWN_COLOR;
    }
    else if (state == BUTTON_STATE_OVER)
    {
        plate = BUTTON_OVER_COLOR;
    }

    RenderColorQuadARGB(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(size.x),
                        static_cast<float>(size.y), plate);
    RenderBorder(pos.x, pos.y, size.x, size.y, highlighted ? BUTTON_EDGE_ON_COLOR : BUTTON_EDGE_COLOR, 1);
    EndRenderColor();

    button.ChangeTextColor(highlighted ? RGBA(255, 210, 76, 255) : RGBA(232, 220, 192, 255));
    button.Render();
}

void SEASON3B::CNewUIBankWindow::RenderTabs()
{
    RenderButton(m_abtn[BTN_TAB_ITEMS], m_page == Page::Items);
    RenderButton(m_abtn[BTN_TAB_VALUES], m_page == Page::Values);
    RenderButton(m_abtn[BTN_TAB_MARKET], m_page == Page::Market);
}

void SEASON3B::CNewUIBankWindow::RenderItemsPage()
{
    const int gridX = m_Pos.x + m_layout.tileOriginX;
    const int gridY = m_Pos.y + m_layout.tileOriginY;
    const int gridWidth = m_layout.tileColumns * m_layout.tileSize;
    const int gridHeight = m_layout.tileRows * m_layout.tileSize;

    RenderColorQuadARGB(static_cast<float>(gridX), static_cast<float>(gridY), static_cast<float>(gridWidth),
                        static_cast<float>(gridHeight), GRID_BACK_COLOR);

    const int hovered = GetTileAtCursor();
    for (int slotOnPage = 0; slotOnPage < m_layout.itemsPerPage; ++slotOnPage)
    {
        const int slot = m_itemPage * m_layout.itemsPerPage + slotOnPage;
        RECT rect;
        GetTileRect(slotOnPage, rect);

        unsigned int fill = TILE_COLOR;
        if (slot >= BANK_TOTAL_SLOTS)
        {
            // The last page can hold more boxes than the bank has; those stand empty and dark.
            fill = GRID_BACK_COLOR;
        }
        else if (slot == m_selectedSlot && m_boxes[slot] != nullptr)
        {
            fill = PICKED_FILL_COLOR;
        }
        else if (slot == hovered)
        {
            fill = HOVERED_FILL_COLOR;
        }

        RenderColorQuadARGB(static_cast<float>(rect.left + 1), static_cast<float>(rect.top + 1),
                            static_cast<float>(m_layout.tileSize - 2), static_cast<float>(m_layout.tileSize - 2), fill);
        RenderBorder(rect.left, rect.top, m_layout.tileSize, m_layout.tileSize, TILE_LINE_COLOR, 1);

        if (slot == m_selectedSlot && slot < BANK_TOTAL_SLOTS && m_boxes[slot] != nullptr)
        {
            RenderBorder(rect.left, rect.top, m_layout.tileSize, m_layout.tileSize, PICKED_EDGE_COLOR, 2);
        }
    }

    RenderBorder(gridX - 4, gridY - 4, gridWidth + 8, gridHeight + 8, GRID_FRAME_COLOR, 2);
    EndRenderColor();

    wchar_t szText[256] = {0};
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);

    int used = 0;
    for (int slot = 0; slot < BANK_TOTAL_SLOTS; ++slot)
    {
        if (m_boxes[slot] != nullptr)
        {
            ++used;
        }
    }

    g_pRenderText->SetTextColor(159, 167, 155, 255);
    mu_swprintf(szText, I18N::Game::BankBoxCount, used, BANK_TOTAL_SLOTS);
    g_pRenderText->RenderText(m_Pos.x + 16, m_Pos.y + m_layout.infoRowTop, szText, 110, 0);

    // What is picked, or which currency a price would be asked in when nothing is.
    g_pRenderText->SetTextColor(255, 210, 76, 255);
    if (m_selectedSlot >= 0 && m_boxes[m_selectedSlot] != nullptr)
    {
        GetItemName(m_boxes[m_selectedSlot]->Type, m_boxes[m_selectedSlot]->Level, szText);
    }
    else
    {
        mu_swprintf(szText, L"%ls", GetCurrencyName(static_cast<Net::Bank::Currency>(m_selectedCurrency)));
    }

    g_pRenderText->RenderText(m_Pos.x + 130, m_Pos.y + m_layout.infoRowTop, szText, m_layout.width - 146, 0,
                              RT3_SORT_CENTER);

    g_pRenderText->SetTextColor(200, 194, 180, 255);
    mu_swprintf(szText, I18N::Game::BankPageOf, m_itemPage + 1, m_layout.itemPageCount);
    g_pRenderText->RenderText(m_Pos.x + m_layout.prevButton.right, m_Pos.y + m_layout.pageRowTop + 3, szText,
                              m_layout.nextButton.left - m_layout.prevButton.right, 0, RT3_SORT_CENTER);

    RenderButton(m_abtn[BTN_PREV], false);
    RenderButton(m_abtn[BTN_NEXT], false);
    RenderButton(m_abtn[BTN_TAKE_ITEM], false);
    RenderButton(m_abtn[BTN_OFFER_ITEM], false);

    RenderHoveredItemInfo();
}

void SEASON3B::CNewUIBankWindow::RenderValuesPage()
{
    wchar_t szAmount[64] = {0};

    g_pRenderText->SetBgColor(0);

    for (int group = 0; group < 2; ++group)
    {
        const int headerTop = group == 0 ? m_layout.moneyHeaderTop : m_layout.jewelHeaderTop;

        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetTextColor(143, 184, 232, 255);
        g_pRenderText->RenderText(m_Pos.x + 20, m_Pos.y + headerTop,
                                  group == 0 ? I18N::Game::BankMoneyGroup : I18N::Game::BankJewelGroup, 240, 0);

        RenderColorQuadARGB(static_cast<float>(m_Pos.x + 16), static_cast<float>(m_Pos.y + headerTop + 16),
                            static_cast<float>(m_layout.width - 32), 1.f, HEADING_LINE_COLOR);
        EndRenderColor();

        const int first = group == 0 ? 0 : MONEY_CURRENCY_COUNT;
        const int last = group == 0 ? MONEY_CURRENCY_COUNT : static_cast<int>(Net::Bank::Currency::Count);

        g_pRenderText->SetFont(g_hFont);
        for (int currency = first; currency < last; ++currency)
        {
            const int top = GetCurrencyRowTop(currency);

            if (currency == m_selectedCurrency)
            {
                RenderColorQuadARGB(static_cast<float>(m_Pos.x + 16), static_cast<float>(top),
                                    static_cast<float>(m_layout.width - 32),
                                    static_cast<float>(m_layout.listLineHeight), PICKED_FILL_COLOR);
                RenderBorder(m_Pos.x + 16, top, m_layout.width - 32, m_layout.listLineHeight, PICKED_EDGE_COLOR, 1);
                EndRenderColor();
                g_pRenderText->SetTextColor(255, 233, 168, 255);
            }
            else
            {
                g_pRenderText->SetTextColor(220, 214, 200, 255);
            }

            g_pRenderText->RenderText(m_Pos.x + 26, top + 4,
                                      GetCurrencyName(static_cast<Net::Bank::Currency>(currency)), 180, 0);

            FormatAmount(Net::Bank::Store::Instance().GetBalance(static_cast<Net::Bank::Currency>(currency)), szAmount,
                         std::size(szAmount));
            g_pRenderText->RenderText(m_Pos.x + 206, top + 4, szAmount, m_layout.width - 232, 0, RT3_SORT_RIGHT);
        }
    }

    RenderButton(m_abtn[BTN_DEPOSIT], false);
    RenderButton(m_abtn[BTN_WITHDRAW], false);
    RenderButton(m_abtn[BTN_OFFER_VALUE], false);
}

void SEASON3B::CNewUIBankWindow::RenderMarketPage()
{
    wchar_t szText[256] = {0};
    wchar_t szAmount[64] = {0};

    const int sellerColumn = m_Pos.x + 26;
    const int nameColumn = m_Pos.x + 116;
    const int priceColumn = m_Pos.x + m_layout.width - 26;

    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetTextColor(143, 184, 232, 255);
    g_pRenderText->RenderText(sellerColumn, m_Pos.y + m_layout.marketHeaderTop, I18N::Game::BankSellerColumn, 90, 0);
    g_pRenderText->RenderText(nameColumn, m_Pos.y + m_layout.marketHeaderTop, I18N::Game::BankItemColumn, 140, 0);
    g_pRenderText->RenderText(priceColumn - 118, m_Pos.y + m_layout.marketHeaderTop, I18N::Game::BankPriceColumn, 118,
                              0, RT3_SORT_RIGHT);

    RenderColorQuadARGB(static_cast<float>(m_Pos.x + 16), static_cast<float>(m_Pos.y + m_layout.marketHeaderTop + 16),
                        static_cast<float>(m_layout.width - 32), 1.f, HEADING_LINE_COLOR);
    EndRenderColor();

    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    g_pRenderText->SetFont(g_hFont);

    if (offers.empty())
    {
        g_pRenderText->SetTextColor(180, 180, 180, 255);
        g_pRenderText->RenderText(m_Pos.x + 20, m_Pos.y + m_layout.marketRowsTop + 60, I18N::Game::BankHasNoOffers,
                                  m_layout.width - 40, 0, RT3_SORT_CENTER);
    }

    const int rows = std::min(static_cast<int>(offers.size()), m_layout.marketRows);
    for (int row = 0; row < rows; ++row)
    {
        const Net::Bank::Offer& offer = offers[row];
        const int top = GetOfferRowTop(row);

        if (row == m_selectedOffer)
        {
            RenderColorQuadARGB(static_cast<float>(m_Pos.x + 16), static_cast<float>(top),
                                static_cast<float>(m_layout.width - 32), static_cast<float>(m_layout.listLineHeight),
                                PICKED_FILL_COLOR);
            RenderBorder(m_Pos.x + 16, top, m_layout.width - 32, m_layout.listLineHeight, PICKED_EDGE_COLOR, 1);
            EndRenderColor();
            g_pRenderText->SetTextColor(255, 233, 168, 255);
        }
        else
        {
            g_pRenderText->SetTextColor(220, 214, 200, 255);
        }

        g_pRenderText->RenderText(sellerColumn, top + 4, offer.SellerName.c_str(), 90, 0);
        g_pRenderText->RenderText(nameColumn, top + 4, offer.OfferName.c_str(), 140, 0);

        FormatAmount(offer.PriceAmount, szAmount, std::size(szAmount));
        mu_swprintf(szText, L"%ls %ls", szAmount, GetCurrencyName(offer.PriceCurrency));
        g_pRenderText->RenderText(priceColumn - 118, top + 4, szText, 118, 0, RT3_SORT_RIGHT);
    }

    g_pRenderText->SetTextColor(200, 194, 180, 255);
    mu_swprintf(szText, I18N::Game::BankPageOf, Net::Bank::Store::Instance().GetOfferPage() + 1,
                Net::Bank::Store::Instance().GetOfferPageCount());
    g_pRenderText->RenderText(m_Pos.x + m_layout.prevButton.right, m_Pos.y + m_layout.pageRowTop + 3, szText,
                              m_layout.nextButton.left - m_layout.prevButton.right, 0, RT3_SORT_CENTER);

    RenderButton(m_abtn[BTN_PREV], false);
    RenderButton(m_abtn[BTN_NEXT], false);
    RenderButton(m_abtn[BTN_BUY], false);
    RenderButton(m_abtn[BTN_CANCEL_OFFER], false);
    RenderButton(m_abtn[BTN_MINE], m_ownOffersOnly);
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
    pWindow->GetTileRect(slot - pWindow->m_itemPage * pWindow->m_layout.itemsPerPage, rect);
    RenderItemInfo(rect.left + pWindow->m_layout.tileSize / 2, rect.top, pWindow->m_boxes[slot], false);
}

void SEASON3B::CNewUIBankWindow::Render3D()
{
    if (!IsVisible() || m_page != Page::Items)
    {
        return;
    }

    for (int slotOnPage = 0; slotOnPage < m_layout.itemsPerPage; ++slotOnPage)
    {
        const int slot = m_itemPage * m_layout.itemsPerPage + slotOnPage;
        if (slot >= BANK_TOTAL_SLOTS)
        {
            break;
        }

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
        const float scale = static_cast<float>(m_layout.tileSize - 10) / static_cast<float>(std::max(columns, rows));
        const float width = columns * scale;
        const float height = rows * scale;
        const float x = rect.left + (m_layout.tileSize - width) / 2.f;
        const float y = rect.top + (m_layout.tileSize - height) / 2.f;

        RenderItem3D(x, y, width, height, pItem->Type, pItem->Level, pItem->ExcellentFlags, pItem->AncientDiscriminator,
                     false);
    }
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
