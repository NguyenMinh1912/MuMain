// <copyright file="EventProtocol.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include "stdafx.h"

#include <array>
#include <cstdint>
#include <string>

/// <summary>
/// The wire values and the received shapes of the event list of the game server (packet code 0xFC).
/// None of this is part of the original protocol.
/// </summary>
namespace Net::Event
{
/// <summary>The packet code which every request and every answer of the event list uses.</summary>
constexpr BYTE PacketCode = 0xFC;

/// <summary>The length of the identifier of an event, as it travels.</summary>
constexpr int EventIdLength = 16;

/// <summary>
/// The countdown of an event whose timetable is empty. It never starts by itself, so there is
/// nothing to count down to.
/// </summary>
constexpr uint32_t NoNextStart = 0xFFFFFFFFu;

/// <summary>
/// Which additional settings an event has. Mirrors the EventKind of the server, which works it out
/// from the type of the configuration of the plug-in.
/// </summary>
enum class Kind : BYTE
{
    Periodic = 0,
    Invasion = 1,
    MiniGame = 2,
    HappyHour = 3,
};

/// <summary>Where an event stands right now. Mirrors the EventRunState of the server.</summary>
enum class RunState : BYTE
{
    Unscheduled = 0,
    Waiting = 1,
    EntranceOpen = 2,
    Running = 3,
};

/// <summary>What pressing the join button of an event does. Mirrors the EventJoinMode of the server.</summary>
enum class JoinMode : BYTE
{
    None = 0,
    Enter = 1,
};

/// <summary>How a join request ended. Mirrors the EventJoinResult of the server.</summary>
enum class JoinResult : BYTE
{
    Success = 0,
    Failed = 1,
    NotOpen = 2,
    CharacterLevelTooHigh = 3,
    CharacterLevelTooLow = 4,
    Full = 5,
    NotEnoughMoney = 6,
    PlayerKillerCantEnter = 7,
    UnknownEvent = 8,
    NotJoinable = 9,
};

/// <summary>One event of the game server, as the client keeps it after it arrived.</summary>
/// <remarks>
/// The countdowns are the durations the server sent and are not turned into points in time: the
/// clock of this machine is not the clock of the server, and the window counts down from the moment
/// the packet was read instead.
/// </remarks>
struct Entry
{
    std::array<BYTE, EventIdLength> EventId{};
    Kind EventKind = Kind::Periodic;
    RunState State = RunState::Unscheduled;
    uint32_t SecondsUntilStart = NoNextStart;
    uint32_t SecondsRemaining = 0;
    uint32_t DurationSeconds = 0;
    BYTE PlayerCount = 0;
    JoinMode Join = JoinMode::None;
    std::wstring Name;
    std::wstring Description;
};
} // namespace Net::Event
