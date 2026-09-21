// <copyright file="EventStore.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include "EventProtocol.h"

#include <vector>

namespace Net::Event
{
/// <summary>
/// What the server last told this client about the events it runs.
/// </summary>
/// <remarks>
/// The event list window draws from here and never keeps its own copy. The tick at which the list
/// arrived is kept with it, so every countdown on screen is worked out from one moment and they
/// cannot drift apart from each other.
/// </remarks>
class Store
{
public:
    /// <summary>Gets the one store of this client.</summary>
    static Store& Instance();

    /// <summary>Replaces the list of events, and remembers when it arrived.</summary>
    void SetEvents(std::vector<Entry> events);

    /// <summary>Gets the events, in the order the server sent them.</summary>
    const std::vector<Entry>& GetEvents() const
    {
        return m_events;
    }

    /// <summary>
    /// Gets how many seconds have passed since the list arrived, which every countdown is reduced by.
    /// </summary>
    unsigned int GetSecondsSinceReceived() const;

    /// <summary>Gets how often the list has been replaced.</summary>
    /// <remarks>
    /// The window pages the events and has to notice that another list arrived, so that it does not
    /// leave the player on a page which no longer exists.
    /// </remarks>
    unsigned int GetGeneration() const
    {
        return m_generation;
    }

    /// <summary>Remembers how the last join request ended.</summary>
    void SetJoinResult(const std::array<BYTE, EventIdLength>& eventId, JoinResult result);

    /// <summary>
    /// Takes the answer which arrived since this was last asked, if one did.
    /// </summary>
    /// <param name="eventId">Receives the event the answer belongs to.</param>
    /// <param name="result">Receives how it ended.</param>
    /// <returns><c>true</c> when there was an answer nobody had seen yet.</returns>
    /// <remarks>
    /// The window polls this instead of the parser telling it directly, so that reading a packet
    /// stays free of anything which draws.
    /// </remarks>
    bool TakeNewJoinResult(std::array<BYTE, EventIdLength>& eventId, JoinResult& result);

    /// <summary>Forgets everything, for a logout or a character change.</summary>
    void Clear();

private:
    std::vector<Entry> m_events;
    DWORD m_receivedTick = 0;
    unsigned int m_generation = 0;
    std::array<BYTE, EventIdLength> m_lastJoinEventId{};
    JoinResult m_lastJoinResult = JoinResult::Failed;
    bool m_hasUnseenJoinResult = false;
};
} // namespace Net::Event
