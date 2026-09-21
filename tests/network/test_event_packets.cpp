#include "Core/Platform/WinCompat.h"
#include "Network/Server/EventPackets.h"
#include "Network/Server/EventStore.h"

#include "doctest.h"

#include <cstring>
#include <string>
#include <vector>

namespace
{
constexpr int EntryLength = 152;
constexpr int NameOffset = 32;
constexpr int DescriptionOffset = 72;

/// <summary>Writes an unsigned 32 bit number the way the server writes it.</summary>
void WriteUInt32(std::vector<BYTE>& packet, const size_t offset, const uint32_t value)
{
    for (int i = 0; i < 4; ++i)
    {
        packet[offset + i] = static_cast<BYTE>((value >> (8 * i)) & 0xFF);
    }
}

void WriteText(std::vector<BYTE>& packet, const size_t offset, const std::string& text)
{
    std::memcpy(packet.data() + offset, text.data(), text.size());
}

/// <summary>Builds an event list packet which carries the given number of entries.</summary>
/// <param name="entryCount">How many entries the packet carries.</param>
/// <param name="claimedCount">What the count byte says, which a hostile packet may exaggerate.</param>
std::vector<BYTE> MakeEventList(const int entryCount, const int claimedCount)
{
    const size_t size = 6 + static_cast<size_t>(entryCount) * EntryLength;
    std::vector<BYTE> packet(size, 0);
    packet[0] = 0xC2;
    packet[1] = static_cast<BYTE>((size >> 8) & 0xFF);
    packet[2] = static_cast<BYTE>(size & 0xFF);
    packet[3] = 0xFC;
    packet[4] = 0x01;
    packet[5] = static_cast<BYTE>(claimedCount);

    for (int index = 0; index < entryCount; ++index)
    {
        const size_t start = 6 + static_cast<size_t>(index) * EntryLength;
        for (int byte = 0; byte < Net::Event::EventIdLength; ++byte)
        {
            packet[start + byte] = static_cast<BYTE>(index * 16 + byte);
        }

        packet[start + 16] = static_cast<BYTE>(Net::Event::Kind::MiniGame);
        packet[start + 17] = static_cast<BYTE>(Net::Event::RunState::EntranceOpen);
        WriteUInt32(packet, start + 18, 0);
        WriteUInt32(packet, start + 22, 150);
        WriteUInt32(packet, start + 26, 1800);
        packet[start + 30] = 7;
        packet[start + 31] = static_cast<BYTE>(Net::Event::JoinMode::Enter);
        WriteText(packet, start + NameOffset, "Blood Castle");
        WriteText(packet, start + DescriptionOffset, "Starts from its timetable.");
    }

    return packet;
}

std::vector<BYTE> MakeJoinResult(const Net::Event::JoinResult result)
{
    std::vector<BYTE> packet(21, 0);
    packet[0] = 0xC1;
    packet[1] = 21;
    packet[2] = 0xFC;
    packet[3] = 0x02;
    for (int byte = 0; byte < Net::Event::EventIdLength; ++byte)
    {
        packet[4 + byte] = static_cast<BYTE>(byte);
    }

    packet[20] = static_cast<BYTE>(result);
    return packet;
}
} // namespace

TEST_CASE("an event list fills the store")
{
    Net::Event::Store::Instance().Clear();

    const std::vector<BYTE> packet = MakeEventList(2, 2);
    Net::Event::HandlePacket(packet);

    const std::vector<Net::Event::Entry>& events = Net::Event::Store::Instance().GetEvents();
    REQUIRE(events.size() == 2);
    CHECK(events[0].EventId[0] == 0);
    CHECK(events[1].EventId[0] == 16);
    CHECK(events[0].EventKind == Net::Event::Kind::MiniGame);
    CHECK(events[0].State == Net::Event::RunState::EntranceOpen);
    CHECK(events[0].SecondsUntilStart == 0);
    CHECK(events[0].SecondsRemaining == 150);
    CHECK(events[0].DurationSeconds == 1800);
    CHECK(events[0].PlayerCount == 7);
    CHECK(events[0].Join == Net::Event::JoinMode::Enter);
    CHECK(events[0].Name == L"Blood Castle");
    CHECK(events[0].Description == L"Starts from its timetable.");
}

TEST_CASE("a list which claims more entries than it carries is dropped")
{
    Net::Event::Store::Instance().Clear();
    Net::Event::HandlePacket(MakeEventList(2, 2));
    const unsigned int generation = Net::Event::Store::Instance().GetGeneration();

    // The count byte says three, the packet carries one: reading it would run past its end.
    Net::Event::HandlePacket(MakeEventList(1, 3));

    CHECK(Net::Event::Store::Instance().GetEvents().size() == 2);
    CHECK(Net::Event::Store::Instance().GetGeneration() == generation);
}

TEST_CASE("a truncated list is dropped")
{
    Net::Event::Store::Instance().Clear();
    Net::Event::HandlePacket(MakeEventList(2, 2));

    std::vector<BYTE> truncated = MakeEventList(1, 1);
    truncated.resize(truncated.size() - 1);
    Net::Event::HandlePacket(truncated);

    CHECK(Net::Event::Store::Instance().GetEvents().size() == 2);
}

TEST_CASE("a packet which holds nothing but its header is dropped")
{
    Net::Event::Store::Instance().Clear();

    const std::vector<BYTE> header{0xC2, 0x00, 0x05, 0xFC, 0x01};
    Net::Event::HandlePacket(header);

    CHECK(Net::Event::Store::Instance().GetEvents().empty());
}

TEST_CASE("a join result is answered once")
{
    Net::Event::Store::Instance().Clear();
    Net::Event::HandlePacket(MakeJoinResult(Net::Event::JoinResult::NotOpen));

    std::array<BYTE, Net::Event::EventIdLength> eventId{};
    Net::Event::JoinResult result = Net::Event::JoinResult::Success;

    REQUIRE(Net::Event::Store::Instance().TakeNewJoinResult(eventId, result));
    CHECK(result == Net::Event::JoinResult::NotOpen);
    CHECK(eventId[3] == 3);

    // The window polls, so an answer which was already read must not be read a second time.
    CHECK_FALSE(Net::Event::Store::Instance().TakeNewJoinResult(eventId, result));
}

TEST_CASE("a countdown is reduced by what has passed since the list arrived")
{
    Net::Event::Store::Instance().Clear();
    Net::Event::HandlePacket(MakeEventList(1, 1));

    // Nothing has passed yet, so nothing is taken off.
    CHECK(Net::Event::Store::Instance().GetSecondsSinceReceived() == 0);
}
