// <copyright file="BankPackets.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include "BankProtocol.h"

#include <span>

namespace Net::Bank
{
/// <summary>
/// Reads a packet of the bank (code 0xFB) into the <see cref="Store"/>.
/// </summary>
/// <param name="packet">The whole packet, header included.</param>
/// <remarks>
/// A packet which is too short for what its header promises is dropped without touching the
/// store, so a truncated or hostile packet can leave the dialog stale but never wrong.
/// </remarks>
void HandlePacket(std::span<const BYTE> packet);
} // namespace Net::Bank
