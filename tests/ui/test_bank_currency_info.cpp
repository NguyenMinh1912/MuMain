#include "App/stdafx.h"

#include "UI/NewUI/Inventory/BankCurrencyInfo.h"

#include "doctest.h"

using namespace SEASON3B::BankUI;
using Net::Bank::Currency;

TEST_CASE("a currency knows what it is made of [ui][bank]")
{
    CHECK(IsJewelCurrency(Currency::JewelOfBless));
    CHECK(IsJewelCurrency(Currency::JewelOfChaos));
    CHECK_FALSE(IsJewelCurrency(Currency::Zen));
    CHECK_FALSE(IsJewelCurrency(Currency::WCoinC));

    CHECK(IsCashCurrency(Currency::WCoinC));
    CHECK(IsCashCurrency(Currency::WCoinP));
    CHECK(IsCashCurrency(Currency::GoblinPoints));
    CHECK_FALSE(IsCashCurrency(Currency::Zen));
    CHECK_FALSE(IsCashCurrency(Currency::JewelOfSoul));

    CHECK(GetJewelItemType(Currency::JewelOfSoul) == ITEM_JEWEL_OF_SOUL);
    CHECK(GetJewelItemType(Currency::Zen) == -1);
}

TEST_CASE("what may go into the bank [ui][bank]")
{
    // Zen and jewels are on the character, so the client knows exactly how much there is.
    CHECK(GetDepositLimit(Currency::Zen, 1250000000) == 1250000000);
    CHECK(GetDepositLimit(Currency::JewelOfSoul, 47) == 47);

    // A character which carries nothing may deposit nothing, which is a limit and not an unknown.
    CHECK(GetDepositLimit(Currency::Zen, 0) == 0);
    CHECK(GetDepositLimit(Currency::Zen, -5) == 0);

    // The wallet of the account never reaches this client.
    CHECK(GetDepositLimit(Currency::WCoinC, 0) == UnknownLimit);
    CHECK(GetDepositLimit(Currency::WCoinP, 5000) == UnknownLimit);
    CHECK(GetDepositLimit(Currency::GoblinPoints, 120) == UnknownLimit);
}

TEST_CASE("what may come out of the bank [ui][bank]")
{
    // Zen: whichever runs out first, the balance or the room the character has left for it.
    CHECK(GetWithdrawLimit(Currency::Zen, 5000000000, 1900000000, 0) == 100000000);
    CHECK(GetWithdrawLimit(Currency::Zen, 50000000, 0, 0) == 50000000);
    CHECK(GetWithdrawLimit(Currency::Zen, 5000000000, MaximumCarriedZen, 0) == 0);

    // Jewels: whichever runs out first, the balance or the empty boxes of the inventory.
    CHECK(GetWithdrawLimit(Currency::JewelOfSoul, 112, 0, 23) == 23);
    CHECK(GetWithdrawLimit(Currency::JewelOfSoul, 8, 0, 23) == 8);
    CHECK(GetWithdrawLimit(Currency::JewelOfSoul, 112, 0, 0) == 0);

    // The cash balances are counted by the bank alone, so nothing narrows them.
    CHECK(GetWithdrawLimit(Currency::WCoinC, 5000, 0, 0) == 5000);

    // A balance which somehow arrived negative is not a limit below nothing.
    CHECK(GetWithdrawLimit(Currency::WCoinC, -1, 0, 0) == 0);
}

TEST_CASE("an offered amount is cut down to what may move [ui][bank]")
{
    CHECK(ClampAmount(10000000000, 1250000000) == 1250000000);
    CHECK(ClampAmount(1000000, 1250000000) == 1000000);
    CHECK(ClampAmount(30, 23) == 23);

    // Nothing to cut down to: the amount is sent as it was typed, and the server decides.
    CHECK(ClampAmount(5000, UnknownLimit) == 5000);

    CHECK(ClampAmount(0, 100) == 0);
    CHECK(ClampAmount(-5, 100) == 0);
    CHECK(ClampAmount(100, 0) == 0);
}

TEST_CASE("an offered amount is written in the largest unit it fills [ui][bank]")
{
    CHECK(DescribePreset(1000000).Unit == PresetUnit::Million);
    CHECK(DescribePreset(1000000).Count == 1);

    CHECK(DescribePreset(100000000).Unit == PresetUnit::Million);
    CHECK(DescribePreset(100000000).Count == 100);

    CHECK(DescribePreset(1000000000).Unit == PresetUnit::Billion);
    CHECK(DescribePreset(1000000000).Count == 1);

    CHECK(DescribePreset(10000000000).Unit == PresetUnit::Billion);
    CHECK(DescribePreset(10000000000).Count == 10);

    CHECK(DescribePreset(30).Unit == PresetUnit::Plain);
    CHECK(DescribePreset(30).Count == 30);

    // Not a round million, so it is written out rather than rounded off.
    CHECK(DescribePreset(1500000).Unit == PresetUnit::Plain);
}

TEST_CASE("every currency offers amounts which suit it [ui][bank]")
{
    const auto zen = GetAmountPresets(Currency::Zen);
    REQUIRE_FALSE(zen.empty());
    CHECK(zen.front() == 1000000);

    const auto jewels = GetAmountPresets(Currency::JewelOfLife);
    REQUIRE_FALSE(jewels.empty());
    CHECK(jewels.front() == 1);

    const auto cash = GetAmountPresets(Currency::GoblinPoints);
    REQUIRE_FALSE(cash.empty());
    CHECK(cash.front() == 10);
}
