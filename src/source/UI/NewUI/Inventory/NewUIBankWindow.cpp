//*****************************************************************************
// File: NewUIBankWindow.cpp
//*****************************************************************************

#include "stdafx.h"

#include "UI/NewUI/Inventory/NewUIBankWindow.h"

#include "Audio/DSPlaySound.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInventory.h"
#include "I18N/All.h"
#include "Network/Server/BankStore.h"
#include "Network/Server/ServerListManager.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Chat/Chat.h"
#include "UI/NewUI/Dialogs/NewUICustomMessageBox.h"
#include "UI/NewUI/Inventory/BankCurrencyInfo.h"
#include "UI/NewUI/Inventory/NewUIItemMng.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Widgets/UIPanelStyle.h"

#include <algorithm>
#include <ctime>
#include <utility>

namespace
{
using namespace UI::PanelStyle;

/// <summary>A column of a ledger row which has nothing in it.</summary>
constexpr TextColor NOTHING_BOOKED_TEXT{170, 165, 155, 255};

/// <summary>How many boxes of the bank are used.</summary>
constexpr TextColor BOX_COUNT_TEXT{159, 167, 155, 255};

/// <summary>An amount the bank gained, and one it lost.</summary>
constexpr TextColor GAINED_TEXT{126, 206, 134, 255};
constexpr TextColor LOST_TEXT{226, 124, 112, 255};

/// <summary>How far the rows of a list stand from the edge of the window.</summary>
constexpr int ROW_INSET = 16;

/// <summary>Where a line of text which stands on its own starts.</summary>
constexpr int TEXT_INSET = 20;

/// <summary>How far under the top of its row a line of a list sits.</summary>
constexpr int LIST_ROW_TEXT_TOP = 4;

/// <summary>How tall one row of a list is.</summary>
constexpr int LIST_LINE_HEIGHT = 22;

/// <summary>How tall one line of a list is, which is what a row is padded around.</summary>
constexpr int LIST_TEXT_HEIGHT = LIST_LINE_HEIGHT - 2 * LIST_ROW_TEXT_TOP;

/// <summary>How wide the label at the left end of a row of its own is.</summary>
constexpr int LABEL_WIDTH = 110;

/// <summary>Where what is picked is written, beside the label of that row.</summary>
constexpr int PICKED_NAME_X = 130;

/// <summary>How far under a heading the line which underlines it is drawn.</summary>
constexpr int HEADING_LINE_TOP = 16;

/// <summary>How far a line which fills half a row stops short of the middle of it.</summary>
constexpr int HALF_ROW_MARGIN = 24;

/// <summary>How far down its page the line stands which says that the page is empty.</summary>
constexpr int EMPTY_LIST_TOP = 60;

/// <summary>How far under the top of its row the text between two buttons sits.</summary>
constexpr int BUTTON_ROW_TEXT_TOP = 3;

/// <summary>How much of a box of the market stays bare left and right of the text in it.</summary>
constexpr int TILE_TEXT_INSET = 2;

/// <summary>How many rows a dropdown of the market shows before it starts to scroll.</summary>
constexpr int MAX_FILTER_ROWS = 10;

/// <summary>Where the picture of a currency is drawn, and how large it is.</summary>
/// <remarks>
/// The picture stands inside the row rather than beside it, so a row which is picked draws its
/// plate around the picture as well - the name and the picture are one thing to click on.
/// </remarks>
constexpr int CURRENCY_ICON_X = 20;

/// <summary>How much of a row stays bare above and below the picture in it.</summary>
constexpr int VALUE_ICON_MARGIN = 2;

/// <summary>
/// How tall a row of the values may grow, so a list with room to spare stops spreading out.
/// </summary>
constexpr int MAX_VALUE_LINE_HEIGHT = 28;

/// <summary>How tall the heading over a group of currencies is.</summary>
constexpr int VALUE_HEADING_HEIGHT = 20;

/// <summary>How far under the top of its coin the letter of a currency sits.</summary>
constexpr int COIN_GLYPH_TOP = 3;

/// <summary>Where the name of a currency and where its amount are written.</summary>
constexpr int CURRENCY_NAME_X = 42;
constexpr int CURRENCY_NAME_WIDTH = 164;
constexpr int CURRENCY_AMOUNT_X = 206;

/// <summary>
/// How far the columns of the ledger stand from the edge of the window.
/// </summary>
/// <remarks>
/// This used to be <see cref="CURRENCY_NAME_X"/>, which the two lists happened to share until the
/// names of the currencies moved right to make room for their pictures. The ledger has no pictures
/// and did not move, so it keeps the inset it always had instead of following that one.
/// </remarks>
constexpr int LEDGER_SIDE_INSET = 26;

/// <summary>How far the amount of a currency stops short of the edge of the window.</summary>
constexpr int CURRENCY_AMOUNT_END = 26;

/// <summary>The edge of a box which holds an offer the player made himself.</summary>
constexpr unsigned int OWN_OFFER_EDGE_COLOR = 0xFF6FA8DCu;

/// <summary>How far apart the two lines under the picture of an offer stand.</summary>
constexpr int PRICE_LINE_HEIGHT = 13;

/// <summary>How much of a box stays bare around the picture in it.</summary>
constexpr int PICTURE_MARGIN = 10;

/// <summary>One billion, which is the unit a price too long for its box is written in.</summary>
constexpr int64_t ONE_BILLION = 1000000000;

/// <summary>
/// The first price which no longer fits into a box in full: ten digits and the dots between them
/// are what the strip under a picture holds, and from here on the price is written in billions.
/// </summary>
constexpr int64_t SHORTENED_PRICE_THRESHOLD = 10 * ONE_BILLION;

/// <summary>What stands between the parts of the line which describes the picked offer.</summary>
constexpr const wchar_t* DETAIL_SEPARATOR = L"  \u00B7  ";

/// <summary>How many of the currencies are money; the rest are jewels.</summary>
constexpr int MONEY_CURRENCY_COUNT = 4;

/// <summary>
/// What a column of a ledger row says when the entry has nothing to put in it: an em dash, so an
/// empty cell reads as "nothing" rather than as a row which failed to draw.
/// </summary>
constexpr const wchar_t* NOTHING_BOOKED = L"\u2014";

/// <summary>Writes an amount with a separator every three digits, which is how balances are read.</summary>
void FormatAmount(int64_t amount, wchar_t* text, size_t textLength)
{
    // The magnitude is taken as an unsigned number on purpose: negating the smallest int64 does
    // not fit back into an int64, which is undefined behaviour rather than a large positive value.
    const bool negative = amount < 0;
    const unsigned long long magnitude =
        negative ? 0ULL - static_cast<unsigned long long>(amount) : static_cast<unsigned long long>(amount);

    wchar_t digits[32] = {0};
    mu_swprintf(digits, L"%llu", magnitude);

    const size_t digitCount = wcslen(digits);
    size_t written = 0;
    if (negative && written + 1 < textLength)
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

/// <summary>
/// Writes a price so that it fits into the strip under the picture of the offer it belongs to.
/// </summary>
void FormatPriceForTile(int64_t amount, wchar_t* text, size_t textLength)
{
    const int64_t magnitude = amount < 0 ? -amount : amount;
    if (magnitude < SHORTENED_PRICE_THRESHOLD)
    {
        FormatAmount(amount, text, textLength);
        return;
    }

    mu_swprintf_s(text, textLength, L"%lld%ls", static_cast<long long>(amount / ONE_BILLION),
                  I18N::Game::BankBillionsSuffix);
}
} // namespace

SEASON3B::CNewUIBankWindow::CNewUIBankWindow()
    : m_pNewUIMng(nullptr), m_pNewUI3DRenderMng(nullptr), m_layout{}, m_page(Page::Items), m_itemPage(0),
      m_selectedCurrency(0), m_priceCurrency(0), m_selectedOffer(-1), m_selectedLedgerRow(-1),
      m_ledgerRequestedPage(-1), m_ledgerLastPage(-1), m_offerGeneration(0), m_selectedSlot(-1),
      m_depositSourceSlot(-1), m_takeSourceSlot(-1), m_ownOffersOnly(false), m_waitingForMarketAnswer(false),
      m_pendingInput(PendingInput::None), m_offerCarriesItem(true), m_offerAmount(0)
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
    m_pNewUI3DRenderMng->Add3DRenderObj(this, BANK_CAMERA_Z_ORDER);

    InitButton(&m_abtn[BTN_TAB_ITEMS], &I18N::Game::BankStorage);
    InitButton(&m_abtn[BTN_TAB_VALUES], &I18N::Game::BankValues);
    InitButton(&m_abtn[BTN_TAB_MARKET], &I18N::Game::BankMarket);
    InitButton(&m_abtn[BTN_TAB_LEDGER], &I18N::Game::BankLedger);
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
    InitButton(&m_abtn[BTN_LEDGER_REFRESH], &I18N::Game::BankRefresh);
    InitButton(&m_abtn[BTN_PRICE_PREV], &I18N::Game::BankPreviousPage);
    InitButton(&m_abtn[BTN_PRICE_NEXT], &I18N::Game::BankNextPage);

    SetPos(x, y);
    BuildMarketFilters();

    Show(false);

    return true;
}

