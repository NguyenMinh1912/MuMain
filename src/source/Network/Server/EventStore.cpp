// <copyright file="EventStore.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "stdafx.h"
#include "EventStore.h"

namespace Net::Event
{
Store& Store::Instance()
{
    static Store instance;
    return instance;
}

void Store::SetEvents(std::vector<Entry> events)
{
    m_events = std::move(events);
    m_receivedTick = GetTickCount();
    ++m_generation;
}

unsigned int Store::GetSecondsSinceReceived() const
{
    if (m_receivedTick == 0)
    {
        return 0;
    }

    // The tick count wraps around after about 49 days; the subtraction of two unsigned values wraps
    // with it, so the difference stays right across the wrap.
    return static_cast<unsigned int>((GetTickCount() - m_receivedTick) / 1000);
}

void Store::SetJoinResult(const std::array<BYTE, EventIdLength>& eventId, const JoinResult result)
{
    m_lastJoinEventId = eventId;
    m_lastJoinResult = result;
    m_hasUnseenJoinResult = true;
}

bool Store::TakeNewJoinResult(std::array<BYTE, EventIdLength>& eventId, JoinResult& result)
{
    if (!m_hasUnseenJoinResult)
    {
        return false;
    }

    eventId = m_lastJoinEventId;
    result = m_lastJoinResult;
    m_hasUnseenJoinResult = false;
    return true;
}

void Store::Clear()
{
    m_events.clear();
    m_receivedTick = 0;
    m_generation = 0;
    m_lastJoinEventId = {};
    m_lastJoinResult = JoinResult::Failed;
    m_hasUnseenJoinResult = false;
}
} // namespace Net::Event
