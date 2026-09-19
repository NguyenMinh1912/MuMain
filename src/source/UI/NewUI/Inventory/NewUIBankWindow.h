//*****************************************************************************
// File: NewUIBankWindow.h
//*****************************************************************************

#pragma once

#include "Network/Server/BankProtocol.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

#include <array>
#include <span>
#include <string>
#include <vector>

namespace SEASON3B
{
/// <summary>
/// The bank of the account: the items it keeps, what it holds of every currency, and the market on
/// which the offers of all players can be bought.
/// </summary>
/// <remarks>
/// Not part of the original client, and not an npc dialog: the player opens it from a button of his
/// inventory. Nothing here is dragged. A right click on an item of the inventory puts it in, a
/// click on a box picks that box, and the buttons act on what is picked. The boxes are drawn as
/// equally sized tiles rather than as the grid of the inventory, because a box of the bank holds
/// one item whatever its size is - which is also how the server counts them.
/// </remarks>
class CNewUIBankWindow : public CNewUIObj, public INewUI3DRenderObj
{
public:
    enum IMAGE_LIST
    {
        // The window borrows the frame of the vault instead of shipping its own, so it needs no
        // new texture files.
        IMAGE_BANK_BACK = CNewUIMessageBoxMng::IMAGE_MSGBOX_BACK,
        IMAGE_BANK_TOP = CNewUIMyInventory::IMAGE_INVENTORY_BACK_TOP,
        IMAGE_BANK_LEFT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_LEFT,
        IMAGE_BANK_RIGHT = CNewUIMyInventory::IMAGE_INVENTORY_BACK_RIGHT,
        IMAGE_BANK_BOTTOM = CNewUIMyInventory::IMAGE_INVENTORY_BACK_BOTTOM,
        IMAGE_BANK_BUTTON = CNewUIMessageBoxMng::IMAGE_MSGBOX_BTN_EMPTY_VERY_SMALL,
    };

    /// <summary>How many boxes the bank has, which has to match what the server is configured with.</summary>
    static constexpr int BANK_TOTAL_SLOTS = 100;

    CNewUIBankWindow();
    ~CNewUIBankWindow() override;

    bool Create(CNewUIManager* pNewUIMng, CNewUI3DRenderMng* pNewUI3DRenderMng, int x, int y);
    void Release();

    void SetPos(int x, int y);

    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;

    /// <summary>Draws the picture of every item which is on the shown page.</summary>
    void Render3D() override;

    /// <inheritdoc/>
    bool IsVisible() const override;

    float GetLayerDepth() override; //. 2.2f

    /// <summary>Tells the server that the dialog was opened, and asks for what it holds.</summary>
    void OpeningProcess();

    /// <summary>Tells the server that the dialog was closed.</summary>
    void ClosingProcess();

    /// <summary>Keeps an item of the bank, so that its box can draw it.</summary>
    bool InsertItem(int iIndex, std::span<const BYTE> pbyItemPacket);

    /// <summary>Puts an item into its box after the server moved it there.</summary>
    void ProcessToReceiveBankItems(int nIndex, std::span<const BYTE> pbyItemPacket);

    /// <summary>Forgets every item, for a list which is about to arrive.</summary>
    void DeleteAllItems();

    /// <summary>
    /// Empties the box an item left after the server confirmed the move which took it out.
    /// </summary>
    void ProcessAutoMoveSuccess();

    /// <summary>
    /// Moves the item under the cursor from the inventory into the bank, which is what a right
    /// click on it means while the bank is open.
    /// </summary>
    /// <param name="sourceCtrl">The control the item was clicked in; the inventory when null.</param>
    /// <returns><c>true</c> when a move was sent.</returns>
    bool ProcessMyInvenItemAutoMove(CNewUIInventoryCtrl* sourceCtrl = nullptr);

    /// <summary>Asks the server for a page of the market.</summary>
    void RequestMarketPage(BYTE page);

    /// <summary>Offers what is picked at the price the player typed.</summary>
    /// <param name="price">The price, in the currency which is selected.</param>
    void FinishOffer(int64_t price);

    /// <summary>Moves the amount the player typed into the bank or out of it.</summary>
    /// <param name="amount">The amount to move.</param>
    void FinishValueAmount(int64_t amount);

    /// <summary>Forgets a half finished dialog, because the player cancelled it.</summary>
    void CancelPendingInput();

private:
    /// <summary>Which of the three tabs the window is showing.</summary>
    enum class Page
    {
        Items,
        Values,
        Market,
    };

    /// <summary>
    /// Which dialog the window wants to open on its next frame.
    /// </summary>
    /// <remarks>
    /// The dialogs are opened from <see cref="Update"/> and never from the callback of another
    /// dialog, so a message box is never created while the one which asked for it is being
    /// destroyed.
    /// </remarks>
    enum class PendingInput
    {
        None,
        Price,
        DepositAmount,
        WithdrawAmount,
        OfferAmount,
    };