void SEASON3B::CNewUIBankWindow::Release()
{
    DeleteAllItems();
    ClearOfferItems();

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
    constexpr int buttonRowBottomGap = 16;
    constexpr int priceRowHeight = 24;
    constexpr int priceButtonWidth = 24;
    constexpr int priceButtonInset = 130;
    constexpr int pageRowHeight = 26;
    constexpr int infoRowHeight = 20;
    constexpr int contentTopGap = 8;
    constexpr int contentBottomGap = 4;
    constexpr int headingHeight = 24;
    constexpr int storageTileSize = 68;
    constexpr int gridSideGap = 8;

    constexpr int tabCount = 4;
    const int tabWidth = std::max(1, (layout.width - 2 * edge - (tabCount - 1) * gap) / tabCount);
    for (int index = 0; index < tabCount; ++index)
    {
        layout.tab[index].left = edge + index * (tabWidth + gap);
        layout.tab[index].top = titleHeight;
        layout.tab[index].right = layout.tab[index].left + tabWidth;
        layout.tab[index].bottom = titleHeight + tabHeight;
    }

    // The row of buttons stands at the bottom; everything else fills what is left above it.
    const int buttonRowTop = layout.height - buttonHeight - buttonRowBottomGap;
    const int buttonWidth = std::max(1, (layout.width - 2 * edge - 2 * gap) / 3);
    for (int index = 0; index < 3; ++index)
    {
        layout.button[index].left = edge + index * (buttonWidth + gap);
        layout.button[index].top = buttonRowTop;
        layout.button[index].right = layout.button[index].left + buttonWidth;
        layout.button[index].bottom = buttonRowTop + buttonHeight;
    }

    // The row which says what a price is named in stands right above the buttons which use it.
    layout.priceRowTop = buttonRowTop - priceRowHeight;
    layout.pricePrevButton = {priceButtonInset, layout.priceRowTop, priceButtonInset + priceButtonWidth,
                              layout.priceRowTop + pageButtonHeight};
    layout.priceNextButton = {layout.width - priceButtonInset - priceButtonWidth, layout.priceRowTop,
                              layout.width - priceButtonInset, layout.priceRowTop + pageButtonHeight};

    layout.pageRowTop = layout.priceRowTop - pageRowHeight;
    layout.prevButton = {edge, layout.pageRowTop, edge + pageButtonWidth, layout.pageRowTop + pageButtonHeight};
    layout.nextButton = {layout.width - edge - pageButtonWidth, layout.pageRowTop, layout.width - edge,
                         layout.pageRowTop + pageButtonHeight};
    layout.infoRowTop = layout.pageRowTop - infoRowHeight;

    layout.contentTop = titleHeight + tabHeight + contentTopGap;
    layout.contentBottom = layout.infoRowTop - contentBottomGap;

    // The boxes: as many as the room holds, and how many pages there are follows from that.
    layout.tileSize = storageTileSize;
    layout.tileColumns = std::max(1, (layout.width - 2 * edge - gridSideGap) / layout.tileSize);
    layout.tileRows = std::max(1, (layout.contentBottom - layout.contentTop) / layout.tileSize);
    layout.itemsPerPage = layout.tileColumns * layout.tileRows;
    layout.itemPageCount = (BANK_TOTAL_SLOTS + layout.itemsPerPage - 1) / layout.itemsPerPage;
    layout.tileOriginX = (layout.width - layout.tileColumns * layout.tileSize) / 2;
    layout.tileOriginY = layout.contentTop;

    // The two lists share a line height; each starts under its own heading. What is left over the
    // rows the currencies need sets the two groups apart, up to a gap which is enough to read them
    // as two: a window with less room puts them closer together rather than writing the last jewel
    // over the row under the lists.
    constexpr int maximumGroupGap = 14;
    constexpr int groupCount = 2;
    layout.listLineHeight = LIST_LINE_HEIGHT;

    // The tab of the values reaches further down than the others: the row which says how many
    // pages there are and the row which says what is picked belong to the tab of the items, and
    // the list of the values has nothing to say in them. The room they leave is what lets a row
    // carry a picture large enough to be read.
    layout.valueListBottom = layout.priceRowTop - contentBottomGap;

    const int valueRoom = layout.valueListBottom - layout.contentTop;
    const int listRows = static_cast<int>(Net::Bank::Currency::Count);
    layout.valueLineHeight =
        std::clamp((valueRoom - groupCount * VALUE_HEADING_HEIGHT - maximumGroupGap) / std::max(1, listRows),
                   LIST_LINE_HEIGHT, MAX_VALUE_LINE_HEIGHT);
    layout.valueIconSize = layout.valueLineHeight - 2 * VALUE_ICON_MARGIN;

    // A taller row does not leave its text at the top of it: the line sits in the middle, beside
    // the picture, whatever height the row ended up with.
    layout.valueTextTop = std::max(0, (layout.valueLineHeight - LIST_TEXT_HEIGHT) / 2);

    const int groupGap = std::clamp(valueRoom - groupCount * VALUE_HEADING_HEIGHT - listRows * layout.valueLineHeight,
                                    0, maximumGroupGap);

    layout.moneyHeaderTop = layout.contentTop;
    layout.moneyRowsTop = layout.moneyHeaderTop + VALUE_HEADING_HEIGHT;
    layout.jewelHeaderTop = layout.moneyRowsTop + MONEY_CURRENCY_COUNT * layout.valueLineHeight + groupGap;
    layout.jewelRowsTop = layout.jewelHeaderTop + VALUE_HEADING_HEIGHT;

    // The market shows its offers as boxes rather than as rows of text, because the picture of an
    // item says more than its name - and under every picture stands what it costs.
    constexpr int filterRowHeight = 24;
    constexpr int tileMinWidth = 80;
    constexpr int preferredTileHeight = 86;
    constexpr int priceStripHeight = 26;
    constexpr int wantedTileRows = 3;

    layout.marketFilterRowTop = layout.contentTop;
    layout.marketTileColumns = std::max(1, (layout.width - 2 * edge) / tileMinWidth);
    layout.marketTileWidth = (layout.width - 2 * edge) / layout.marketTileColumns;
    layout.marketTileOriginY = layout.marketFilterRowTop + filterRowHeight + gap;

    // The market wants the three rows a page of the server fills, and its boxes are as tall as the
    // room allows up to the size at which a picture is comfortable. A window with less room for
    // them therefore draws smaller boxes instead of dropping the offers which no longer fit in.
    const int marketRoom = std::max(1, layout.contentBottom - layout.marketTileOriginY);
    layout.marketTileHeight = std::clamp(marketRoom / wantedTileRows, priceStripHeight + 1, preferredTileHeight);
    layout.marketPictureHeight = layout.marketTileHeight - priceStripHeight;
    layout.marketTileRows = std::max(1, marketRoom / layout.marketTileHeight);
    layout.marketTilesPerPage = layout.marketTileColumns * layout.marketTileRows;
    layout.marketTileOriginX = (layout.width - layout.marketTileColumns * layout.marketTileWidth) / 2;

    // Three dropdowns fill the row over the boxes, and the fourth stands where the tab of the
    // items says what a price is named in - which the tab of the market has no use for.
    constexpr int filterCount = 3;
    const int filterWidth = std::max(1, (layout.width - 2 * edge - (filterCount - 1) * gap) / filterCount);
    for (int index = 0; index < filterCount; ++index)
    {
        layout.marketFilter[index].left = edge + index * (filterWidth + gap);
        layout.marketFilter[index].top = layout.marketFilterRowTop;
        layout.marketFilter[index].right = layout.marketFilter[index].left + filterWidth;
        layout.marketFilter[index].bottom = layout.marketFilterRowTop + filterRowHeight;
    }

    layout.marketCurrencyFilter = {edge, layout.priceRowTop, edge + filterWidth, layout.priceRowTop + filterRowHeight};

    layout.ledgerHeaderTop = layout.contentTop;
    layout.ledgerRowsTop = layout.ledgerHeaderTop + headingHeight;
    layout.ledgerRows = std::max(1, (layout.contentBottom - layout.ledgerRowsTop) / layout.listLineHeight);

    // The columns stand between the same edges the market list uses, so the two read as one window.
    // They are shares of that width and not four typed numbers, which is what lets a wider window
    // widen its columns instead of leaving a gap at the end.
    // Out of every hundred parts of the width: when it was booked, why, what moved, and how much.
    constexpr int percent = 100;
    constexpr int typeColumnStart = 21;
    constexpr int detailColumnStart = 44;
    constexpr int detailColumnShare = 28;
    constexpr int amountColumnShare = 27;

    const int listLeft = LEDGER_SIDE_INSET;
    const int listRight = layout.width - LEDGER_SIDE_INSET;
    const int listWidth = std::max(1, listRight - listLeft);

    layout.ledgerTimeX = listLeft;
    layout.ledgerTypeX = listLeft + listWidth * typeColumnStart / percent;
    layout.ledgerDetailX = listLeft + listWidth * detailColumnStart / percent;
    layout.ledgerDetailWidth = listWidth * detailColumnShare / percent;
    layout.ledgerAmountRight = listRight;
    layout.ledgerAmountWidth = listWidth * amountColumnShare / percent;
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
    place(BTN_TAB_LEDGER, m_layout.tab[3]);

    place(BTN_PREV, m_layout.prevButton);
    place(BTN_NEXT, m_layout.nextButton);

    place(BTN_PRICE_PREV, m_layout.pricePrevButton);
    place(BTN_PRICE_NEXT, m_layout.priceNextButton);

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

    place(BTN_LEDGER_REFRESH, m_layout.button[1]);

    ApplyLayoutToFilters();
}

void SEASON3B::CNewUIBankWindow::ApplyLayoutToFilters()
{
    // The dropdown of the currencies stands on the last row of the window, where its list has no
    // room under it: told where the window ends, it opens upwards over the boxes instead of past
    // the bottom of the screen, where the currencies at the end of it could not be clicked.
    const int bottomLimit = m_Pos.y + m_layout.height;

    for (int index = 0; index < MAX_FILTER; ++index)
    {
        const RECT& rect = index == FILTER_CURRENCY ? m_layout.marketCurrencyFilter : m_layout.marketFilter[index];
        m_marketFilter[index].SetPos(m_Pos.x + rect.left, m_Pos.y + rect.top);
        m_marketFilter[index].SetBottomLimit(bottomLimit);
    }
}

