// <copyright file="BankProtocol.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include "stdafx.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

/// <summary>
/// The wire values and the received shapes of the bank of the account and of the market between
/// players (packet code 0xFB). None of this is part of the original protocol.
/// </summary>
namespace Net::Bank
{
/// <summary>The packet code which every request and every answer of the bank uses.</summary>
constexpr BYTE PacketCode = 0xFB;

/// <summary>
/// The value the server uses for the bank in the npc window and in the item list, far above the
/// windows the original client knows.
/// </summary>
constexpr BYTE WindowValue = 200;

/// <summary>The length of the identifier of a market offer, as it travels.</summary>
constexpr int ListingIdLength = 16;

/// <summary>The length of the raw item of a market offer, as it travels.</summary>
constexpr int OfferItemDataLength = 16;

/// <summary>The value of a filter of the market which lets everything through.</summary>
constexpr BYTE AnyFilter = 255;

/// <summary>
/// What an offered item is, as a player searches for it. Mirrors the MarketItemCategory of the
/// server, which works it out when the offer is made.
/// </summary>
enum class ItemCategory : BYTE
{
    None = 0,
    Armor = 1,
    Weapon = 2,
    Shield = 3,
    Wings = 4,
    Jewelry = 5,
    Other = 6,
};

/// <summary>A kind of value which the bank holds. Mirrors the BankCurrency of the server.</summary>
enum class Currency : BYTE
{
    Zen = 0,
    WCoinC = 1,
    WCoinP = 2,
    GoblinPoints = 3,
    JewelOfBless = 4,
    JewelOfSoul = 5,
    JewelOfLife = 6,
    JewelOfCreation = 7,
    JewelOfChaos = 8,
    Count = 9,
};

/// <summary>Which request an answer of the server belongs to.</summary>
enum class Operation : BYTE
{
    Deposit = 0,
    Withdrawal = 1,
    MarketRegister = 4,
    MarketBuy = 5,
    MarketCancel = 6,
};

/// <summary>How a request ended.</summary>
enum class ResultCode : BYTE
{
    Success = 0,
    Failed = 1,
    BankNotOpened = 2,
    InvalidAmount = 3,
    NotEnoughValue = 4,
    BalanceLimitReached = 5,
    InventoryFull = 6,
    BankStorageFull = 7,
    ReceiverNotFound = 8,
    ReceiverIsSelf = 9,
    TransferLimitReached = 10,
    CharacterLevelTooLow = 11,
    CurrencyNotTransferable = 12,
    TransfersDisabled = 13,
    MarketDisabled = 14,
    OfferGone = 15,
    TooManyOffers = 16,
    ItemNotFound = 17,
    ItemNotTradable = 18,
    OwnOffer = 19,
};

/// <summary>What a market offer offers.</summary>
enum class OfferKind : BYTE
{
    Items = 0,
    Currency = 1,
};

/// <summary>Why a ledger entry was booked.</summary>
enum class LedgerEntryType : BYTE
{
    Deposit = 0,
    Withdrawal = 1,
    TransferOut = 2,
    TransferIn = 3,
    Fee = 4,
    MarketListed = 5,
    MarketSale = 6,
    MarketPurchase = 7,
    MarketReturn = 8,
    Correction = 9,
};

/// <summary>One offer of the market, as the client keeps it after it arrived.</summary>
struct Offer
{
    std::array<BYTE, ListingIdLength> ListingId{};
    std::wstring SellerName;
    OfferKind Kind = OfferKind::Items;
    Currency PriceCurrency = Currency::Zen;
    int64_t PriceAmount = 0;
    Currency OfferedCurrency = Currency::Zen;
    int64_t OfferedAmount = 0;
    std::array<BYTE, OfferItemDataLength> ItemData{};
    std::wstring OfferName;
};

/// <summary>One booked movement of the bank, as the client keeps it after it arrived.</summary>
struct LedgerEntry
{
    int64_t Timestamp = 0;
    LedgerEntryType Type = LedgerEntryType::Correction;
    Currency EntryCurrency = Currency::Zen;
    int64_t Amount = 0;
    int64_t BalanceAfter = 0;
    std::wstring CounterpartyName;
    std::wstring Description;
};
} // namespace Net::Bank
