// <copyright file="EventPackets.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include "EventProtocol.h"

#include <span>

namespace Net::Event
{
/// <summary>
/// Reads a packet of the event list (code 0xFC) into the <see cref="Store"/>.
/// </summary>
/// <param name="packet">The whole packet, header included.</param>
/// <remarks>
/// A packet which is too short for what its header promises is dropped without touching the store,
/// so a truncated or hostile packet can leave the window stale but never wrong.
/// </remarks>
void HandlePacket(std::span<const BYTE> packet);
} // namespace Net::Event
