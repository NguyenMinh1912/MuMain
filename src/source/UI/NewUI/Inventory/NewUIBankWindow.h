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
///
/// The window draws its own panel, boxes and buttons out of plain quads instead of borrowing the
/// textures of the vault dialog. Those textures are cut for a window of 190 pixels: stretched over
/// a wider one they leave half of it bare, which is exactly what they did here.
/// </remarks>
class CNewUIBankWindow : public CNewUIObj, public INewUI3DRenderObj
{
public:
    /// <summary>How many boxes the bank has, which has to match what the server is configured with.</summary>
    static constexpr int BANK_TOTAL_SLOTS = 100;

    /// <summary>
    /// Gets how wide the window is, so that whoever places it can put it beside the inventory
    /// instead of on top of it.
    /// </summary>
    static constexpr int GetWindowWidth();

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

    /// <summary>
    /// Gets the name of the currency a price is named in, which is the one selected on the tab of
    /// the values. The dialog which asks for a price says it, so the player is not left guessing.
    /// </summary>
    const wchar_t* GetPriceCurrencyName() const;

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
        BTN_PRICE_PREV,
        BTN_PRICE_NEXT,
        MAX_BTN
    };

    /// <summary>The size of the window. Everything else follows from it.</summary>
    static constexpr int BANK_WIDTH = 380;
    static constexpr int BANK_HEIGHT = 448;

    /// <summary>
    /// Where every part of the window stands, in window coordinates.
    /// </summary>
    /// <remarks>
    /// Nothing here is a number typed twice. <see cref="BuildLayout"/> works it out from the size
    /// of the window, so a window of another size lays itself out - including how many boxes and
    /// how many rows of a list fit - instead of needing every coordinate corrected by hand. It is
    /// also the one place drawing and hit testing both read, so the two cannot drift apart.
    /// </remarks>
    struct Layout
    {
        int width;
        int height;

        RECT tab[3];

        int contentTop;
        int contentBottom;

        int tileSize;
        int tileColumns;
        int tileRows;
        int tileOriginX;
        int tileOriginY;
        int itemsPerPage;
        int itemPageCount;

        int infoRowTop;
        int pageRowTop;
        int priceRowTop;
        RECT prevButton;
        RECT nextButton;
        RECT pricePrevButton;
        RECT priceNextButton;
        RECT button[3];

        int listLineHeight;
        int moneyHeaderTop;
        int moneyRowsTop;
        int jewelHeaderTop;
        int jewelRowsTop;
        int marketHeaderTop;
        int marketRowsTop;
        int marketRows;
    };

    void BuildLayout();
    void ApplyLayoutToButtons();

    void InitButton(CNewUIButton* pButton, const wchar_t* const* captionSlot);
    void ShowRefusedRequest(Net::Bank::ResultCode result);
    void OpenPendingInput();

    void RenderFrame();
    void RenderTabs();
    void RenderItemsPage();
    void RenderValuesPage();
    void RenderMarketPage();
    void RenderHoveredItemInfo();

    /// <summary>Draws the row which says, and lets the player change, what a price is named in.</summary>
    void RenderPriceCurrencyRow();

    /// <summary>Draws the plate of a button and then lets the button draw its own caption.</summary>
    void RenderButton(CNewUIButton& button, bool highlighted);

    bool ProcessTabs();
    bool ProcessButtons();
    bool ProcessItemsPageButtons();
    bool ProcessValuesPageButtons();
    bool ProcessMarketPageButtons();
    bool ProcessPriceCurrencyButtons();
    bool ProcessTileSelection();
    bool ProcessValueSelection();
    bool ProcessOfferSelection();

    /// <summary>Gets the box the cursor is over, or -1 when it is over none.</summary>
    int GetTileAtCursor() const;

    /// <summary>Gets where a box of the shown page is drawn, in screen coordinates.</summary>
    void GetTileRect(int slotOnPage, RECT& rect) const;

    /// <summary>Gets where the row of a currency is drawn, which is also where it is clicked.</summary>
    int GetCurrencyRowTop(int currency) const;

    /// <summary>Gets where a row of the market is drawn, which is also where it is clicked.</summary>
    int GetOfferRowTop(int row) const;

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

    /// <summary>Draws what has to be drawn above the boxes, which is the tooltip of an item.</summary>
    static void UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB);

    CNewUIManager* m_pNewUIMng;
    CNewUI3DRenderMng* m_pNewUI3DRenderMng;
    POINT m_Pos;
    Layout m_layout;

    CNewUIButton m_abtn[MAX_BTN];

    Page m_page;
    int m_itemPage;
    int m_selectedCurrency;

    /// <summary>
    /// The currency a price is named in. It is the player's own choice and has nothing to do with
    /// the row he picked on the tab of the values, which is what he moves in and out of the bank.
    /// </summary>
    int m_priceCurrency;

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

    /// <summary>
    /// The offer a request was sent about, while no answer has arrived. Clicking five times in a
    /// second used to send five requests for the same offer, and the server had to sort out what
    /// that meant; now the four which follow are not sent at all.
    /// </summary>
    std::array<BYTE, Net::Bank::ListingIdLength> m_pendingListingId{};
    bool m_waitingForMarketAnswer;

    PendingInput m_pendingInput;

    /// <summary>Whether the offer being made carries the picked item or an amount of a currency.</summary>
    bool m_offerCarriesItem;

    /// <summary>How much of a currency is being offered, while its price is still being asked for.</summary>
    int64_t m_offerAmount;
};

inline constexpr int CNewUIBankWindow::GetWindowWidth()
{
    return BANK_WIDTH;
}
} // namespace SEASON3B