    /// <summary>What a 2d effect of this window draws.</summary>
    enum RENDER_KIND
    {
        RENDER_ITEM_TOOLTIP = 1,
    };

    enum BANK_BUTTON
    {
        BTN_TAB_ITEMS = 0,
        BTN_TAB_VALUES,
        BTN_TAB_MARKET,
        BTN_PREV,
        BTN_NEXT,
        BTN_TAKE_ITEM,
        BTN_OFFER_ITEM,
        BTN_DEPOSIT,
        BTN_WITHDRAW,
        BTN_OFFER_VALUE,
        BTN_BUY,
        BTN_CANCEL_OFFER,
        BTN_MINE,
        MAX_BTN
    };

    static constexpr float BANK_WIDTH = 260.0f;
    static constexpr float BANK_HEIGHT = 380.0f;

    /// <summary>The tiles of one page of the item boxes.</summary>
    static constexpr int TILE_SIZE = 48;
    static constexpr int TILE_COLUMNS = 5;
    static constexpr int TILE_ROWS = 4;
    static constexpr int ITEMS_PER_PAGE = TILE_COLUMNS * TILE_ROWS;
    static constexpr int ITEM_PAGE_COUNT = BANK_TOTAL_SLOTS / ITEMS_PER_PAGE;

    /// <summary>Where the rows of the window stand, relative to its own corner.</summary>
    static constexpr int TAB_ROW_TOP = 28;
    static constexpr int TAB_WIDTH = 82;
    static constexpr int TAB_HEIGHT = 24;
    static constexpr int CONTENT_TOP = 56;
    static constexpr int TILE_ORIGIN_X = 10;
    static constexpr int INFO_ROW_TOP = 252;
    static constexpr int PAGE_ROW_TOP = 272;
    static constexpr int BUTTON_ROW_TOP = 298;
    static constexpr int BUTTON_WIDTH = 74;
    static constexpr int BUTTON_HEIGHT = 26;

    /// <summary>How many rows of the market and how tall they are.</summary>
    static constexpr int MARKET_ROWS = 11;
    static constexpr int LIST_LINE_HEIGHT = 16;

    void InitButton(CNewUIButton* pButton, const wchar_t* const* captionSlot);
    void LayoutButtons();
    void ShowRefusedRequest();
    void OpenPendingInput();

    void RenderFrame();
    void RenderTabs();
    void RenderItemsPage();
    void RenderValuesPage();
    void RenderMarketPage();
    void RenderHoveredItemInfo();

    bool ProcessTabs();
    bool ProcessButtons();
    bool ProcessItemsPageButtons();
    bool ProcessValuesPageButtons();
    bool ProcessMarketPageButtons();
    bool ProcessTileSelection();
    bool ProcessValueSelection();
    bool ProcessOfferSelection();

    /// <summary>Gets the box the cursor is over, or -1 when it is over none.</summary>
    int GetTileAtCursor() const;

    /// <summary>Gets where a box of the shown page is drawn.</summary>
    void GetTileRect(int slotOnPage, RECT& rect) const;

    /// <summary>Tells the player to pick a box first, when none is picked.</summary>
    bool HasSelectedItem();

    /// <summary>Sends the item of the picked box back into the inventory.</summary>
    void TakeSelectedItem();

    /// <summary>Forgets the item of a box and gives it back to the item manager.</summary>
    void ClearBox(int slot);

    void BuySelectedOffer();
    void CancelSelectedOffer();

    /// <summary>Gets the name of a currency as it is listed.</summary>
    static const wchar_t* GetCurrencyName(Net::Bank::Currency currency);

    /// <summary>Gets how many of the currencies are money rather than jewels.</summary>
    static int GetMoneyCurrencyCount();

    /// <summary>Draws what has to be drawn above the boxes, which is the tooltip of an item.</summary>
    static void UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB);

    CNewUIManager* m_pNewUIMng;
    CNewUI3DRenderMng* m_pNewUI3DRenderMng;
    POINT m_Pos;

    CNewUIButton m_abtn[MAX_BTN];

    Page m_page;
    int m_itemPage;
    int m_selectedCurrency;
    int m_selectedOffer;

    /// <summary>What the server sent for every box, by box number.</summary>
    std::array<ITEM*, BANK_TOTAL_SLOTS> m_boxes{};

    /// <summary>The box the player clicked; -1 when none is picked. What the buttons act on.</summary>
    int m_selectedSlot;

    /// <summary>The box of the inventory an item is leaving, while that move is on its way.</summary>
    int m_depositSourceSlot;

    /// <summary>The box of the bank an item is leaving, while that move is on its way.</summary>
    int m_takeSourceSlot;

    /// <summary>Whether only the offers of this account are listed on the market tab.</summary>
    bool m_ownOffersOnly;

    PendingInput m_pendingInput;

    /// <summary>Whether the offer being made carries the picked item or an amount of a currency.</summary>
    bool m_offerCarriesItem;

    /// <summary>How much of a currency is being offered, while its price is still being asked for.</summary>
    int64_t m_offerAmount;
};
} // namespace SEASON3B
