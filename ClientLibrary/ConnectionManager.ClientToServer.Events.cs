// <copyright file="ConnectionManager.ClientToServer.Events.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.Client.Library;

using System;
using System.Runtime.InteropServices;

/// <summary>
/// The requests of the event list window, which all share the code 0xFC and differ by sub code.
/// </summary>
/// <remarks>
/// Not part of the original protocol. The events are reached from the menu at the bottom right and
/// not by talking to an npc, so both requests are ones of their own.
/// </remarks>
public unsafe partial class ConnectionManager
{
    /// <summary>
    /// The length of the identifier of an event, as it travels.
    /// </summary>
    private const int EventIdLength = 16;

    /// <summary>
    /// Sends the request to list the events which this game server runs.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <remarks>Not part of the original protocol (0xFC, 0x01).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendEventList")]
    public static void SendEventList(int handle)
    {
        SendEventPacket(handle, "event list", 0x01, 4, static _ => { });
    }

    /// <summary>
    /// Sends the request to join an event of the list.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="eventId">The identifier of the event, as it arrived in the list.</param>
    /// <remarks>Not part of the original protocol (0xFC, 0x02).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendEventJoin")]
    public static void SendEventJoin(int handle, byte* eventId)
    {
        if (eventId is null)
        {
            return;
        }

        var id = new ReadOnlySpan<byte>(eventId, EventIdLength).ToArray();
        SendEventPacket(handle, "event join", 0x02, 20, packet => id.CopyTo(packet[4..]));
    }

    /// <summary>
    /// Writes and sends a packet of the event list.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="what">What is sent, for the log of a failure.</param>
    /// <param name="subCode">The sub code of the request.</param>
    /// <param name="length">The length of the packet.</param>
    /// <param name="fill">Writes the fields behind the header.</param>
    private static void SendEventPacket(int handle, string what, byte subCode, int length, PacketWriter fill)
    {
        if (!Connections.TryGetValue(handle, out var connection))
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: {what} send skipped; connection handle={handle} not found");
            return;
        }

        try
        {
            connection.CreateAndSend(pipeWriter =>
            {
                var packet = pipeWriter.GetSpan(length)[..length];
                packet.Clear();
                packet[0] = 0xC1;
                packet[1] = (byte)length;
                packet[2] = 0xFC;
                packet[3] = subCode;
                fill(packet);

                return length;
            });
        }
        catch (Exception ex)
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: {what} packet staging failed, handle={handle}: {ex}");
        }
    }
}
