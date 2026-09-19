// <copyright file="BankPackets.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "stdafx.h"
#include "BankPackets.h"
#include "BankStore.h"
#include "Data/Translation/MultiLanguage.h"

#include <algorithm>
#include <cstring>

namespace Net::Bank
{
namespace
{
/// <summary>The sub codes the server answers with.</summary>
constexpr BYTE SubCodeBalances = 0x01;
constexpr BYTE SubCodeOperationResult = 0x02;
constexpr BYTE SubCodeOffers = 0x03;
constexpr BYTE SubCodeLedger = 0x04;

/// <summary>The byte lengths of the fields which repeat inside the list packets.</summary>
constexpr int BalanceEntryLength = 9;
constexpr int OfferLength = 121;
constexpr int LedgerEntryLength = 96;
constexpr int NameLength = 10;
constexpr int OfferNameLength = 60;
constexpr int DescriptionLength = 60;

/// <summary>The longest string this file converts, plus the terminator.</summary>
constexpr int MaxConvertedLength = 64;

/// <summary>Reads a signed 64 bit number which the server wrote in little endian order.</summary>
int64_t ReadInt64(const std::span<const BYTE> data, const int offset)
{
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
    {
        value |= static_cast<uint64_t>(data[offset + i]) << (8 * i);
    }

    return static_cast<int64_t>(value);
}

/// <summary>Reads a utf-8 field of a fixed length into a wide string.</summary>
std::wstring ReadString(const std::span<const BYTE> data, const int offset, const int length)
{
    char source[MaxConvertedLength] = {};
    const int copied = std::min(length, MaxConvertedLength - 1);
    std::memcpy(source, data.data() + offset, copied);

    wchar_t target[MaxConvertedLength] = {};
    CMultiLanguage::ConvertFromUtf8(target, source, copied);
    return std::wstring(target);
}

/// <summary>Gets the index at which the fields of the packet start.</summary>
int GetFieldOffset(const std::span<const BYTE> packet)
{
    // C1 and C3 carry a one byte length, C2 and C4 a two byte one; the sub code follows the code.
    return (packet[0] % 2 == 1) ? 4 : 5;
}

void ReadBalances(const std::span<const BYTE> packet, const int offset)
{
    const int count = packet[offset];
    if (static_cast<int>(packet.size()) < offset + 1 + count * BalanceEntryLength)
    {
        return;
    }

    for (int i = 0; i < count; ++i)
    {
        const int entry = offset + 1 + i * BalanceEntryLength;
        Store::Instance().SetBalance(static_cast<Currency>(packet[entry]), ReadInt64(packet, entry + 1));
    }
}

void ReadOperationResult(const std::span<const BYTE> packet, const int offset)
{
    if (static_cast<int>(packet.size()) < offset + 2)
    {
        return;
    }

    Store::Instance().SetLastResult(static_cast<Operation>(packet[offset]), static_cast<ResultCode>(packet[offset + 1]));
}

void ReadOffers(const std::span<const BYTE> packet, const int offset)
{
    if (static_cast<int>(packet.size()) < offset + 3)
    {
        return;
    }

    const BYTE page = packet[offset];
    const BYTE pageCount = packet[offset + 1];
    const int count = packet[offset + 2];
    const int first = offset + 3;
    if (static_cast<int>(packet.size()) < first + count * OfferLength)
    {
        return;
    }

    std::vector<Offer> offers;
    offers.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        const int start = first + i * OfferLength;
        Offer offer;
        std::memcpy(offer.ListingId.data(), packet.data() + start, ListingIdLength);
        offer.SellerName = ReadString(packet, start + 16, NameLength);
        offer.Kind = static_cast<OfferKind>(packet[start + 26]);
        offer.PriceCurrency = static_cast<Currency>(packet[start + 27]);
        offer.PriceAmount = ReadInt64(packet, start + 28);
        offer.OfferedCurrency = static_cast<Currency>(packet[start + 36]);
        offer.OfferedAmount = ReadInt64(packet, start + 37);
        std::memcpy(offer.ItemData.data(), packet.data() + start + 45, OfferItemDataLength);
        offer.OfferName = ReadString(packet, start + 61, OfferNameLength);
        offers.push_back(std::move(offer));
    }

    Store::Instance().SetOffers(std::move(offers), page, pageCount);
}

void ReadLedger(const std::span<const BYTE> packet, const int offset)
{
    if (static_cast<int>(packet.size()) < offset + 2)
    {
        return;
    }

    const BYTE page = packet[offset];
    const int count = packet[offset + 1];
    const int first = offset + 2;
    if (static_cast<int>(packet.size()) < first + count * LedgerEntryLength)
    {
        return;
    }

    std::vector<LedgerEntry> entries;
    entries.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        const int start = first + i * LedgerEntryLength;
        LedgerEntry entry;
        entry.Timestamp = ReadInt64(packet, start);
        entry.Type = static_cast<LedgerEntryType>(packet[start + 8]);
        entry.EntryCurrency = static_cast<Currency>(packet[start + 9]);
        entry.Amount = ReadInt64(packet, start + 10);
        entry.BalanceAfter = ReadInt64(packet, start + 18);
        entry.CounterpartyName = ReadString(packet, start + 26, NameLength);
        entry.Description = ReadString(packet, start + 36, DescriptionLength);
        entries.push_back(std::move(entry));
    }

    Store::Instance().SetLedger(std::move(entries), page);
}
} // namespace

void HandlePacket(const std::span<const BYTE> packet)
{
    const int offset = GetFieldOffset(packet);
    if (static_cast<int>(packet.size()) <= offset)
    {
        return;
    }

    const BYTE subCode = packet[offset - 1];
    switch (subCode)
    {
    case SubCodeBalances:
        ReadBalances(packet, offset);
        break;
    case SubCodeOperationResult:
        ReadOperationResult(packet, offset);
        break;
    case SubCodeOffers:
        ReadOffers(packet, offset);
        break;
    case SubCodeLedger:
        ReadLedger(packet, offset);
        break;
    default:
        break;
    }
}
} // namespace Net::Bank
