// <copyright file="EventPackets.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "stdafx.h"
#include "EventPackets.h"
#include "EventStore.h"
#include "Data/Translation/MultiLanguage.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace Net::Event
{
namespace
{
/// <summary>The sub codes the server answers with.</summary>
constexpr BYTE SubCodeEventList = 0x01;
constexpr BYTE SubCodeJoinResult = 0x02;

/// <summary>The byte lengths of the fields which repeat inside the list packet.</summary>
constexpr int EntryLength = 208;
constexpr int NameLength = 48;
constexpr int DescriptionLength = 128;

/// <summary>The longest string this file converts, plus the terminator.</summary>
constexpr int MaxConvertedLength = 144;

/// <summary>Reads an unsigned 32 bit number which the server wrote in little endian order.</summary>
uint32_t ReadUInt32(const std::span<const BYTE> data, const int offset)
{
    uint32_t value = 0;
    for (int i = 0; i < 4; ++i)
    {
        value |= static_cast<uint32_t>(data[offset + i]) << (8 * i);
    }

    return value;
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

void ReadEventList(const std::span<const BYTE> packet, const int offset)
{
    if (static_cast<int>(packet.size()) < offset + 1)
    {
        return;
    }

    const int count = packet[offset];
    const int first = offset + 1;
    if (static_cast<int>(packet.size()) < first + count * EntryLength)
    {
        return;
    }

    std::vector<Entry> events;
    events.reserve(count);
    for (int i = 0; i < count; ++i)
    {
        const int start = first + i * EntryLength;
        Entry entry;
        std::memcpy(entry.EventId.data(), packet.data() + start, EventIdLength);
        entry.EventKind = static_cast<Kind>(packet[start + 16]);
        entry.State = static_cast<RunState>(packet[start + 17]);
        entry.SecondsUntilStart = ReadUInt32(packet, start + 18);
        entry.SecondsRemaining = ReadUInt32(packet, start + 22);
        entry.DurationSeconds = ReadUInt32(packet, start + 26);
        entry.PlayerCount = packet[start + 30];
        entry.Join = static_cast<JoinMode>(packet[start + 31]);
        entry.Name = ReadString(packet, start + 32, NameLength);
        entry.Description = ReadString(packet, start + 80, DescriptionLength);
        events.push_back(std::move(entry));
    }

    Store::Instance().SetEvents(std::move(events));
}

void ReadJoinResult(const std::span<const BYTE> packet, const int offset)
{
    if (static_cast<int>(packet.size()) < offset + EventIdLength + 1)
    {
        return;
    }

    std::array<BYTE, EventIdLength> eventId{};
    std::memcpy(eventId.data(), packet.data() + offset, EventIdLength);
    Store::Instance().SetJoinResult(eventId, static_cast<JoinResult>(packet[offset + EventIdLength]));
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
    case SubCodeEventList:
        ReadEventList(packet, offset);
        break;
    case SubCodeJoinResult:
        ReadJoinResult(packet, offset);
        break;
    default:
        break;
    }
}
} // namespace Net::Event
