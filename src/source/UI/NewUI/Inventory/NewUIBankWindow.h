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

#include <string>

namespace SEASON3B
{
/// <summary>
/// The bank of the account: the items it keeps, what it holds of every currency, and the market
/// on which the offers of all players can be bought.
/// </summary>
/// <remarks>
/// Not part of the original client. The item boxes are an ordinary inventory control on the
/// storage type of the bank, so moving an item in or out is the same drag and drop - and the same
/// packet - as with the vault. Everything else is drawn from what the server last sent, which
/// <see cref="Net::Bank::Store"/> keeps.
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

    /// <summary>Gets the control which draws the item boxes of the bank.</summary>
    CNewUIInventoryCtrl* GetInventoryCtrl() const
    {
        return m_pInventoryCtrl;
    }

    /// <summary>Puts an item of the bank into its box, while the list is arriving.</summary>
    bool InsertItem(int iIndex, std::span<const BYTE> pbyItemPacket);

    /// <summary>Puts an item into its box after the server moved it there.</summary>
    void ProcessToReceiveBankItems(int nIndex, std::span<const BYTE> pbyItemPacket);

    /// <summary>Empties every box, for a list which is about to arrive.</summary>
    void DeleteAllItems();

    /// <summary>Asks the server for a page of the market.</summary>
    void RequestMarketPage(BYTE page);

    /// <summary>Tells the server that the dialog was closed, so the player leaves the npc.</summary>
    void ClosingProcess();

    /// <summary>
    /// Offers the picked item at the price the player typed.
    /// </summary>
    /// <param name="price">The price, in the currency which is selected.</param>
    void FinishOffer(int64_t price);

    /// <summary>
    /// Takes the account a transfer is addressed to, and either sends the item at once or asks
    /// how much of the selected currency should go.
    /// </summary>
    /// <param name="receiverName">The character or login name the player typed.</param>
    void SetTransferReceiver(const wchar_t* receiverName);

    /// <summary>
    /// Sends the transfer of the selected currency with the amount the player typed.
    /// </summary>
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
        BTN_OFFER,
        BTN_SEND_ITEM,
        BTN_SEND_VALUE,
        MAX_BTN
    };

    static constexpr float BANK_WIDTH = 190.0f;

    // Taller than the vault, because the balances and three rows of buttons live below the item
    // boxes instead of replacing a row of them.
    static constexpr float BANK_HEIGHT = 560.0f;

    /// <summary>The boxes of the bank, which has to match the rows the server configured.</summary>
    static constexpr int BANK_COLUMNS = 8;
    static constexpr int BANK_ROWS = 15;

    void LoadImages();
    void UnloadImages();

    void InitButton(CNewUIButton* pButton, int x, int y, const wchar_t* caption);
    void ShowRefusedRequest();
    void OpenPendingInput();
    void RenderFrame();
    void RenderStoragePage();
    void RenderMarketPage();
    bool ProcessButtons();
    bool ProcessStorageButtons();
    bool ProcessMarketButtons();
    void ProcessSelection();
    void MoveSelectedCurrency(bool deposit);
    void BuySelectedOffer();
    void CancelSelectedOffer();
    bool RequireSelectedItem();

    /// <summary>Gets the name of a currency as it is listed.</summary>
    static const wchar_t* GetCurrencyName(Net::Bank::Currency currency);

    CNewUIManager* m_pNewUIMng;
    CNewUIInventoryCtrl* m_pInventoryCtrl;
    POINT m_Pos;

    CNewUIButton m_abtn[MAX_BTN];

    Page m_page;
    int m_selectedCurrency;
    int m_selectedOffer;

    /// <summary>The box the player picked with the right mouse button; -1 when none.</summary>
    int m_selectedSlot;

    PendingInput m_pendingInput;

    /// <summary>Whether the transfer being asked about carries the picked item or a currency.</summary>
    bool m_transferCarriesItem;

    std::wstring m_transferReceiver;
};
} // namespace SEASON3B
