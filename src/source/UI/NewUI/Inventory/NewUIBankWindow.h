//*****************************************************************************
// File: NewUIBankWindow.h
//*****************************************************************************

#pragma once

#include "Network/Server/BankProtocol.h"
#include "UI/NewUI/Inventory/BankCurrencyInfo.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/Market/SetCatalog.h"
#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/NewUI3DRenderMng.h"
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/Widgets/NewUIButton.h"
#include "UI/NewUI/Widgets/NewUIComboBox.h"
#include "UI/Scaling/UITransform.h"

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

    float GetLayerDepth() override; //. 5.9f

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

    /// <summary>
    /// Opens the window on the tab of the market, which is what the box of the menu asks for.
    /// </summary>
    void ShowMarketPage();

    /// <summary>Tells whether the tab of the market is the one being shown.</summary>
    bool IsMarketPage() const;

    /// <summary>Asks the server for a page of the market.</summary>
    void RequestMarketPage(BYTE page);

    /// <summary>Asks the server for a page of what the bank booked.</summary>
    void RequestLedgerPage(BYTE page);

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

    /// <summary>Gets what the dialog which asks for an amount is about.</summary>
    BankUI::AmountRequest GetAmountInputContext() const;

private:
    /// <summary>Which of the three tabs the window is showing.</summary>
    enum class Page
    {
        Items,
        Values,
        Market,
        Ledger,
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
        RENDER_OFFER_TOOLTIP = 2,
    };

    /// <summary>The dropdowns which narrow what the market lists.</summary>
    enum MARKET_FILTER
    {
        FILTER_KIND = 0,
        FILTER_CLASS,
        FILTER_SET,
        FILTER_CURRENCY,
        MAX_FILTER,
    };

    /// <summary>
    /// One entry of the dropdown which narrows the market to a set of armour, or to a kind of item
    /// which is not worn as a set.
    /// </summary>
    struct MarketSetFilter
    {
        std::wstring Name;

        /// <summary>What the offered item has to be; <c>AnyFilter</c> when it may be anything.</summary>
        BYTE Category;

        /// <summary>The set it has to belong to; <c>AnyFilter</c> when any set will do.</summary>
        BYTE SetNumber;

        /// <summary>
        /// Which families of classes may wear it, as one bit per family; zero when the entry says
        /// nothing about a class and is therefore offered to every one of them.
        /// </summary>
        int ClassMask;
    };

    enum BANK_BUTTON
    {
        BTN_TAB_ITEMS = 0,
        BTN_TAB_VALUES,
        BTN_TAB_MARKET,
        BTN_TAB_LEDGER,
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
        BTN_LEDGER_REFRESH,
        BTN_PRICE_PREV,
        BTN_PRICE_NEXT,
        MAX_BTN
    };

    /// <summary>The size of the window. Everything else follows from it.</summary>
    static constexpr int BANK_WIDTH = 440;

    /// <summary>
    /// How tall the window is, which is the whole height the dock it stands in has.
    /// </summary>
    /// <remarks>
    /// The dock ends at <see cref="UI::Scaling::DockLogicalBottom"/> - under it stands the hud
    /// frame, and under that nothing at all. A taller window does not scroll and is not moved up:
    /// its bottom is simply drawn past the edge of the screen, which is where the row of buttons
    /// stands. The height is therefore taken from the dock rather than typed, so the window cannot
    /// outgrow the room it is given again.
    /// </remarks>
    static constexpr int BANK_HEIGHT = UI::Scaling::DockLogicalBottom;

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

        RECT tab[4];

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

        /// <summary>
        /// How tall one row of the tab of the values is.
        /// </summary>
        /// <remarks>
        /// Taller than a row of the ledger, because a row here carries the picture of what it
        /// counts and a picture of eighteen pixels is a smudge. The room comes from the tab of the
        /// values reaching down to the row which says what a price is named in: the rows which
        /// stop the other tabs short - the number of the page and what is picked - belong to the
        /// tab of the items and say nothing here.
        /// </remarks>
        int valueLineHeight;

        /// <summary>How far down the list of the values may reach.</summary>
        int valueListBottom;

        /// <summary>How large the picture beside a currency is drawn.</summary>
        int valueIconSize;

        /// <summary>How far under the top of its row the text of a currency sits.</summary>
        int valueTextTop;

        int moneyHeaderTop;
        int moneyRowsTop;
        int jewelHeaderTop;
        int jewelRowsTop;
        /// <summary>
        /// The boxes of the market. They are wider and taller than the boxes of the storage,
        /// because under the picture of an offer stands what it costs.
        /// </summary>
        int marketFilterRowTop;
        RECT marketFilter[3];
        RECT marketCurrencyFilter;
        int marketTileWidth;
        int marketTileHeight;
        int marketPictureHeight;
        int marketTileColumns;
        int marketTileRows;
        int marketTileOriginX;
        int marketTileOriginY;
        int marketTilesPerPage;

        int ledgerHeaderTop;
        int ledgerRowsTop;
        int ledgerRows;

        /// <summary>
        /// Where the four columns of a ledger row start, and where the amount ends. The amount is
        /// written towards its right edge, because a column of numbers is read by its last digit.
        /// </summary>
        int ledgerTimeX;
        int ledgerTypeX;
        int ledgerDetailX;
        int ledgerDetailWidth;
        int ledgerAmountRight;
        int ledgerAmountWidth;
    };

    void BuildLayout();
    void ApplyLayoutToButtons();
    void ApplyLayoutToFilters();

    /// <summary>Fills the dropdowns which narrow the market, once, when the window is made.</summary>
    void BuildMarketFilters();

    /// <summary>Lists the sets the chosen class may wear, and none of the other classes'.</summary>
    void RefreshSetFilter();

    /// <summary>Reads the dropdowns of the market.</summary>
    /// <returns><c>true</c> when the cursor belongs to a dropdown and nothing else may have it.</returns>
    bool ProcessMarketFilters();

    /// <summary>Draws the dropdowns of the market, over everything they may cover.</summary>
    void RenderMarketFilters();

    /// <summary>Shuts every dropdown, which is what leaving the tab of the market means.</summary>
    void CloseMarketFilters();

    /// <summary>Gets whether a dropdown is open over the boxes of the market.</summary>
    bool IsAnyMarketFilterOpen() const;

    /// <summary>Gets which set or kind of item the dropdown of the sets is on.</summary>
    const MarketSetFilter& GetSelectedSetFilter() const;

    void InitButton(CNewUIButton* pButton, const wchar_t* const* captionSlot);
    void ShowRefusedRequest(Net::Bank::ResultCode result);
    void OpenPendingInput();

    void RenderFrame();
    void RenderTabs();
    void RenderItemsPage();
    void RenderValuesPage();
    void RenderMarketPage();
    void RenderLedgerPage();
    void RenderHoveredItemInfo();

    /// <summary>Draws the box every offer of the listed page stands in.</summary>
    void RenderOfferTilePlates();

    /// <summary>Writes under every offer of the listed page what it costs.</summary>
    void RenderOfferTilePrices();

    /// <summary>Writes what the picked offer is, who sells it and what it costs exactly.</summary>
    void RenderSelectedOfferDetail();

    /// <summary>Draws the tooltip of the offer the cursor is over.</summary>
    void RenderHoveredOfferInfo();

    /// <summary>Draws the pictures of the items which are in the boxes of the bank.</summary>
    void RenderStorageItems3D();

    /// <summary>
    /// Draws the picture of the jewel beside every row of jewels on the tab of the values.
    /// </summary>
    /// <remarks>
    /// A jewel is an item, and the picture of an item is drawn in the 3d pass like every other
    /// one. The coins of the currencies which are plain numbers are pictures of their own and are
    /// drawn with the rest of the page.
    /// </remarks>
    void RenderValueIcons3D();

    /// <summary>Draws the pictures of the items which are offered on the listed page.</summary>
    void RenderOfferItems3D();

    /// <summary>Draws the plate a box stands on: the fill inside its line, and the line.</summary>
    static void RenderTilePlate(const RECT& rect, unsigned int fill);

    /// <summary>Draws the coin which stands beside a currency that is a plain number.</summary>
    /// <remarks>
    /// Out of quads, like the rest of this window, because the client has no picture of a coin -
    /// see <see cref="BankUI::GetCoinColors"/>.
    /// </remarks>
    static void RenderCurrencyCoin(const RECT& rect, Net::Bank::Currency currency);

    /// <summary>Draws the picture of an item as large as it fits into the box it stands in.</summary>
    /// <param name="margin">
    /// How much of the box stays bare around the picture. A box of a storage leaves room for the
    /// edge it is drawn with; the picture beside the name of a currency has no edge and fills its
    /// whole square, so the margin is given by the caller rather than fixed here.
    /// </param>
    static void RenderItemPicture(const ITEM& item, int left, int top, int width, int height, int margin);

    /// <summary>Draws the row which says, and lets the player change, what a price is named in.</summary>
    void RenderPriceCurrencyRow();

    /// <summary>Draws the plate of a button and then lets the button draw its own caption.</summary>
    void RenderButton(CNewUIButton& button, bool highlighted);

    bool ProcessTabs();
    bool ProcessButtons();
    bool ProcessItemsPageButtons();
    bool ProcessValuesPageButtons();
    bool ProcessMarketPageButtons();
    bool ProcessLedgerPageButtons();
    bool ProcessPriceCurrencyButtons();
    bool ProcessTileSelection();
    bool ProcessValueSelection();
    bool ProcessOfferSelection();
    bool ProcessLedgerSelection();

    /// <summary>Gets the box the cursor is over, or -1 when it is over none.</summary>
    int GetTileAtCursor() const;

    /// <summary>Gets where a box of the shown page is drawn, in screen coordinates.</summary>
    void GetTileRect(int slotOnPage, RECT& rect) const;

    /// <summary>Gets where the row of a currency is drawn, which is also where it is clicked.</summary>
    int GetCurrencyRowTop(int currency) const;

    /// <summary>Gets where the picture of a currency is drawn, in screen coordinates.</summary>
    void GetCurrencyIconRect(int currency, RECT& rect) const;

    /// <summary>Gets where a box of the market is drawn, which is also where it is clicked.</summary>
    void GetOfferTileRect(int tile, RECT& rect) const;

    /// <summary>Gets the box of the market the cursor is over, or -1 when it is over none.</summary>
    int GetOfferTileAtCursor() const;

    /// <summary>Gets how many boxes of the market the listed page fills.</summary>
    int GetShownOfferCount() const;

    /// <summary>Gets where a row of the ledger is drawn, which is also where it is clicked.</summary>
    int GetLedgerRowTop(int row) const;

    /// <summary>Tells the player to pick a box first, when none is picked.</summary>
    bool HasSelectedItem();

    /// <summary>Sends the item of the picked box back into the inventory.</summary>
    void TakeSelectedItem();

    /// <summary>Forgets the item of a box and gives it back to the item manager.</summary>
    void ClearBox(int slot);

    void BuySelectedOffer();
    void CancelSelectedOffer();

    /// <summary>Builds an item for every offer of the listed page, so its picture can be drawn.</summary>
    void RebuildOfferItems();

    /// <summary>Gives the items of the listed offers back to the item manager.</summary>
    void ClearOfferItems();

    /// <summary>Gets whether an offer was made by the character which is playing.</summary>
    static bool IsOwnOffer(const Net::Bank::Offer& offer);

    /// <summary>Gets the name of a currency as it is listed.</summary>
    static const wchar_t* GetCurrencyName(Net::Bank::Currency currency);

    /// <summary>
    /// Gets the short name of a currency, which is what fits beside an amount in a ledger row.
    /// </summary>
    static const wchar_t* GetCurrencyShortName(Net::Bank::Currency currency);

    /// <summary>Gets why a ledger entry was booked, in words.</summary>
    static const wchar_t* GetLedgerTypeName(Net::Bank::LedgerEntryType type);

    /// <summary>
    /// Writes when an entry was booked, in the time of this machine.
    /// </summary>
    /// <remarks>
    /// The server sends seconds since the unix epoch in UTC; a player reads the clock on his wall,
    /// so it is turned into local time here and nowhere else.
    /// </remarks>
    static void FormatLedgerTime(int64_t timestamp, wchar_t* text, size_t textLength);

    /// <summary>
    /// Gets what a ledger row says under the columns: the description when there is one, else who
    /// the other side was, else nothing at all.
    /// </summary>
    static const wchar_t* GetLedgerDetail(const Net::Bank::LedgerEntry& entry);

    /// <summary>Draws what has to be drawn above the boxes, which is the tooltip of an item.</summary>
    static void UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB);

    CNewUIManager* m_pNewUIMng;
    CNewUI3DRenderMng* m_pNewUI3DRenderMng;
    POINT m_Pos;
    Layout m_layout;

    CNewUIButton m_abtn[MAX_BTN];

    CNewUIComboBox m_marketFilter[MAX_FILTER];

    /// <summary>
    /// What the dropdown of the sets can be set to: everything, then every set of armour the item
    /// list of the server holds, then the kinds of item which are not worn as a set.
    /// </summary>
    std::vector<MarketSetFilter> m_setFilters;

    /// <summary>Which of them the dropdown currently lists, by their place in it.</summary>
    std::vector<int> m_shownSetFilters;

    /// <summary>
    /// The captions the dropdowns read. A dropdown does not own what it is given, so these outlive
    /// it here rather than in the call which set it up.
    /// </summary>
    std::vector<const wchar_t*> m_setFilterLabels;
    std::vector<const wchar_t*> m_currencyFilterLabels;
    std::array<const wchar_t*, 3> m_kindFilterLabels{};
    std::array<const wchar_t*, 8> m_classFilterLabels{};

    Page m_page;
    int m_itemPage;
    int m_selectedCurrency;

    /// <summary>
    /// The currency a price is named in. It is the player's own choice and has nothing to do with
    /// the row he picked on the tab of the values, which is what he moves in and out of the bank.
    /// </summary>
    int m_priceCurrency;

    int m_selectedOffer;

    /// <summary>The ledger row the player picked; -1 when none is. Its detail is written out below.</summary>
    int m_selectedLedgerRow;

    /// <summary>
    /// The ledger page which was last asked for; -1 when none has been.
    /// </summary>
    /// <remarks>
    /// The store keeps the page which arrived, not the one on its way, so without this the window
    /// would read the old page for as many frames as the answer takes and act on it again in every
    /// one of them - which turned one correction into a request per frame.
    /// </remarks>
    int m_ledgerRequestedPage;

    /// <summary>
    /// The last page of the ledger, once the window has found it; -1 while it has not.
    /// </summary>
    /// <remarks>
    /// The server sends no count of pages and no page size, so the end cannot be worked out from a
    /// page which arrived - a bank with three entries and a bank whose pages hold three look alike.
    /// The end is therefore found by turning past it once: a page which comes back empty says the
    /// one before it was the last, and the window goes straight back to it. Asking for page 0
    /// forgets this again, because a bank which booked something since has an end further on.
    /// </remarks>
    int m_ledgerLastPage;

    /// <summary>What the server sent for every box, by box number.</summary>
    std::array<ITEM*, BANK_TOTAL_SLOTS> m_boxes{};

    /// <summary>The box the player clicked; -1 when none is picked. What the buttons act on.</summary>
    int m_selectedSlot;

    /// <summary>The box of the inventory an item is leaving, while that move is on its way.</summary>
    int m_depositSourceSlot;

    /// <summary>The box of the bank an item is leaving, while that move is on its way.</summary>
    int m_takeSourceSlot;

    /// <summary>
    /// An item for every offer of the listed page, in the order the offers arrived; null where an
    /// offer carries a currency rather than an item.
    /// </summary>
    /// <remarks>
    /// An offer carries its item in the shape it has in the inventory, and a picture is drawn from
    /// an <c>ITEM</c> like every other one. They are built once per page and not per frame, which
    /// is what <see cref="Net::Bank::Store::GetOfferGeneration"/> is compared for.
    /// </remarks>
    std::vector<ITEM*> m_offerItems;

    /// <summary>Which page of the market the items were built for.</summary>
    unsigned int m_offerGeneration;

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