void SEASON3B::CNewUIBankWindow::BuildMarketFilters()
{
    m_kindFilterLabels = {I18N::Game::BankFilterAnyKind, I18N::Game::BankFilterItems, I18N::Game::BankFilterCurrency};
    m_classFilterLabels = {
        I18N::Game::BankFilterAnyClass, I18N::Game::DarkWizard, I18N::Game::DarkKnight, I18N::Game::Elf,
        I18N::Game::MagicGladiator,     I18N::Game::DarkLord,   I18N::Game::Summoner,   I18N::Game::RageFighter};

    m_currencyFilterLabels.clear();
    m_currencyFilterLabels.push_back(I18N::Game::BankFilterAnyCurrency);
    for (int currency = 0; currency < static_cast<int>(Net::Bank::Currency::Count); ++currency)
    {
        m_currencyFilterLabels.push_back(GetCurrencyName(static_cast<Net::Bank::Currency>(currency)));
    }

    m_setFilters.clear();
    m_setFilters.push_back({I18N::Game::BankFilterAnySet, Net::Bank::AnyFilter, Net::Bank::AnyFilter, 0});

    // The sets come out of the item list of this client, so a server which lists other items is
    // searched by the names of those instead of by a table written out a second time here.
    for (const UI::Market::ArmorSet& set : UI::Market::ReadArmorSets())
    {
        m_setFilters.push_back(
            {set.Name, static_cast<BYTE>(Net::Bank::ItemCategory::Armor), set.Number, set.ClassMask});
    }

    // What is not worn as a set is narrowed by what it is instead.
    const std::pair<const wchar_t*, Net::Bank::ItemCategory> categories[] = {
        {I18N::Game::BankFilterWings, Net::Bank::ItemCategory::Wings},
        {I18N::Game::BankFilterWeapons, Net::Bank::ItemCategory::Weapon},
        {I18N::Game::BankFilterShields, Net::Bank::ItemCategory::Shield},
        {I18N::Game::BankFilterJewelry, Net::Bank::ItemCategory::Jewelry},
        {I18N::Game::BankFilterOtherItems, Net::Bank::ItemCategory::Other},
    };

    for (const auto& [name, category] : categories)
    {
        m_setFilters.push_back({name, static_cast<BYTE>(category), Net::Bank::AnyFilter, 0});
    }

    const int height = m_layout.marketFilter[0].bottom - m_layout.marketFilter[0].top;
    const int width = m_layout.marketFilter[0].right - m_layout.marketFilter[0].left;

    m_marketFilter[FILTER_KIND].Setup(m_Pos.x + m_layout.marketFilter[FILTER_KIND].left,
                                      m_Pos.y + m_layout.marketFilter[FILTER_KIND].top, width, height,
                                      m_kindFilterLabels.data(), static_cast<int>(m_kindFilterLabels.size()), 0);
    m_marketFilter[FILTER_CLASS].Setup(m_Pos.x + m_layout.marketFilter[FILTER_CLASS].left,
                                       m_Pos.y + m_layout.marketFilter[FILTER_CLASS].top, width, height,
                                       m_classFilterLabels.data(), static_cast<int>(m_classFilterLabels.size()), 0);
    m_marketFilter[FILTER_CURRENCY].Setup(
        m_Pos.x + m_layout.marketCurrencyFilter.left, m_Pos.y + m_layout.marketCurrencyFilter.top, width, height,
        m_currencyFilterLabels.data(), static_cast<int>(m_currencyFilterLabels.size()), 0, MAX_FILTER_ROWS);

    RefreshSetFilter();
}

void SEASON3B::CNewUIBankWindow::RefreshSetFilter()
{
    const int family = m_marketFilter[FILTER_CLASS].GetSelectedIndex() - 1;

    m_shownSetFilters.clear();
    m_setFilterLabels.clear();

    for (int index = 0; index < static_cast<int>(m_setFilters.size()); ++index)
    {
        const MarketSetFilter& entry = m_setFilters[index];

        // A set the chosen class cannot wear is not offered to him. An entry which says nothing
        // about a class - everything, and the kinds which are not armour - always is.
        if (family >= 0 && entry.ClassMask != 0 && (entry.ClassMask & (1 << family)) == 0)
        {
            continue;
        }

        m_shownSetFilters.push_back(index);
        m_setFilterLabels.push_back(entry.Name.c_str());
    }

    const RECT& rect = m_layout.marketFilter[FILTER_SET];
    m_marketFilter[FILTER_SET].Setup(m_Pos.x + rect.left, m_Pos.y + rect.top, rect.right - rect.left,
                                     rect.bottom - rect.top, m_setFilterLabels.data(),
                                     static_cast<int>(m_setFilterLabels.size()), 0, MAX_FILTER_ROWS);
}

const SEASON3B::CNewUIBankWindow::MarketSetFilter& SEASON3B::CNewUIBankWindow::GetSelectedSetFilter() const
{
    const int shown = m_marketFilter[FILTER_SET].GetSelectedIndex();
    if (shown >= 0 && shown < static_cast<int>(m_shownSetFilters.size()))
    {
        return m_setFilters[m_shownSetFilters[shown]];
    }

    // The first entry of the list is the one which lets everything through, and so is this, for
    // the frames before the list has been built at all.
    static const MarketSetFilter everything{{}, Net::Bank::AnyFilter, Net::Bank::AnyFilter, 0};
    return m_setFilters.empty() ? everything : m_setFilters.front();
}

