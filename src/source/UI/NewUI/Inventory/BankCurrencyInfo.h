//*****************************************************************************
// File: BankCurrencyInfo.h
//*****************************************************************************

#pragma once

#include "Network/Server/BankProtocol.h"

#include <cstdint>
#include <span>

/// <summary>
/// What the bank dialog needs to know about a currency before it draws it or asks for an amount of
/// it: which item it is worn as, which colour its coin is, which amounts are offered at a click,
/// and how much of it may actually move.
/// </summary>
/// <remarks>
/// Nothing here reads a global or draws anything, which is what lets the rules be tested on their
/// own. The window feeds it what it read from the character and from the store; the dialog only
/// asks it what to write in a box.
/// </remarks>
namespace SEASON3B::BankUI
{
/// <summary>
/// The answer for a limit the client cannot work out, which is not the same as a limit of nothing.
/// </summary>
/// <remarks>
/// The balances of the cash shop sit on the account and never reach this client, so a deposit of
/// them has no limit to offer. The server treats the amount as an upper bound and moves what the
/// player actually has, so a number which is too large costs nothing - but a button which claims
/// to move everything would be a lie, and is therefore not shown.
/// </remarks>
inline constexpr int64_t UnknownLimit = -1;

/// <summary>
/// The most zen a character may carry, which is what the server's MaximumInventoryMoney is set to.
/// </summary>
inline constexpr int64_t MaximumCarriedZen = 2000000000;

/// <summary>
/// Everything the dialog which asks for an amount has to draw, and nothing it has to work out.
/// </summary>
/// <remarks>
/// The rules stay in the bank window: what the character carries, what the bank holds and how much
/// room the inventory has left are read in one place and the dialog is handed the answer. It
/// therefore knows nothing about currencies, and a rule which changes changes in one place.
/// </remarks>
struct AmountRequest
{
    /// <summary>What the amount which is being asked for is going to do.</summary>
    enum class Purpose
    {
        /// <summary>It goes from the character into the bank.</summary>
        Deposit,

        /// <summary>It comes out of the bank onto the character.</summary>
        Withdraw,

        /// <summary>It leaves the bank onto the market, where it waits for a buyer.</summary>
        Offer,
    };

    Purpose What = Purpose::Deposit;

    Net::Bank::Currency Currency = Net::Bank::Currency::Zen;

    /// <summary>
    /// The name of the currency, as the dialog writes it in its heading.
    /// </summary>
    /// <remarks>
    /// Handed over rather than looked up, so that the dialog needs no table of its own and stays
    /// out of the question of what a currency is called.
    /// </remarks>
    const wchar_t* CurrencyName = nullptr;

    /// <summary>What the bank holds of the currency.</summary>
    int64_t Balance = 0;

    /// <summary>The most which may move, or <see cref="UnknownLimit"/> when it cannot be known.</summary>
    int64_t Limit = 0;

    /// <summary>How many boxes of the inventory are empty; only a jewel is held back by them.</summary>
    int FreeInventorySlots = 0;
};

/// <summary>Gets whether the currency is one the bank counts as items of the inventory.</summary>
bool IsJewelCurrency(Net::Bank::Currency currency);

/// <summary>Gets whether the currency is a balance of the cash shop, which this client never sees.</summary>
bool IsCashCurrency(Net::Bank::Currency currency);

/// <summary>
/// Gets the item one unit of the currency is, so its picture can be drawn beside its name.
/// </summary>
/// <param name="currency">The currency.</param>
/// <returns>The item type, or -1 when the currency is a plain number and has no item.</returns>
short GetJewelItemType(Net::Bank::Currency currency);

/// <summary>The two colours the coin of a currency which is a plain number is drawn out of.</summary>
struct CoinColors
{
    /// <summary>The face of the coin.</summary>
    DWORD Face;

    /// <summary>The rim around it, and the colour its letter is written in.</summary>
    DWORD Edge;
};

/// <summary>
/// Gets the colours the coin of a currency which is a plain number is drawn in.
/// </summary>
/// <remarks>
/// The coin is drawn, not loaded: the client has no artwork for a coin of any size. What it has
/// under Interface are the 170x24 strips the inventory and the shops write a balance on, which is
/// a bar and not a picture of a currency. The four coins are therefore told apart by their colour
/// and by the letter on them, both of which the window draws out of the same plain quads and text
/// it draws everything else with.
/// </remarks>
CoinColors GetCoinColors(Net::Bank::Currency currency);

/// <summary>
/// Gets the letter which stands on the coin of a currency which is a plain number.
/// </summary>
/// <remarks>
/// Not the first letter of the name, which is translated: two of the names begin with the same
/// letter in English and a third begins with one no coin could carry in Vietnamese. These four are
/// symbols and stay what they are in every language.
/// </remarks>
const wchar_t* GetCoinGlyph(Net::Bank::Currency currency);

/// <summary>Gets the amounts which are offered at a click for the given currency.</summary>
std::span<const int64_t> GetAmountPresets(Net::Bank::Currency currency);

/// <summary>Which unit an offered amount is written in, so it fits on a button.</summary>
enum class PresetUnit
{
    /// <summary>The amount itself.</summary>
    Plain,
    Million,
    Billion,
};

/// <summary>An offered amount, as it is written on its button.</summary>
struct PresetLabel
{
    PresetUnit Unit;

    /// <summary>How many of the unit, which is the amount itself when the unit is plain.</summary>
    int Count;
};

/// <summary>Gets how an offered amount is written, which is the largest unit it fills exactly.</summary>
PresetLabel DescribePreset(int64_t amount);

/// <summary>
/// Gets how much of a currency may go into the bank.
/// </summary>
/// <param name="currency">The currency.</param>
/// <param name="carried">What the character has of it: its zen, or its jewels of that kind.</param>
/// <returns>The limit, or <see cref="UnknownLimit"/> when the client cannot know it.</returns>
int64_t GetDepositLimit(Net::Bank::Currency currency, int64_t carried);

/// <summary>
/// Gets how much of a currency may come out of the bank.
/// </summary>
/// <param name="currency">The currency.</param>
/// <param name="balance">What the bank holds of it.</param>
/// <param name="carriedZen">The zen the character carries; read for zen and ignored otherwise.</param>
/// <param name="freeInventorySlots">
/// The empty boxes of the inventory; read for jewels and ignored otherwise.
/// </param>
/// <returns>The limit, which is never below zero.</returns>
int64_t GetWithdrawLimit(Net::Bank::Currency currency, int64_t balance, int64_t carriedZen, int freeInventorySlots);

/// <summary>
/// Cuts an amount down to a limit.
/// </summary>
/// <param name="wanted">The amount which was asked for.</param>
/// <param name="limit">The limit, or <see cref="UnknownLimit"/> when there is none to apply.</param>
/// <returns>The amount which may be sent.</returns>
int64_t ClampAmount(int64_t wanted, int64_t limit);
} // namespace SEASON3B::BankUI
