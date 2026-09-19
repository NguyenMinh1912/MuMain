//*****************************************************************************
// File: NewUIBankWindow.h
//*****************************************************************************

#pragma once

#include "Network/Server/BankProtocol.h"
#include "UI/NewUI/Dialogs/NewUIMessageBox.h"
#include "UI/NewUI/Inventory/NewUIInventoryCtrl.h"
#include "UI/NewUI/Inventory/NewUIMyInventory.h"
#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

#include <map>
#include <string>
#include <vector>

namespace SEASON3B
{
/// <summary>
/// The bank of the account: what it holds of every currency, the items it keeps, and the market on
/// which the offers of all players can be bought.
/// </summary>
/// <remarks>
/// Not part of the original client, and not an npc dialog: the player opens it from a button of
/// his inventory. The item boxes are an ordinary inventory control, so moving an item in or out is
/// the same drag and drop - and the same packet - as with the vault. The bank holds more boxes
/// than fit on screen, so the control shows one page at a time and the window keeps what the
/// server sent for the pages which are not shown.
/// </remarks>
class CNewUIBankWindow : public CNewUIObj
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

    CNewUIBankWindow();
    ~CNewUIBankWindow() override;

    bool Create(CNewUIManager* pNewUIMng, int x, int y);
    void Release();

    void SetPos(int x, int y);

    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;

    float GetLayerDepth() override; //. 2.2f

    /// <summary>Gets the control which draws the item boxes of the shown page.</summary>
    CNewUIInventoryCtrl* GetInventoryCtrl() const
    {
        return m_pInventoryCtrl;
    }

    /// <summary>Tells the server that the dialog was opened, and asks for what it holds.</summary>
    void OpeningProcess();

    /// <summary>Tells the server that the dialog was closed.</summary>
    void ClosingProcess();

    /// <summary>Keeps an item of the bank, and draws it when its page is the shown one.</summary>
    bool InsertItem(int iIndex, std::span<const BYTE> pbyItemPacket);

    /// <summary>Puts an item into its box after the server moved it there.</summary>
    void ProcessToReceiveBankItems(int nIndex, std::span<const BYTE> pbyItemPacket);

    /// <summary>Forgets every item, for a list which is about to arrive.</summary>
    void DeleteAllItems();

    /// <summary>
    /// Takes the item out of the boxes after the server confirmed the move which the right mouse
    /// button asked for.
    /// </summary>
    void ProcessAutoMoveSuccess();

    /// <summary>Asks the server for a page of the market.</summary>
    void RequestMarketPage(BYTE page);

    /// <summary>Offers the picked item at the price the player typed.</summary>
    /// <param name="price">The price, in the currency which is selected.</param>
    void FinishOffer(int64_t price);

    /// <summary>
    /// Takes the account a transfer is addressed to, and either sends the item at once or asks how
    /// much of the selected currency should go.
    /// </summary>
    /// <param name="receiverName">The character or login name the player typed.</param>
    void SetTransferReceiver(const wchar_t* receiverName);

    /// <summary>Sends the transfer of the selected currency with the amount the player typed.</summary>
    /// <param name="amount">The amount the receiver gets.</param>
    void FinishValueTransfer(int64_t amount);

    /// <summary>Forgets a half finished dialog, because the player cancelled it.</summary>
    void CancelPendingInput();

private:
    /// <summary>Which half of the bank the window is showing.</summary>
    enum class Page
    {
        Storage,
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
        Receiver,
        Amount,
    };

    enum BANK_BUTTON
    {
        BTN_PAGE = 0,
        BTN_PREV,
        BTN_NEXT,
        BTN_DEPOSIT,
        BTN_WITHDRAW,
        BTN_SEND_VALUE,
        BTN_OFFER,
        BTN_SEND_ITEM,
        MAX_BTN
    };

    static constexpr float BANK_WIDTH = 190.0f;
    static constexpr float BANK_HEIGHT = 470.0f;

    /// <summary>The squares of one page of the item boxes.</summary>
    static constexpr int BANK_COLUMNS = 8;
    static constexpr int BANK_PAGE_ROWS = 5;
    static constexpr int ITEMS_PER_PAGE = BANK_COLUMNS * BANK_PAGE_ROWS;

    /// <summary>
    /// How many boxes the bank has in total, which has to match the rows the server is configured
    /// with (its default is 15 rows of 8).
    /// </summary>
    static constexpr int BANK_TOTAL_ROWS = 15;
    static constexpr int BANK_TOTAL_SLOTS = BANK_COLUMNS * BANK_TOTAL_ROWS;
    static constexpr int ITEM_PAGE_COUNT = BANK_TOTAL_SLOTS / ITEMS_PER_PAGE;

    void LoadImages();
    void UnloadImages();

    void InitButton(CNewUIButton* pButton, const wchar_t* caption);
    void ShowRefusedRequest();
    void OpenPendingInput();
    void RenderFrame();
    void RenderStoragePage();
    void RenderBalances();
    void RenderMarketPage();
    bool ProcessButtons();
    bool ProcessStorageButtons();
    bool ProcessMarketButtons();
    void ProcessInventoryCtrl();
    void ProcessAutoMove();
    void ProcessOfferSelection();
    void ProcessBalanceSelection();

    /// <summary>Shows the given page of the item boxes, refilling the control from what is kept.</summary>
    void ShowItemPage(int page);

    /// <summary>
    /// Reads which box the item on the cursor came from, and puts that item back.
    /// </summary>
    /// <param name="slot">Receives the box of the bank the item was picked up from.</param>
    /// <returns><c>false</c> when nothing from the bank is on the cursor; the player was told.</returns>
    bool TakePickedBankSlot(int& slot);
    void MoveSelectedCurrency(bool deposit);
    void BuySelectedOffer();
    void CancelSelectedOffer();

    /// <summary>Gets the name of a currency as it is listed.</summary>
    static const wchar_t* GetCurrencyName(Net::Bank::Currency currency);

    CNewUIManager* m_pNewUIMng;
    CNewUIInventoryCtrl* m_pInventoryCtrl;
    POINT m_Pos;

    CNewUIButton m_abtn[MAX_BTN];

    Page m_page;
    int m_itemPage;
    int m_selectedCurrency;
    int m_selectedOffer;

    /// <summary>
    /// What the server sent for every box, by box number, so a page which is not shown is not lost.
    /// </summary>
    std::map<int, std::vector<BYTE>> m_storedItems;

    /// <summary>The box a dialog is about to act on; -1 when no dialog is pending.</summary>
    int m_pendingSlot;

    /// <summary>The item the right mouse button is moving out of the bank, while it is on its way.</summary>
    ITEM* m_autoMoveItem;

    PendingInput m_pendingInput;

    /// <summary>Whether the transfer being asked about carries the picked item or a currency.</summary>
    bool m_transferCarriesItem;

    std::wstring m_transferReceiver;
};
} // namespace SEASON3B