bool SEASON3B::CNewUIBankWindow::ProcessMarketFilters()
{
    bool changed = m_marketFilter[FILTER_CLASS].UpdateMouseEvent();
    if (changed)
    {
        // Which sets a player is offered follows from the class he is looking for.
        RefreshSetFilter();
    }

    for (int index = 0; index < MAX_FILTER; ++index)
    {
        if (index != FILTER_CLASS && m_marketFilter[index].UpdateMouseEvent())
        {
            changed = true;
        }
    }

    if (changed)
    {
        m_selectedOffer = -1;
        RequestMarketPage(0);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    // An open dropdown hangs over the boxes under it, so a click in it must not reach them.
    for (const CNewUIComboBox& filter : m_marketFilter)
    {
        if (filter.IsMouseOverWidget())
        {
            return true;
        }
    }

    return false;
}

void SEASON3B::CNewUIBankWindow::RenderMarketFilters()
{
    for (CNewUIComboBox& filter : m_marketFilter)
    {
        filter.Render();
    }
}

void SEASON3B::CNewUIBankWindow::CloseMarketFilters()
{
    for (CNewUIComboBox& filter : m_marketFilter)
    {
        filter.Close();
    }
}

bool SEASON3B::CNewUIBankWindow::IsAnyMarketFilterOpen() const
{
    for (const CNewUIComboBox& filter : m_marketFilter)
    {
        if (filter.IsOpen())
        {
            return true;
        }
    }

    return false;
}

float SEASON3B::CNewUIBankWindow::GetLayerDepth()
{
    // The window is wider than the dock is used to, so whatever else is open - the vault, the
    // chaos machine, a quest window, the inventory - stood over it at the depth of the vault it
    // was given. It is drawn over every one of them now, and under only what has to stay on top
    // of a window: the log this window writes its refusals into, the chat, the tooltip of an
    // item, the menu, the options, the hud frame, and the message boxes it opens itself.
    return 5.9f;
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
    m_waitingForMarketAnswer = false;
    m_pendingInput = PendingInput::None;

    SocketClient->ToGameServer()->SendBankDialog(true);
}

void SEASON3B::CNewUIBankWindow::ClosingProcess()
{
    SocketClient->ToGameServer()->SendBankDialog(false);

    DeleteAllItems();
    ClearOfferItems();
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

void SEASON3B::CNewUIBankWindow::ShowMarketPage()
{
    // The same three lines the tab of the market runs, so reaching the market from the menu and
    // reaching it from the tab cannot drift apart.
    m_page = Page::Market;
    m_selectedOffer = -1;
    RequestMarketPage(0);
}

bool SEASON3B::CNewUIBankWindow::IsMarketPage() const
{
    return m_page == Page::Market;
}

void SEASON3B::CNewUIBankWindow::RequestMarketPage(BYTE page)
{
    // A dropdown lists what it lets through first, so its first row is the one which filters
    // nothing; every row after it names a value the server counts from zero.
    const auto filterValue = [](int selectedIndex)
    { return selectedIndex > 0 ? static_cast<BYTE>(selectedIndex - 1) : Net::Bank::AnyFilter; };

    const MarketSetFilter& set = GetSelectedSetFilter();

    SocketClient->ToGameServer()->SendMarketList(
        page, filterValue(m_marketFilter[FILTER_CURRENCY].GetSelectedIndex()), m_ownOffersOnly, L"",
        filterValue(m_marketFilter[FILTER_KIND].GetSelectedIndex()),
        filterValue(m_marketFilter[FILTER_CLASS].GetSelectedIndex()), set.Category, set.SetNumber);
}

void SEASON3B::CNewUIBankWindow::RequestLedgerPage(BYTE page)
{
    // Which row was picked says nothing about the page which is on its way.
    m_selectedLedgerRow = -1;

    if (page == 0)
    {
        // Starting again from the front: whatever end was found before may have moved.
        m_ledgerLastPage = -1;
    }

    m_ledgerRequestedPage = page;
    SocketClient->ToGameServer()->SendBankLedger(page);
}

void SEASON3B::CNewUIBankWindow::FinishOffer(int64_t price)
{
    const auto priceCurrency = static_cast<Net::Bank::Currency>(m_priceCurrency);

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
        const auto offeredCurrency = static_cast<Net::Bank::Currency>(m_selectedCurrency);
        if (offeredCurrency == priceCurrency)
        {
            // Asking for the same currency which is offered is a trade with itself.
            g_pChatListBox->AddText(L"", I18N::Game::BankPriceCurrencyMustDiffer, SEASON3B::TYPE_ERROR_MESSAGE);
            m_offerAmount = 0;
            return;
        }

        SocketClient->ToGameServer()->SendMarketRegisterCurrency(offeredCurrency, m_offerAmount, priceCurrency, price);
    }

    m_offerAmount = 0;
}

void SEASON3B::CNewUIBankWindow::FinishValueAmount(int64_t amount)
{
    const auto currency = static_cast<Net::Bank::Currency>(m_selectedCurrency);

    // The dialog hands back what was typed; how much of it may actually move is known here. A
    // request which is cut down to nothing is refused out loud, because a button which silently
    // does nothing is the worst of the three answers.
    amount = BankUI::ClampAmount(amount, GetAmountInputContext().Limit);
    if (amount <= 0)
    {
        g_pChatListBox->AddText(L"", I18N::Game::BankNotEnoughValue, SEASON3B::TYPE_ERROR_MESSAGE);
        m_pendingInput = PendingInput::None;
        return;
    }

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

const wchar_t* SEASON3B::CNewUIBankWindow::GetPriceCurrencyName() const
{
    return GetCurrencyName(static_cast<Net::Bank::Currency>(m_priceCurrency));
}

SEASON3B::BankUI::AmountRequest SEASON3B::CNewUIBankWindow::GetAmountInputContext() const
{
    BankUI::AmountRequest context;
    context.Currency = static_cast<Net::Bank::Currency>(m_selectedCurrency);
    context.CurrencyName = GetCurrencyName(context.Currency);
    context.Balance = Net::Bank::Store::Instance().GetBalance(context.Currency);

    switch (m_pendingInput)
    {
    case PendingInput::WithdrawAmount:
        context.What = BankUI::AmountRequest::Purpose::Withdraw;
        break;
    case PendingInput::OfferAmount:
        context.What = BankUI::AmountRequest::Purpose::Offer;
        break;
    default:
        context.What = BankUI::AmountRequest::Purpose::Deposit;
        break;
    }

    // What the character has of the currency: its zen, or the jewels of that kind it is carrying.
    // The balances of the cash shop are on the account and are not among them.
    int64_t carried = 0;
    CNewUIInventoryCtrl* pInventory = g_pMyInventory != nullptr ? g_pMyInventory->GetInventoryCtrl() : nullptr;
    const short jewelType = BankUI::GetJewelItemType(context.Currency);

    if (pInventory != nullptr)
    {
        context.FreeInventorySlots = pInventory->GetEmptySlotCount();

        if (jewelType >= 0)
        {
            carried = pInventory->GetItemCount(jewelType);
        }
    }

    if (context.Currency == Net::Bank::Currency::Zen)
    {
        carried = static_cast<int64_t>(CharacterMachine->Gold);
    }

    switch (context.What)
    {
    case BankUI::AmountRequest::Purpose::Deposit:
        context.Limit = BankUI::GetDepositLimit(context.Currency, carried);
        break;
    case BankUI::AmountRequest::Purpose::Withdraw:
        context.Limit =
            BankUI::GetWithdrawLimit(context.Currency, context.Balance, static_cast<int64_t>(CharacterMachine->Gold),
                                     context.FreeInventorySlots);
        break;
    case BankUI::AmountRequest::Purpose::Offer:
        // What is offered leaves the bank, so the bank is the only thing which limits it.
        context.Limit = std::max<int64_t>(0, context.Balance);
        break;
    }

    return context;
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

void SEASON3B::CNewUIBankWindow::ShowRefusedRequest(Net::Bank::ResultCode result)
{
    // A player who is told only that the bank refused him cannot tell a full bank from a price he
    // cannot afford, so the answers worth acting on say what they mean.
    const wchar_t* message = I18N::Game::BankRefusedTheRequest;
    switch (result)
    {
    case Net::Bank::ResultCode::BankStorageFull:
        message = I18N::Game::BankIsFull;
        break;
    case Net::Bank::ResultCode::InventoryFull:
        message = I18N::Game::InventorySpaceIsInsufficient;
        break;
    case Net::Bank::ResultCode::NotEnoughValue:
        message = I18N::Game::BankNotEnoughValue;
        break;
    case Net::Bank::ResultCode::OfferGone:
        message = I18N::Game::BankOfferIsGone;
        break;
    case Net::Bank::ResultCode::OwnOffer:
        message = I18N::Game::BankOwnOffer;
        break;
    case Net::Bank::ResultCode::TooManyOffers:
        message = I18N::Game::BankTooManyOffers;
        break;
    default:
        break;
    }

    g_pChatListBox->AddText(L"", message, SEASON3B::TYPE_ERROR_MESSAGE);
}

bool SEASON3B::CNewUIBankWindow::Update()
{
    if (!IsVisible())
    {
        return true;
    }

    // Another page of the market arrived, so the items which draw its pictures are built again.
    if (m_offerGeneration != Net::Bank::Store::Instance().GetOfferGeneration())
    {
        RebuildOfferItems();
    }

    Net::Bank::Operation operation = Net::Bank::Operation::Deposit;
    Net::Bank::ResultCode result = Net::Bank::ResultCode::Success;
    if (Net::Bank::Store::Instance().TakeNewResult(operation, result))
    {
        if (operation == Net::Bank::Operation::MarketBuy || operation == Net::Bank::Operation::MarketCancel)
        {
            m_waitingForMarketAnswer = false;
        }

        if (result != Net::Bank::ResultCode::Success)
        {
            ShowRefusedRequest(result);
        }
        else if (operation == Net::Bank::Operation::MarketRegister || operation == Net::Bank::Operation::MarketBuy ||
                 operation == Net::Bank::Operation::MarketCancel)
        {
            m_selectedOffer = -1;
            RequestMarketPage(Net::Bank::Store::Instance().GetOfferPage());
        }

        if (result == Net::Bank::ResultCode::Success && m_page == Page::Ledger)
        {
            RequestLedgerPage(Net::Bank::Store::Instance().GetLedgerPage());
        }
    }

    // Only the answer to the page which was asked for says anything about where the ledger ends.
    // While another one is on its way the store still holds the page before it, and reading that
    // one again in every frame turned a single correction into a request per frame.
    if (m_page == Page::Ledger && Net::Bank::Store::Instance().GetLedgerPage() == m_ledgerRequestedPage)
    {
        const auto& store = Net::Bank::Store::Instance();
        const int page = store.GetLedgerPage();

        if (store.GetLedger().empty() && page > 0)
        {
            // Turned past the end. The page before this one was the last, and the window goes back
            // to it rather than leaving the player looking at nothing.
            m_ledgerLastPage = page - 1;
            RequestLedgerPage(static_cast<BYTE>(page - 1));
        }
        else if (!store.GetLedger().empty() && m_ledgerLastPage >= 0 && page > m_ledgerLastPage)
        {
            m_ledgerLastPage = page;
        }
    }

    // A dialog is opened from here and never from the callback of another one, so a message box is
    // never created while the box which asked for it is being destroyed.
    if (m_pendingInput != PendingInput::None && g_MessageBox && g_MessageBox->IsEmpty())
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

    if (m_page == Page::Market && ProcessMarketFilters())
    {
        return false;
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

    if (m_page == Page::Ledger && ProcessLedgerSelection())
    {
        return false;
    }

    // The window swallows what happens over it, so a click next to a box does not walk the
    // character to the other side of the map.
    if (SEASON3B::CheckMouseIn(m_Pos.x, m_Pos.y, m_layout.width, m_layout.height))
    {
        if (g_pPickedItem && g_pPickedItem->GetItem() && SEASON3B::IsRelease(VK_LBUTTON))
        {
            CNewUIInventoryCtrl::BackupPickedItem();
        }

        return false;
    }

    return true;
}

bool SEASON3B::CNewUIBankWindow::ProcessTabs()
{
    if (m_abtn[BTN_TAB_ITEMS].UpdateMouseEvent())
    {
        CloseMarketFilters();
        m_page = Page::Items;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_TAB_VALUES].UpdateMouseEvent())
    {
        CloseMarketFilters();
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

    if (m_abtn[BTN_TAB_LEDGER].UpdateMouseEvent())
    {
        CloseMarketFilters();
        m_page = Page::Ledger;
        RequestLedgerPage(0);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool SEASON3B::CNewUIBankWindow::ProcessPriceCurrencyButtons()
{
    const int count = static_cast<int>(Net::Bank::Currency::Count);

    if (m_abtn[BTN_PRICE_PREV].UpdateMouseEvent())
    {
        m_priceCurrency = (m_priceCurrency + count - 1) % count;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_PRICE_NEXT].UpdateMouseEvent())
    {
        m_priceCurrency = (m_priceCurrency + 1) % count;
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool SEASON3B::CNewUIBankWindow::ProcessButtons()
{
    // The row which names the currency of a price belongs to the two tabs which offer something.
    if ((m_page == Page::Items || m_page == Page::Values) && ProcessPriceCurrencyButtons())
    {
        return true;
    }

    if (m_page != Page::Values)
    {
        if (m_abtn[BTN_PREV].UpdateMouseEvent())
        {
            const auto& store = Net::Bank::Store::Instance();
            switch (m_page)
            {
            case Page::Items:
                m_itemPage = m_itemPage > 0 ? m_itemPage - 1 : m_layout.itemPageCount - 1;
                break;
            case Page::Market:
            {
                const BYTE page = store.GetOfferPage();
                RequestMarketPage(page > 0 ? static_cast<BYTE>(page - 1) : 0);
                break;
            }
            case Page::Ledger:
            {
                const BYTE page = store.GetLedgerPage();
                if (page > 0)
                {
                    RequestLedgerPage(static_cast<BYTE>(page - 1));
                }

                break;
            }
            default:
                break;
            }

            PlayBuffer(SOUND_CLICK01);
            return true;
        }

        if (m_abtn[BTN_NEXT].UpdateMouseEvent())
        {
            const auto& store = Net::Bank::Store::Instance();
            switch (m_page)
            {
            case Page::Items:
                m_itemPage = (m_itemPage + 1) % m_layout.itemPageCount;
                break;
            case Page::Market:
            {
                const BYTE page = store.GetOfferPage();
                if (page + 1 < store.GetOfferPageCount())
                {
                    RequestMarketPage(static_cast<BYTE>(page + 1));
                }

                break;
            }
            case Page::Ledger:
                // Turning forwards stops at the end once the end has been found.
                if (!store.GetLedger().empty() && (m_ledgerLastPage < 0 || store.GetLedgerPage() < m_ledgerLastPage))
                {
                    RequestLedgerPage(static_cast<BYTE>(store.GetLedgerPage() + 1));
                }

                break;
            default:
                break;
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
    case Page::Ledger:
        return ProcessLedgerPageButtons();
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

bool SEASON3B::CNewUIBankWindow::ProcessLedgerPageButtons()
{
    if (m_abtn[BTN_LEDGER_REFRESH].UpdateMouseEvent())
    {
        RequestLedgerPage(Net::Bank::Store::Instance().GetLedgerPage());
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

void SEASON3B::CNewUIBankWindow::BuySelectedOffer()
{
    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    if (m_selectedOffer < 0 || m_selectedOffer >= static_cast<int>(offers.size()) || m_waitingForMarketAnswer)
    {
        return;
    }

    m_pendingListingId = offers[m_selectedOffer].ListingId;
    m_waitingForMarketAnswer = true;
    SocketClient->ToGameServer()->SendMarketBuy(m_pendingListingId.data());
}

void SEASON3B::CNewUIBankWindow::CancelSelectedOffer()
{
    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    if (m_selectedOffer < 0 || m_selectedOffer >= static_cast<int>(offers.size()) || m_waitingForMarketAnswer)
    {
        return;
    }

    m_pendingListingId = offers[m_selectedOffer].ListingId;
    m_waitingForMarketAnswer = true;
    SocketClient->ToGameServer()->SendMarketCancel(m_pendingListingId.data());
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
    return m_Pos.y + (isMoney ? m_layout.moneyRowsTop : m_layout.jewelRowsTop) + rowInGroup * m_layout.valueLineHeight;
}

void SEASON3B::CNewUIBankWindow::GetCurrencyIconRect(int currency, RECT& rect) const
{
    // The picture is as tall as it is wide and sits in the middle of its row, so a row which grows
    // taller keeps it centred instead of leaving it at the top.
    const int top = GetCurrencyRowTop(currency) + (m_layout.valueLineHeight - m_layout.valueIconSize) / 2;

    rect.left = m_Pos.x + CURRENCY_ICON_X;
    rect.top = top;
    rect.right = rect.left + m_layout.valueIconSize;
    rect.bottom = top + m_layout.valueIconSize;
}

void SEASON3B::CNewUIBankWindow::GetOfferTileRect(int tile, RECT& rect) const
{
    const int column = tile % m_layout.marketTileColumns;
    const int row = tile / m_layout.marketTileColumns;

    rect.left = m_Pos.x + m_layout.marketTileOriginX + column * m_layout.marketTileWidth;
    rect.top = m_Pos.y + m_layout.marketTileOriginY + row * m_layout.marketTileHeight;
    rect.right = rect.left + m_layout.marketTileWidth;
    rect.bottom = rect.top + m_layout.marketTileHeight;
}

int SEASON3B::CNewUIBankWindow::GetShownOfferCount() const
{
    // A page can hold more boxes than the server sends offers for; those stand empty.
    return std::min(static_cast<int>(Net::Bank::Store::Instance().GetOffers().size()), m_layout.marketTilesPerPage);
}

int SEASON3B::CNewUIBankWindow::GetOfferTileAtCursor() const
{
    const int shown = GetShownOfferCount();
    for (int tile = 0; tile < shown; ++tile)
    {
        RECT rect;
        GetOfferTileRect(tile, rect);
        if (SEASON3B::CheckMouseIn(rect.left, rect.top, m_layout.marketTileWidth, m_layout.marketTileHeight))
        {
            return tile;
        }
    }

    return -1;
}

int SEASON3B::CNewUIBankWindow::GetLedgerRowTop(int row) const
{
    return m_Pos.y + m_layout.ledgerRowsTop + row * m_layout.listLineHeight;
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

    if (g_pPickedItem && g_pPickedItem->GetItem())
    {
        // The bank is not used by carrying items into it - a right click in the inventory is what
        // puts one in. An item which is already on the cursor goes back where it came from, so it
        // is never left with nowhere to go.
        if (SEASON3B::IsRelease(VK_LBUTTON))
        {
            CNewUIInventoryCtrl::BackupPickedItem();
        }

        return true;
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
        if (SEASON3B::CheckMouseIn(m_Pos.x + ROW_INSET, top, m_layout.width - 2 * ROW_INSET, m_layout.valueLineHeight))
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
    const int tile = GetOfferTileAtCursor();
    if (tile < 0)
    {
        return false;
    }

    if (SEASON3B::IsRelease(VK_LBUTTON))
    {
        m_selectedOffer = tile;
        PlayBuffer(SOUND_CLICK01);
    }

    // The cursor is over a box, so nothing behind the window may take this frame.
    return true;
}

bool SEASON3B::CNewUIBankWindow::ProcessLedgerSelection()
{
    const auto& entries = Net::Bank::Store::Instance().GetLedger();
    const int rows = std::min(static_cast<int>(entries.size()), m_layout.ledgerRows);

    for (int row = 0; row < rows; ++row)
    {
        const int top = GetLedgerRowTop(row);
        if (SEASON3B::CheckMouseIn(m_Pos.x + ROW_INSET, top, m_layout.width - 2 * ROW_INSET, m_layout.listLineHeight))
        {
            if (SEASON3B::IsRelease(VK_LBUTTON))
            {
                m_selectedLedgerRow = row;
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
    case Page::Ledger:
        RenderLedgerPage();
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
    UseTextColor(TITLE_TEXT);
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
    RenderButton(m_abtn[BTN_TAB_LEDGER], m_page == Page::Ledger);
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

        RenderTilePlate(rect, fill);

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

    UseTextColor(BOX_COUNT_TEXT);
    mu_swprintf(szText, I18N::Game::BankBoxCount, used, BANK_TOTAL_SLOTS);
    g_pRenderText->RenderText(m_Pos.x + ROW_INSET, m_Pos.y + m_layout.infoRowTop, szText, LABEL_WIDTH, 0);

    // The name of what is picked, and nothing at all when nothing is.
    if (m_selectedSlot >= 0 && m_boxes[m_selectedSlot] != nullptr)
    {
        UseTextColor(HIGHLIGHT_TEXT);
        GetItemName(m_boxes[m_selectedSlot]->Type, m_boxes[m_selectedSlot]->Level, szText);
        g_pRenderText->RenderText(m_Pos.x + PICKED_NAME_X, m_Pos.y + m_layout.infoRowTop, szText,
                                  m_layout.width - PICKED_NAME_X - ROW_INSET, 0, RT3_SORT_CENTER);
    }

    UseTextColor(LABEL_TEXT);
    mu_swprintf(szText, I18N::Game::BankPageOf, m_itemPage + 1, m_layout.itemPageCount);
    g_pRenderText->RenderText(m_Pos.x + m_layout.prevButton.right, m_Pos.y + m_layout.pageRowTop + BUTTON_ROW_TEXT_TOP,
                              szText, m_layout.nextButton.left - m_layout.prevButton.right, 0, RT3_SORT_CENTER);

    RenderButton(m_abtn[BTN_PREV], false);
    RenderButton(m_abtn[BTN_NEXT], false);
    RenderButton(m_abtn[BTN_TAKE_ITEM], false);
    RenderButton(m_abtn[BTN_OFFER_ITEM], false);

    RenderPriceCurrencyRow();
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
        UseTextColor(HEADING_TEXT);
        g_pRenderText->RenderText(m_Pos.x + TEXT_INSET, m_Pos.y + headerTop,
                                  group == 0 ? I18N::Game::BankMoneyGroup : I18N::Game::BankJewelGroup, 240, 0);

        RenderColorQuadARGB(static_cast<float>(m_Pos.x + ROW_INSET),
                            static_cast<float>(m_Pos.y + headerTop + HEADING_LINE_TOP),
                            static_cast<float>(m_layout.width - 2 * ROW_INSET), 1.f, HEADING_LINE_COLOR);
        EndRenderColor();

        const int first = group == 0 ? 0 : MONEY_CURRENCY_COUNT;
        const int last = group == 0 ? MONEY_CURRENCY_COUNT : static_cast<int>(Net::Bank::Currency::Count);

        g_pRenderText->SetFont(g_hFont);
        for (int currency = first; currency < last; ++currency)
        {
            const int top = GetCurrencyRowTop(currency);

            if (currency == m_selectedCurrency)
            {
                RenderColorQuadARGB(static_cast<float>(m_Pos.x + ROW_INSET), static_cast<float>(top),
                                    static_cast<float>(m_layout.width - 2 * ROW_INSET),
                                    static_cast<float>(m_layout.valueLineHeight), PICKED_FILL_COLOR);
                RenderBorder(m_Pos.x + ROW_INSET, top, m_layout.width - 2 * ROW_INSET, m_layout.valueLineHeight,
                             PICKED_EDGE_COLOR, 1);
                EndRenderColor();
                UseTextColor(PICKED_ROW_TEXT);
            }
            else
            {
                UseTextColor(ROW_TEXT);
            }

            if (group == 0)
            {
                // The jewels of the group below draw their pictures in the 3d pass; a coin is not
                // an item and is drawn here, with the rest of the page.
                RECT iconRect;
                GetCurrencyIconRect(currency, iconRect);
                RenderCurrencyCoin(iconRect, static_cast<Net::Bank::Currency>(currency));

                // The coin wrote in its own colour and font, so the row takes both back.
                g_pRenderText->SetFont(g_hFont);
                UseTextColor(currency == m_selectedCurrency ? PICKED_ROW_TEXT : ROW_TEXT);
            }

            g_pRenderText->RenderText(m_Pos.x + CURRENCY_NAME_X, top + m_layout.valueTextTop,
                                      GetCurrencyName(static_cast<Net::Bank::Currency>(currency)), CURRENCY_NAME_WIDTH,
                                      0);

            FormatAmount(Net::Bank::Store::Instance().GetBalance(static_cast<Net::Bank::Currency>(currency)), szAmount,
                         std::size(szAmount));
            g_pRenderText->RenderText(m_Pos.x + CURRENCY_AMOUNT_X, top + m_layout.valueTextTop, szAmount,
                                      m_layout.width - CURRENCY_AMOUNT_X - CURRENCY_AMOUNT_END, 0, RT3_SORT_RIGHT);
        }
    }

    RenderPriceCurrencyRow();

    RenderButton(m_abtn[BTN_DEPOSIT], false);
    RenderButton(m_abtn[BTN_WITHDRAW], false);
    RenderButton(m_abtn[BTN_OFFER_VALUE], false);
}

void SEASON3B::CNewUIBankWindow::RenderMarketPage()
{
    wchar_t szText[128] = {0};

    RenderOfferTilePlates();
    RenderOfferTilePrices();
    RenderSelectedOfferDetail();

    const auto& store = Net::Bank::Store::Instance();
    g_pRenderText->SetFont(g_hFont);
    UseTextColor(LABEL_TEXT);
    mu_swprintf(szText, I18N::Game::BankPageOf, store.GetOfferPage() + 1, store.GetOfferPageCount());
    g_pRenderText->RenderText(m_Pos.x + m_layout.prevButton.right, m_Pos.y + m_layout.pageRowTop + BUTTON_ROW_TEXT_TOP,
                              szText, m_layout.nextButton.left - m_layout.prevButton.right, 0, RT3_SORT_CENTER);

    RenderButton(m_abtn[BTN_PREV], false);
    RenderButton(m_abtn[BTN_NEXT], false);
    RenderButton(m_abtn[BTN_BUY], false);
    RenderButton(m_abtn[BTN_CANCEL_OFFER], false);
    RenderButton(m_abtn[BTN_MINE], m_ownOffersOnly);

    // Last, because an open one hangs over the boxes it stands above.
    RenderMarketFilters();

    RenderHoveredOfferInfo();
}

void SEASON3B::CNewUIBankWindow::RenderOfferTilePlates()
{
    const int gridX = m_Pos.x + m_layout.marketTileOriginX;
    const int gridY = m_Pos.y + m_layout.marketTileOriginY;
    const int gridWidth = m_layout.marketTileColumns * m_layout.marketTileWidth;
    const int gridHeight = m_layout.marketTileRows * m_layout.marketTileHeight;

    RenderColorQuadARGB(static_cast<float>(gridX), static_cast<float>(gridY), static_cast<float>(gridWidth),
                        static_cast<float>(gridHeight), GRID_BACK_COLOR);

    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    const int shown = GetShownOfferCount();
    const int hovered = GetOfferTileAtCursor();

    for (int tile = 0; tile < m_layout.marketTilesPerPage; ++tile)
    {
        RECT rect;
        GetOfferTileRect(tile, rect);

        unsigned int fill = TILE_COLOR;
        if (tile >= shown)
        {
            // The last page holds fewer offers than boxes; those stand empty and dark.
            fill = GRID_BACK_COLOR;
        }
        else if (tile == m_selectedOffer)
        {
            fill = PICKED_FILL_COLOR;
        }
        else if (tile == hovered)
        {
            fill = HOVERED_FILL_COLOR;
        }

        RenderTilePlate(rect, fill);

        if (tile >= shown)
        {
            continue;
        }

        if (tile == m_selectedOffer)
        {
            RenderBorder(rect.left, rect.top, m_layout.marketTileWidth, m_layout.marketTileHeight, PICKED_EDGE_COLOR,
                         2);
        }
        else if (IsOwnOffer(offers[tile]))
        {
            // An own offer cannot be bought, only taken back, so it says so before it is clicked.
            RenderBorder(rect.left, rect.top, m_layout.marketTileWidth, m_layout.marketTileHeight, OWN_OFFER_EDGE_COLOR,
                         1);
        }
    }

    RenderBorder(gridX - 4, gridY - 4, gridWidth + 8, gridHeight + 8, GRID_FRAME_COLOR, 2);
    EndRenderColor();
}

void SEASON3B::CNewUIBankWindow::RenderOfferTilePrices()
{
    wchar_t szText[64] = {0};

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);

    const int shown = GetShownOfferCount();
    if (shown == 0)
    {
        UseTextColor(EMPTY_LIST_TEXT);
        g_pRenderText->RenderText(m_Pos.x + TEXT_INSET,
                                  m_Pos.y + m_layout.marketTileOriginY + m_layout.marketTileHeight,
                                  I18N::Game::BankHasNoOffers, m_layout.width - 2 * TEXT_INSET, 0, RT3_SORT_CENTER);
        return;
    }

    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    const int textWidth = m_layout.marketTileWidth - 2 * TILE_TEXT_INSET;

    for (int tile = 0; tile < shown; ++tile)
    {
        const Net::Bank::Offer& offer = offers[tile];
        RECT rect;
        GetOfferTileRect(tile, rect);

        if (offer.Kind == Net::Bank::OfferKind::Currency)
        {
            // An amount of a currency has no picture, so its own amount fills the space one would
            // have taken.
            UseTextColor(ROW_TEXT);
            FormatPriceForTile(offer.OfferedAmount, szText, std::size(szText));
            g_pRenderText->RenderText(rect.left + TILE_TEXT_INSET,
                                      rect.top + m_layout.marketPictureHeight / 2 - PRICE_LINE_HEIGHT, szText,
                                      textWidth, 0, RT3_SORT_CENTER);
            g_pRenderText->RenderText(rect.left + TILE_TEXT_INSET, rect.top + m_layout.marketPictureHeight / 2,
                                      GetCurrencyShortName(offer.OfferedCurrency), textWidth, 0, RT3_SORT_CENTER);
        }
        else if (tile < static_cast<int>(m_offerItems.size()) && m_offerItems[tile] != nullptr &&
                 m_offerItems[tile]->Level > 0)
        {
            // Two of the same item at two prices differ by their level, which no picture shows.
            UseTextColor(HIGHLIGHT_TEXT);
            mu_swprintf(szText, L"+%d", static_cast<int>(m_offerItems[tile]->Level));
            g_pRenderText->RenderText(rect.left + TILE_TEXT_INSET, rect.top + TILE_TEXT_INSET, szText, textWidth, 0,
                                      RT3_SORT_RIGHT);
        }

        const int priceTop = rect.top + m_layout.marketPictureHeight;

        UseTextColor(HIGHLIGHT_TEXT);
        FormatPriceForTile(offer.PriceAmount, szText, std::size(szText));
        g_pRenderText->RenderText(rect.left + TILE_TEXT_INSET, priceTop, szText, textWidth, 0, RT3_SORT_CENTER);

        UseTextColor(MUTED_TEXT);
        g_pRenderText->RenderText(rect.left + TILE_TEXT_INSET, priceTop + PRICE_LINE_HEIGHT,
                                  GetCurrencyShortName(offer.PriceCurrency), textWidth, 0, RT3_SORT_CENTER);
    }
}

void SEASON3B::CNewUIBankWindow::RenderSelectedOfferDetail()
{
    if (m_selectedOffer < 0 || m_selectedOffer >= GetShownOfferCount())
    {
        return;
    }

    wchar_t szAmount[64] = {0};
    wchar_t szText[256] = {0};

    const Net::Bank::Offer& offer = Net::Bank::Store::Instance().GetOffers()[m_selectedOffer];

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);
    UseTextColor(HIGHLIGHT_TEXT);
    g_pRenderText->RenderText(m_Pos.x + ROW_INSET, m_Pos.y + m_layout.infoRowTop, offer.OfferName.c_str(),
                              m_layout.width / 2 - TEXT_INSET, 0);

    // The price stands here in full, because the strip under a picture holds a shortened one, and
    // beside it stands who is asking for it.
    FormatAmount(offer.PriceAmount, szAmount, std::size(szAmount));
    mu_swprintf(szText, L"%ls %ls%ls%ls", szAmount, GetCurrencyName(offer.PriceCurrency), DETAIL_SEPARATOR,
                offer.SellerName.c_str());

    UseTextColor(LABEL_TEXT);
    g_pRenderText->RenderText(m_Pos.x + m_layout.width / 2, m_Pos.y + m_layout.infoRowTop, szText,
                              m_layout.width / 2 - ROW_INSET, 0, RT3_SORT_RIGHT);
}

void SEASON3B::CNewUIBankWindow::RenderLedgerPage()
{
    wchar_t szText[256] = {0};
    wchar_t szAmount[64] = {0};
    wchar_t szTime[32] = {0};

    const int timeColumn = m_Pos.x + m_layout.ledgerTimeX;
    const int typeColumn = m_Pos.x + m_layout.ledgerTypeX;
    const int detailColumn = m_Pos.x + m_layout.ledgerDetailX;
    const int amountColumn = m_Pos.x + m_layout.ledgerAmountRight - m_layout.ledgerAmountWidth;
    const int timeWidth = m_layout.ledgerTypeX - m_layout.ledgerTimeX;
    const int typeWidth = m_layout.ledgerDetailX - m_layout.ledgerTypeX;

    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetFont(g_hFontBold);
    UseTextColor(HEADING_TEXT);
    g_pRenderText->RenderText(timeColumn, m_Pos.y + m_layout.ledgerHeaderTop, I18N::Game::BankTimeColumn, timeWidth, 0);
    g_pRenderText->RenderText(typeColumn, m_Pos.y + m_layout.ledgerHeaderTop, I18N::Game::BankTypeColumn, typeWidth, 0);
    g_pRenderText->RenderText(detailColumn, m_Pos.y + m_layout.ledgerHeaderTop, I18N::Game::BankDetailColumn,
                              m_layout.ledgerDetailWidth, 0);
    g_pRenderText->RenderText(amountColumn, m_Pos.y + m_layout.ledgerHeaderTop, I18N::Game::BankAmountColumn,
                              m_layout.ledgerAmountWidth, 0, RT3_SORT_RIGHT);

    RenderColorQuadARGB(static_cast<float>(m_Pos.x + ROW_INSET),
                        static_cast<float>(m_Pos.y + m_layout.ledgerHeaderTop + 16),
                        static_cast<float>(m_layout.width - 2 * ROW_INSET), 1.f, HEADING_LINE_COLOR);
    EndRenderColor();

    const auto& entries = Net::Bank::Store::Instance().GetLedger();
    g_pRenderText->SetFont(g_hFont);

    if (entries.empty())
    {
        UseTextColor(EMPTY_LIST_TEXT);
        g_pRenderText->RenderText(m_Pos.x + TEXT_INSET, m_Pos.y + m_layout.ledgerRowsTop + EMPTY_LIST_TOP,
                                  I18N::Game::BankHasNoLedgerEntries, m_layout.width - 2 * TEXT_INSET, 0,
                                  RT3_SORT_CENTER);
    }

    const int rows = std::min(static_cast<int>(entries.size()), m_layout.ledgerRows);
    for (int row = 0; row < rows; ++row)
    {
        const Net::Bank::LedgerEntry& entry = entries[row];
        const int top = GetLedgerRowTop(row);
        const bool picked = row == m_selectedLedgerRow;

        if (picked)
        {
            RenderColorQuadARGB(static_cast<float>(m_Pos.x + ROW_INSET), static_cast<float>(top),
                                static_cast<float>(m_layout.width - 2 * ROW_INSET),
                                static_cast<float>(m_layout.listLineHeight), PICKED_FILL_COLOR);
            RenderBorder(m_Pos.x + ROW_INSET, top, m_layout.width - 2 * ROW_INSET, m_layout.listLineHeight,
                         PICKED_EDGE_COLOR, 1);
            EndRenderColor();
            UseTextColor(PICKED_ROW_TEXT);
        }
        else
        {
            UseTextColor(ROW_TEXT);
        }

        FormatLedgerTime(entry.Timestamp, szTime, std::size(szTime));
        g_pRenderText->RenderText(timeColumn, top + LIST_ROW_TEXT_TOP, szTime, timeWidth, 0);
        g_pRenderText->RenderText(typeColumn, top + LIST_ROW_TEXT_TOP, GetLedgerTypeName(entry.Type), typeWidth, 0);

        const wchar_t* detail = GetLedgerDetail(entry);
        g_pRenderText->RenderText(detailColumn, top + LIST_ROW_TEXT_TOP, *detail != L'\0' ? detail : NOTHING_BOOKED,
                                  m_layout.ledgerDetailWidth, 0);

        // A movement of items books no amount: the server writes a zero and leaves the currency at
        // zen, so a "0 Zen" drawn here would be a number nobody booked.
        if (entry.Amount == 0)
        {
            UseTextColor(NOTHING_BOOKED_TEXT);
            g_pRenderText->RenderText(amountColumn, top + LIST_ROW_TEXT_TOP, NOTHING_BOOKED, m_layout.ledgerAmountWidth,
                                      0, RT3_SORT_RIGHT);
            continue;
        }

        if (entry.Amount > 0)
        {
            UseTextColor(GAINED_TEXT);
        }
        else
        {
            UseTextColor(LOST_TEXT);
        }

        FormatAmount(entry.Amount, szAmount, std::size(szAmount));
        mu_swprintf(szText, L"%ls%ls %ls", entry.Amount > 0 ? L"+" : L"", szAmount,
                    GetCurrencyShortName(entry.EntryCurrency));
        g_pRenderText->RenderText(amountColumn, top + LIST_ROW_TEXT_TOP, szText, m_layout.ledgerAmountWidth, 0,
                                  RT3_SORT_RIGHT);
    }

    // What the picked row holds, in full: a description of sixty letters never fits its column, and
    // the balance belongs beside it rather than in a fifth column nothing would have room for.
    if (m_selectedLedgerRow >= 0 && m_selectedLedgerRow < rows)
    {
        const Net::Bank::LedgerEntry& entry = entries[m_selectedLedgerRow];
        const wchar_t* detail = GetLedgerDetail(entry);

        UseTextColor(LABEL_TEXT);
        if (*detail != L'\0')
        {
            g_pRenderText->RenderText(m_Pos.x + TEXT_INSET, m_Pos.y + m_layout.infoRowTop, detail,
                                      m_layout.width / 2 - HALF_ROW_MARGIN, 0);
        }

        if (entry.Amount != 0)
        {
            FormatAmount(entry.BalanceAfter, szAmount, std::size(szAmount));
            mu_swprintf(szText, I18N::Game::BankBalanceAfter, szAmount, GetCurrencyShortName(entry.EntryCurrency));
            g_pRenderText->RenderText(m_Pos.x + m_layout.width / 2, m_Pos.y + m_layout.infoRowTop, szText,
                                      m_layout.width / 2 - TEXT_INSET, 0, RT3_SORT_RIGHT);
        }
    }

    // The server sends no count of pages, so the number stands alone instead of promising a total
    // the client would have to invent.
    UseTextColor(LABEL_TEXT);
    mu_swprintf(szText, I18N::Game::BankPageNumber, Net::Bank::Store::Instance().GetLedgerPage() + 1);
    g_pRenderText->RenderText(m_Pos.x + m_layout.prevButton.right, m_Pos.y + m_layout.pageRowTop + BUTTON_ROW_TEXT_TOP,
                              szText, m_layout.nextButton.left - m_layout.prevButton.right, 0, RT3_SORT_CENTER);

    RenderButton(m_abtn[BTN_PREV], false);
    RenderButton(m_abtn[BTN_NEXT], false);
    RenderButton(m_abtn[BTN_LEDGER_REFRESH], false);
}

void SEASON3B::CNewUIBankWindow::RenderPriceCurrencyRow()
{
    wchar_t szText[128] = {0};

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);
    UseTextColor(LABEL_TEXT);
    mu_swprintf(szText, L"%ls:", I18N::Game::BankPriceIn);
    g_pRenderText->RenderText(m_Pos.x + TEXT_INSET, m_Pos.y + m_layout.priceRowTop + BUTTON_ROW_TEXT_TOP, szText,
                              LABEL_WIDTH, 0);

    UseTextColor(HIGHLIGHT_TEXT);
    g_pRenderText->RenderText(m_Pos.x + m_layout.pricePrevButton.right,
                              m_Pos.y + m_layout.priceRowTop + BUTTON_ROW_TEXT_TOP,
                              GetCurrencyName(static_cast<Net::Bank::Currency>(m_priceCurrency)),
                              m_layout.priceNextButton.left - m_layout.pricePrevButton.right, 0, RT3_SORT_CENTER);

    RenderButton(m_abtn[BTN_PRICE_PREV], false);
    RenderButton(m_abtn[BTN_PRICE_NEXT], false);
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

    m_pNewUI3DRenderMng->RenderUI2DEffect(BANK_CAMERA_Z_ORDER, UI2DEffectCallback, this, RENDER_ITEM_TOOLTIP, 0);
}

void SEASON3B::CNewUIBankWindow::RenderHoveredOfferInfo()
{
    if (m_pNewUI3DRenderMng == nullptr || (g_pPickedItem && g_pPickedItem->GetItem()))
    {
        return;
    }

    if (IsAnyMarketFilterOpen())
    {
        // The cursor is reading a dropdown, not the box which happens to be under it.
        return;
    }

    const int tile = GetOfferTileAtCursor();
    if (tile < 0 || tile >= static_cast<int>(m_offerItems.size()) || m_offerItems[tile] == nullptr)
    {
        return;
    }

    m_pNewUI3DRenderMng->RenderUI2DEffect(BANK_CAMERA_Z_ORDER, UI2DEffectCallback, this, RENDER_OFFER_TOOLTIP, 0);
}

void SEASON3B::CNewUIBankWindow::UI2DEffectCallback(LPVOID pClass, DWORD dwParamA, DWORD dwParamB)
{
    auto* pWindow = static_cast<CNewUIBankWindow*>(pClass);
    if (pWindow == nullptr)
    {
        return;
    }

    RECT rect;

    if (dwParamA == RENDER_ITEM_TOOLTIP)
    {
        const int slot = pWindow->GetTileAtCursor();
        if (slot < 0 || pWindow->m_boxes[slot] == nullptr)
        {
            return;
        }

        pWindow->GetTileRect(slot - pWindow->m_itemPage * pWindow->m_layout.itemsPerPage, rect);
        RenderItemInfo(rect.left + pWindow->m_layout.tileSize / 2, rect.top, pWindow->m_boxes[slot], false);
        return;
    }

    if (dwParamA != RENDER_OFFER_TOOLTIP)
    {
        return;
    }

    const int tile = pWindow->GetOfferTileAtCursor();
    if (tile < 0 || tile >= static_cast<int>(pWindow->m_offerItems.size()) || pWindow->m_offerItems[tile] == nullptr)
    {
        return;
    }

    pWindow->GetOfferTileRect(tile, rect);
    RenderItemInfo(rect.left + pWindow->m_layout.marketTileWidth / 2, rect.top, pWindow->m_offerItems[tile], false);
}

void SEASON3B::CNewUIBankWindow::Render3D()
{
    if (!IsVisible())
    {
        return;
    }

    if (m_page == Page::Items)
    {
        RenderStorageItems3D();
    }
    else if (m_page == Page::Values)
    {
        RenderValueIcons3D();
    }
    else if (m_page == Page::Market)
    {
        RenderOfferItems3D();
    }
}

void SEASON3B::CNewUIBankWindow::RenderStorageItems3D()
{
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
        RenderItemPicture(*pItem, rect.left, rect.top, m_layout.tileSize, m_layout.tileSize, PICTURE_MARGIN);
    }
}

void SEASON3B::CNewUIBankWindow::RenderValueIcons3D()
{
    for (int currency = MONEY_CURRENCY_COUNT; currency < static_cast<int>(Net::Bank::Currency::Count); ++currency)
    {
        const short type = BankUI::GetJewelItemType(static_cast<Net::Bank::Currency>(currency));
        if (type < 0)
        {
            continue;
        }

        // The row draws a jewel, not one the player owns, so the item is built here and thrown
        // away again rather than taken from the item manager, which keeps what it hands out.
        ITEM jewel{};
        jewel.Type = type;

        RECT rect;
        GetCurrencyIconRect(currency, rect);
        RenderItemPicture(jewel, rect.left, rect.top, m_layout.valueIconSize, m_layout.valueIconSize, 0);
    }
}

void SEASON3B::CNewUIBankWindow::RenderOfferItems3D()
{
    // The pictures are drawn in a pass of their own, after everything this window draws, so an
    // open dropdown cannot cover them - and a picture drawn through a list nobody can read is
    // worse than a box left empty while that list is open.
    if (IsAnyMarketFilterOpen())
    {
        return;
    }

    const int shown = std::min(GetShownOfferCount(), static_cast<int>(m_offerItems.size()));
    for (int tile = 0; tile < shown; ++tile)
    {
        const ITEM* pItem = m_offerItems[tile];
        if (pItem == nullptr)
        {
            continue;
        }

        RECT rect;
        GetOfferTileRect(tile, rect);

        // Only the upper part of a box holds the picture; under it stands what the offer costs.
        RenderItemPicture(*pItem, rect.left, rect.top, m_layout.marketTileWidth, m_layout.marketPictureHeight,
                          PICTURE_MARGIN);
    }
}

void SEASON3B::CNewUIBankWindow::RenderTilePlate(const RECT& rect, unsigned int fill)
{
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;

    RenderColorQuadARGB(static_cast<float>(rect.left + 1), static_cast<float>(rect.top + 1),
                        static_cast<float>(width - 2), static_cast<float>(height - 2), fill);
    RenderBorder(rect.left, rect.top, width, height, TILE_LINE_COLOR, 1);
}

void SEASON3B::CNewUIBankWindow::RenderCurrencyCoin(const RECT& rect, Net::Bank::Currency currency)
{
    const BankUI::CoinColors colors = BankUI::GetCoinColors(currency);

    const float left = static_cast<float>(rect.left);
    const float top = static_cast<float>(rect.top);
    const float size = static_cast<float>(rect.right - rect.left);

    // Two crossed quads make an octagon, and an octagon this small reads as a coin. The rim is one
    // such shape and the face a smaller one inside it, so the coin keeps its edge at any size.
    const float rimCorner = size / 6.f;
    const float faceInset = size / 9.f;

    RenderColorQuadARGB(left, top + rimCorner, size, size - 2.f * rimCorner, colors.Edge);
    RenderColorQuadARGB(left + rimCorner, top, size - 2.f * rimCorner, size, colors.Edge);

    const float faceSize = size - 2.f * faceInset;
    RenderColorQuadARGB(left + faceInset, top + faceInset + faceInset, faceSize, faceSize - 2.f * faceInset,
                        colors.Face);
    RenderColorQuadARGB(left + faceInset + faceInset, top + faceInset, faceSize - 2.f * faceInset, faceSize,
                        colors.Face);
    EndRenderColor();

    // The letter is written in the colour of the rim, which is the darkest of the two and the only
    // one the face is guaranteed to be read against.
    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    g_pRenderText->SetTextColor(static_cast<int>((colors.Edge >> 16) & 0xFFu),
                                static_cast<int>((colors.Edge >> 8) & 0xFFu), static_cast<int>(colors.Edge & 0xFFu),
                                255);
    g_pRenderText->RenderText(rect.left, rect.top + COIN_GLYPH_TOP, BankUI::GetCoinGlyph(currency),
                              rect.right - rect.left, 0, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIBankWindow::RenderItemPicture(const ITEM& item, int left, int top, int width, int height,
                                                   int margin)
{
    // Every box is the same size, so the picture keeps the proportions of the item and is drawn as
    // large as it can be inside its box.
    const ITEM_ATTRIBUTE* pItemAttr = &ItemAttribute[item.Type];
    const int columns = pItemAttr->Width > 0 ? pItemAttr->Width : 1;
    const int rows = pItemAttr->Height > 0 ? pItemAttr->Height : 1;
    const int room = std::max(1, std::min(width, height) - margin);
    const float scale = static_cast<float>(room) / static_cast<float>(std::max(columns, rows));
    const float pictureWidth = columns * scale;
    const float pictureHeight = rows * scale;

    RenderItem3D(left + (width - pictureWidth) / 2.f, top + (height - pictureHeight) / 2.f, pictureWidth, pictureHeight,
                 item.Type, item.Level, item.ExcellentFlags, item.AncientDiscriminator, false);
}

void SEASON3B::CNewUIBankWindow::RebuildOfferItems()
{
    ClearOfferItems();

    const auto& offers = Net::Bank::Store::Instance().GetOffers();
    m_offerItems.reserve(offers.size());

    for (const Net::Bank::Offer& offer : offers)
    {
        // An offer of a currency carries no item, and neither does one whose item the server could
        // not write; both draw their amount where a picture would have stood.
        const bool carriesItem =
            offer.Kind == Net::Bank::OfferKind::Items &&
            std::any_of(offer.ItemData.begin(), offer.ItemData.end(), [](BYTE value) { return value != 0; });

        m_offerItems.push_back(carriesItem ? g_pNewItemMng->CreateItem(offer.ItemData) : nullptr);
    }

    if (m_selectedOffer >= static_cast<int>(offers.size()))
    {
        // The page which arrived is shorter than the one the pick was made on.
        m_selectedOffer = -1;
    }

    m_offerGeneration = Net::Bank::Store::Instance().GetOfferGeneration();
}

void SEASON3B::CNewUIBankWindow::ClearOfferItems()
{
    for (ITEM* pItem : m_offerItems)
    {
        if (pItem != nullptr && g_pNewItemMng != nullptr)
        {
            g_pNewItemMng->DeleteItem(pItem);
        }
    }

    m_offerItems.clear();
}

bool SEASON3B::CNewUIBankWindow::IsOwnOffer(const Net::Bank::Offer& offer)
{
    // The server lists the character which made the offer, which is the name this client plays.
    return Hero != nullptr && !offer.SellerName.empty() && offer.SellerName == Hero->ID;
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

const wchar_t* SEASON3B::CNewUIBankWindow::GetCurrencyShortName(Net::Bank::Currency currency)
{
    // A ledger row names its currency beside the amount, where the full name of a jewel would push
    // the digits out of the column.
    switch (currency)
    {
    case Net::Bank::Currency::Zen:
        return I18N::Game::BankCurrencyShortZen;
    case Net::Bank::Currency::WCoinC:
        return I18N::Game::BankCurrencyShortWcoinC;
    case Net::Bank::Currency::WCoinP:
        return I18N::Game::BankCurrencyShortWcoinP;
    case Net::Bank::Currency::GoblinPoints:
        return I18N::Game::BankCurrencyShortGoblinPoints;
    case Net::Bank::Currency::JewelOfBless:
        return I18N::Game::BankCurrencyShortJewelOfBless;
    case Net::Bank::Currency::JewelOfSoul:
        return I18N::Game::BankCurrencyShortJewelOfSoul;
    case Net::Bank::Currency::JewelOfLife:
        return I18N::Game::BankCurrencyShortJewelOfLife;
    case Net::Bank::Currency::JewelOfCreation:
        return I18N::Game::BankCurrencyShortJewelOfCreation;
    case Net::Bank::Currency::JewelOfChaos:
        return I18N::Game::BankCurrencyShortJewelOfChaos;
    default:
        return L"";
    }
}

const wchar_t* SEASON3B::CNewUIBankWindow::GetLedgerTypeName(Net::Bank::LedgerEntryType type)
{
    switch (type)
    {
    case Net::Bank::LedgerEntryType::Deposit:
        return I18N::Game::BankLedgerDeposit;
    case Net::Bank::LedgerEntryType::Withdrawal:
        return I18N::Game::BankLedgerWithdrawal;
    case Net::Bank::LedgerEntryType::TransferOut:
        return I18N::Game::BankLedgerTransferOut;
    case Net::Bank::LedgerEntryType::TransferIn:
        return I18N::Game::BankLedgerTransferIn;
    case Net::Bank::LedgerEntryType::Fee:
        return I18N::Game::BankLedgerFee;
    case Net::Bank::LedgerEntryType::MarketListed:
        return I18N::Game::BankLedgerMarketListed;
    case Net::Bank::LedgerEntryType::MarketSale:
        return I18N::Game::BankLedgerMarketSale;
    case Net::Bank::LedgerEntryType::MarketPurchase:
        return I18N::Game::BankLedgerMarketPurchase;
    case Net::Bank::LedgerEntryType::MarketReturn:
        return I18N::Game::BankLedgerMarketReturn;
    case Net::Bank::LedgerEntryType::Correction:
        return I18N::Game::BankLedgerCorrection;
    default:
        return L"";
    }
}

void SEASON3B::CNewUIBankWindow::FormatLedgerTime(int64_t timestamp, wchar_t* text, size_t textLength)
{
    if (text == nullptr || textLength == 0)
    {
        return;
    }

    text[0] = L'\0';

    const std::time_t seconds = static_cast<std::time_t>(timestamp);
    std::tm local{};
#ifdef _WIN32
    if (localtime_s(&local, &seconds) != 0)
    {
        return;
    }
#else
    if (localtime_r(&seconds, &local) == nullptr)
    {
        return;
    }
#endif

    if (std::wcsftime(text, textLength, L"%d/%m %H:%M", &local) == 0)
    {
        text[0] = L'\0';
    }
}

const wchar_t* SEASON3B::CNewUIBankWindow::GetLedgerDetail(const Net::Bank::LedgerEntry& entry)
{
    // What moved is worth more than who moved it, so a description wins when the entry has both -
    // a sale names the item in the row and the buyer below it.
    if (!entry.Description.empty())
    {
        return entry.Description.c_str();
    }

    return entry.CounterpartyName.c_str();
}
