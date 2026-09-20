//*****************************************************************************
// File: BankCurrencyInfo.cpp
//*****************************************************************************

#include "stdafx.h"

#include "UI/NewUI/Inventory/BankCurrencyInfo.h"

#include <algorithm>
#include <array>
#include <limits>

namespace
{
constexpr int64_t ONE_MILLION = 1000000;
constexpr int64_t ONE_BILLION = 1000000000;

/// <summary>
/// What zen is offered at a click. The last of them is above what a character may carry, so it is
/// the button which moves a whole bank of zen out in one go rather than a round number.
/// </summary>
constexpr std::array<int64_t, 5> ZEN_PRESETS{1000000, 10000000, 100000000, 1000000000, 10000000000};

/// <summary>What a balance of the cash shop is offered at a click.</summary>
constexpr std::array<int64_t, 4> CASH_PRESETS{10, 100, 1000, 10000};

/// <summary>
/// What a jewel is offered at a click: the small round numbers jewels are counted in, rather than
/// the millions the money is.
/// </summary>
constexpr std::array<int64_t, 4> JEWEL_PRESETS{1, 5, 10, 30};
} // namespace

bool SEASON3B::BankUI::IsJewelCurrency(Net::Bank::Currency currency)
{
    return GetJewelItemType(currency) >= 0;
}

bool SEASON3B::BankUI::IsCashCurrency(Net::Bank::Currency currency)
{
    return currency == Net::Bank::Currency::WCoinC || currency == Net::Bank::Currency::WCoinP ||
           currency == Net::Bank::Currency::GoblinPoints;
}

short SEASON3B::BankUI::GetJewelItemType(Net::Bank::Currency currency)
{
    switch (currency)
    {
    case Net::Bank::Currency::JewelOfBless:
        return ITEM_JEWEL_OF_BLESS;
    case Net::Bank::Currency::JewelOfSoul:
        return ITEM_JEWEL_OF_SOUL;
    case Net::Bank::Currency::JewelOfLife:
        return ITEM_JEWEL_OF_LIFE;
    case Net::Bank::Currency::JewelOfCreation:
        return ITEM_JEWEL_OF_CREATION;
    case Net::Bank::Currency::JewelOfChaos:
        return ITEM_JEWEL_OF_CHAOS;
    default:
        return -1;
    }
}

SEASON3B::BankUI::CoinColors SEASON3B::BankUI::GetCoinColors(Net::Bank::Currency currency)
{
    switch (currency)
    {
    case Net::Bank::Currency::WCoinC:
        return {0xFFC9CDD4u, 0xFF5C6672u};
    case Net::Bank::Currency::WCoinP:
        return {0xFFB48AD8u, 0xFF543A70u};
    case Net::Bank::Currency::GoblinPoints:
        return {0xFF8FCB63u, 0xFF3A5A22u};
    default:
        // Zen, in the gold the rest of this window is trimmed with.
        return {0xFFE8C25Au, 0xFF8A6415u};
    }
}

const wchar_t* SEASON3B::BankUI::GetCoinGlyph(Net::Bank::Currency currency)
{
    switch (currency)
    {
    case Net::Bank::Currency::WCoinC:
        return L"C";
    case Net::Bank::Currency::WCoinP:
        return L"P";
    case Net::Bank::Currency::GoblinPoints:
        return L"G";
    default:
        return L"Z";
    }
}

std::span<const int64_t> SEASON3B::BankUI::GetAmountPresets(Net::Bank::Currency currency)
{
    if (IsJewelCurrency(currency))
    {
        return JEWEL_PRESETS;
    }

    if (IsCashCurrency(currency))
    {
        return CASH_PRESETS;
    }

    return ZEN_PRESETS;
}

SEASON3B::BankUI::PresetLabel SEASON3B::BankUI::DescribePreset(int64_t amount)
{
    if (amount >= ONE_BILLION && amount % ONE_BILLION == 0)
    {
        return {PresetUnit::Billion, static_cast<int>(amount / ONE_BILLION)};
    }

    if (amount >= ONE_MILLION && amount % ONE_MILLION == 0)
    {
        return {PresetUnit::Million, static_cast<int>(amount / ONE_MILLION)};
    }

    return {PresetUnit::Plain, static_cast<int>(std::min<int64_t>(amount, std::numeric_limits<int>::max()))};
}

int64_t SEASON3B::BankUI::GetDepositLimit(Net::Bank::Currency currency, int64_t carried)
{
    if (IsCashCurrency(currency))
    {
        // The wallet of the account never reaches this client, so there is nothing to cut down to.
        return UnknownLimit;
    }

    return std::max<int64_t>(0, carried);
}

int64_t SEASON3B::BankUI::GetWithdrawLimit(Net::Bank::Currency currency, int64_t balance, int64_t carriedZen,
                                           int freeInventorySlots)
{
    const int64_t held = std::max<int64_t>(0, balance);

    if (currency == Net::Bank::Currency::Zen)
    {
        // Zen sits on the character, which may not carry more than the server allows.
        const int64_t room = std::max<int64_t>(0, MaximumCarriedZen - std::max<int64_t>(0, carriedZen));
        return std::min(held, room);
    }

    if (IsJewelCurrency(currency))
    {
        // Every jewel which comes out needs a box of its own to appear in.
        return std::min<int64_t>(held, std::max(0, freeInventorySlots));
    }

    return held;
}

int64_t SEASON3B::BankUI::ClampAmount(int64_t wanted, int64_t limit)
{
    if (wanted <= 0)
    {
        return 0;
    }

    if (limit == UnknownLimit)
    {
        return wanted;
    }

    return std::min(wanted, std::max<int64_t>(0, limit));
}
