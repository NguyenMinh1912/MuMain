// <copyright file="ConnectionManager.ClientToServer.Custom.cs" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

namespace MUnique.Client.Library;

using System;
using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Text;
using MUnique.OpenMU.Network;
using MUnique.OpenMU.Network.Packets.ClientToServer;
using MUnique.OpenMU.Network.Xor;

/// <summary>
/// Extension methods to start writing messages of this namespace on a <see cref="IConnection"/>.
/// </summary>
public unsafe partial class ConnectionManager
{
    /// <summary>
    /// The length of the identifier of a market offer of the bank, as it travels.
    /// </summary>
    private const int ListingIdLength = 16;

    private static readonly Xor3Encryptor Xor3Encryptor = new(0);

    /// <summary>
    /// Sends a <see cref="LoginLongPassword" /> to this connection.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="username">The user name, "encrypted" with Xor3.</param>
    /// <param name="password">The password, "encrypted" with Xor3.</param>
    /// <param name="tickCount">The tick count.</param>
    /// <param name="clientVersion">The client version.</param>
    /// <param name="clientSerial">The client serial.</param>
    /// <remarks>
    /// Is sent by the client when: The player tries to log into the game.
    /// Causes reaction on server side: The server is authenticating the sent login name and password. If it's correct, the state of the player is proceeding to be logged in.
    /// </remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendLogin")]
    public static void SendLogin(int handle, IntPtr username, IntPtr password, uint @tickCount, byte* @clientVersion, byte* @clientSerial)
    {
        if (!Connections.TryGetValue(handle, out var connection))
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: Login send skipped; connection handle={handle} not found");
            return;
        }

        try
        {
            var usernameStr = NativeInterop.PtrToWideString(@username)
                ?? throw new ArgumentNullException(nameof(username));
            var passwordStr = NativeInterop.PtrToWideString(@password)
                ?? throw new ArgumentNullException(nameof(password));
            ArgumentNullException.ThrowIfNull(@clientVersion);
            ArgumentNullException.ThrowIfNull(@clientSerial);

            const int usernameLength = 10;
            const int passwordLength = 20;
            if (Encoding.UTF8.GetByteCount(usernameStr) > usernameLength
                || Encoding.UTF8.GetByteCount(passwordStr) > passwordLength)
            {
                throw new ArgumentException("Login credentials exceed packet field length.");
            }

            connection.CreateAndSend(pipeWriter =>
            {
                Span<byte> usernameBytes = stackalloc byte[usernameLength];
                Span<byte> passwordBytes = stackalloc byte[passwordLength];
                usernameBytes.Clear();
                passwordBytes.Clear();
                Encoding.UTF8.GetBytes(usernameStr, usernameBytes);
                Encoding.UTF8.GetBytes(passwordStr, passwordBytes);
                Xor3Encryptor.Encrypt(usernameBytes);
                Xor3Encryptor.Encrypt(passwordBytes);

                var length = LoginLongPasswordRef.Length;
                var packet = new LoginLongPasswordRef(pipeWriter.GetSpan(length)[..length]);
                usernameBytes.CopyTo(packet.Username);
                passwordBytes.CopyTo(packet.Password);
                packet.TickCount = @tickCount;
                new Span<byte>(@clientVersion, packet.ClientVersion.Length).CopyTo(packet.ClientVersion);
                new Span<byte>(@clientSerial, packet.ClientSerial.Length).CopyTo(packet.ClientSerial);

                return length;
            });
            ManagedLog.Write(ManagedLog.Level.Info, $"NET: Login packet staged, handle={handle}, bytes={LoginLongPasswordRef.Length}");
        }
        catch (Exception ex)
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: Login packet staging failed, handle={handle}: {ex}");
        }
    }

    /// <summary>
    /// Sends a stat point increase request for several points at once to this connection.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="statType">The stat type (0 = strength, 1 = agility, 2 = vitality, 3 = energy, 4 = leadership).</param>
    /// <param name="amount">The number of points to add to that stat.</param>
    /// <remarks>
    /// Not part of the original protocol: the original client sends one 0xF3, 0x06 packet per point, which makes
    /// spending a big pool of level-up-points slow. The sub code 0xE0 is unused by the original client.
    /// There is no generated struct for it, because the packet definitions come from the NuGet package.
    /// </remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendIncreaseCharacterStatPointMultiple")]
    public static void SendIncreaseCharacterStatPointMultiple(int handle, byte @statType, ushort @amount)
    {
        if (!Connections.TryGetValue(handle, out var connection))
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: Stat point increase send skipped; connection handle={handle} not found");
            return;
        }

        if (@amount == 0)
        {
            return;
        }

        try
        {
            connection.CreateAndSend(pipeWriter =>
            {
                const int length = 7;
                var packet = pipeWriter.GetSpan(length)[..length];
                packet[0] = 0xC1;
                packet[1] = length;
                packet[2] = 0xF3;
                packet[3] = 0xE0;
                packet[4] = @statType;
                packet[5] = (byte)(@amount >> 8);
                packet[6] = (byte)(@amount & 0xFF);

                return length;
            });
        }
        catch (Exception ex)
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: Stat point increase packet staging failed, handle={handle}: {ex}");
        }
    }

    /// <summary>
    /// Sends a master skill point add request for several points at once to this connection.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="skillId">The master skill to raise.</param>
    /// <param name="amount">The number of points to add to that skill.</param>
    /// <remarks>
    /// Not part of the original protocol: the original client sends one 0xF3, 0x52 packet per point, which makes
    /// filling a master skill tree slow. The sub code 0xE1 is unused by the original client.
    /// There is no generated struct for it, because the packet definitions come from the NuGet package.
    /// </remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendAddMasterSkillPointMultiple")]
    public static void SendAddMasterSkillPointMultiple(int handle, ushort @skillId, byte @amount)
    {
        if (!Connections.TryGetValue(handle, out var connection))
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: Master skill point add send skipped; connection handle={handle} not found");
            return;
        }

        if (@amount == 0)
        {
            return;
        }

        try
        {
            connection.CreateAndSend(pipeWriter =>
            {
                const int length = 7;
                var packet = pipeWriter.GetSpan(length)[..length];
                packet[0] = 0xC1;
                packet[1] = length;
                packet[2] = 0xF3;
                packet[3] = 0xE1;
                packet[4] = (byte)(@skillId & 0xFF);
                packet[5] = (byte)(@skillId >> 8);
                packet[6] = @amount;

                return length;
            });
        }
        catch (Exception ex)
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: Master skill point add packet staging failed, handle={handle}: {ex}");
        }
    }

    /// <summary>
    /// Sends the answer of the reset confirmation dialog to this connection.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="resetTypeIndex">The reset type index of the corresponding confirmation request.</param>
    /// <param name="accepted">1, when the player accepted the reset; otherwise 0.</param>
    /// <remarks>
    /// Not part of the original protocol: the server announces a reset with a 0xF3, 0xE0 message and only
    /// performs it after this answer, so a misclick at the reset npc can't cost a character its progress.
    /// There is no generated struct for it, because the packet definitions come from the NuGet package.
    /// </remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendResetConfirmation")]
    public static void SendResetConfirmation(int handle, byte @resetTypeIndex, byte @accepted)
    {
        if (!Connections.TryGetValue(handle, out var connection))
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: Reset confirmation send skipped; connection handle={handle} not found");
            return;
        }

        try
        {
            connection.CreateAndSend(pipeWriter =>
            {
                const int length = 6;
                var packet = pipeWriter.GetSpan(length)[..length];
                packet[0] = 0xC1;
                packet[1] = length;
                packet[2] = 0xF3;
                packet[3] = 0xE2;
                packet[4] = @resetTypeIndex;
                packet[5] = @accepted;

                return length;
            });
        }
        catch (Exception ex)
        {
            ManagedLog.Write(ManagedLog.Level.Error, $"NET: Reset confirmation packet staging failed, handle={handle}: {ex}");
        }
    }

    /// <summary>
    /// Sends the request to move value between the character and the bank of its account.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="deposit">1 to move the value into the bank, 0 to take it out.</param>
    /// <param name="currency">The currency, as the BankCurrency of the server.</param>
    /// <param name="amount">The amount to move; always positive.</param>
    /// <remarks>
    /// Not part of the original protocol (0xFB, 0x01). Written by hand because the packet
    /// definitions of the bank are not in the NuGet package the generated functions come from.
    /// </remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendBankMoveValue")]
    public static void SendBankMoveValue(int handle, byte @deposit, byte @currency, long @amount)
    {
        SendBankPacket(handle, "bank move value", 0x01, 14, packet =>
        {
            packet[4] = @deposit;
            packet[5] = @currency;
            BinaryPrimitives.WriteInt64LittleEndian(packet[6..], @amount);
        });
    }

    /// <summary>
    /// Sends the request to transfer an amount of a currency to another account.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="receiverName">The character or login name of the receiver.</param>
    /// <param name="currency">The currency, as the BankCurrency of the server.</param>
    /// <param name="amount">The amount the receiver gets.</param>
    /// <param name="note">The message for the receiver.</param>
    /// <remarks>Not part of the original protocol (0xFB, 0x02).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendBankTransferValue")]
    public static void SendBankTransferValue(int handle, IntPtr @receiverName, byte @currency, long @amount, IntPtr @note)
    {
        var receiver = NativeInterop.PtrToWideString(@receiverName) ?? string.Empty;
        var message = NativeInterop.PtrToWideString(@note) ?? string.Empty;
        SendBankPacket(handle, "bank transfer value", 0x02, 83, packet =>
        {
            WriteFixedString(packet.Slice(4, 10), receiver);
            packet[14] = @currency;
            BinaryPrimitives.WriteInt64LittleEndian(packet[15..], @amount);
            WriteFixedString(packet.Slice(23, 60), message);
        });
    }

    /// <summary>
    /// Sends the request to transfer an item of the bank to another account.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="receiverName">The character or login name of the receiver.</param>
    /// <param name="bankSlot">The box of the item in the item storage of the bank.</param>
    /// <param name="note">The message for the receiver.</param>
    /// <remarks>Not part of the original protocol (0xFB, 0x03).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendBankTransferItem")]
    public static void SendBankTransferItem(int handle, IntPtr @receiverName, byte @bankSlot, IntPtr @note)
    {
        var receiver = NativeInterop.PtrToWideString(@receiverName) ?? string.Empty;
        var message = NativeInterop.PtrToWideString(@note) ?? string.Empty;
        SendBankPacket(handle, "bank transfer item", 0x03, 75, packet =>
        {
            WriteFixedString(packet.Slice(4, 10), receiver);
            packet[14] = @bankSlot;
            WriteFixedString(packet.Slice(15, 60), message);
        });
    }

    /// <summary>
    /// Sends the request to offer an item of the bank on the market.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="bankSlot">The box of the item in the item storage of the bank.</param>
    /// <param name="priceCurrency">The currency the seller wants to be paid in.</param>
    /// <param name="price">The price.</param>
    /// <remarks>Not part of the original protocol (0xFB, 0x04).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendMarketRegisterItem")]
    public static void SendMarketRegisterItem(int handle, byte @bankSlot, byte @priceCurrency, long @price)
    {
        SendBankPacket(handle, "market register item", 0x04, 14, packet =>
        {
            packet[4] = @bankSlot;
            packet[5] = @priceCurrency;
            BinaryPrimitives.WriteInt64LittleEndian(packet[6..], @price);
        });
    }

    /// <summary>
    /// Sends the request to offer an amount of a currency of the bank on the market.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="offeredCurrency">The offered currency.</param>
    /// <param name="offeredAmount">The offered amount.</param>
    /// <param name="priceCurrency">The currency the seller wants to be paid in.</param>
    /// <param name="price">The price.</param>
    /// <remarks>Not part of the original protocol (0xFB, 0x05).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendMarketRegisterCurrency")]
    public static void SendMarketRegisterCurrency(int handle, byte @offeredCurrency, long @offeredAmount, byte @priceCurrency, long @price)
    {
        SendBankPacket(handle, "market register currency", 0x05, 22, packet =>
        {
            packet[4] = @offeredCurrency;
            BinaryPrimitives.WriteInt64LittleEndian(packet[5..], @offeredAmount);
            packet[13] = @priceCurrency;
            BinaryPrimitives.WriteInt64LittleEndian(packet[14..], @price);
        });
    }

    /// <summary>
    /// Sends the request to buy an offer of the market.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="listingId">The 16 bytes of the identifier of the offer, as they arrived.</param>
    /// <remarks>Not part of the original protocol (0xFB, 0x06).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendMarketBuy")]
    public static void SendMarketBuy(int handle, byte* @listingId)
    {
        SendListingRequest(handle, "market buy", 0x06, @listingId);
    }

    /// <summary>
    /// Sends the request to take an own offer off the market.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="listingId">The 16 bytes of the identifier of the offer, as they arrived.</param>
    /// <remarks>Not part of the original protocol (0xFB, 0x07).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendMarketCancel")]
    public static void SendMarketCancel(int handle, byte* @listingId)
    {
        SendListingRequest(handle, "market cancel", 0x07, @listingId);
    }

    /// <summary>
    /// Sends the request to list the offers of the market.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="page">The page to show, starting at 0.</param>
    /// <param name="priceCurrencyFilter">The currency the price has to be in; 255 for any.</param>
    /// <param name="ownOffersOnly">1 to list only the offers of this account.</param>
    /// <param name="nameFilter">A text which the name of the offer has to contain.</param>
    /// <remarks>Not part of the original protocol (0xFB, 0x08).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendMarketList")]
    public static void SendMarketList(int handle, byte @page, byte @priceCurrencyFilter, byte @ownOffersOnly, IntPtr @nameFilter)
    {
        var filter = NativeInterop.PtrToWideString(@nameFilter) ?? string.Empty;
        SendBankPacket(handle, "market list", 0x08, 27, packet =>
        {
            packet[4] = @page;
            packet[5] = @priceCurrencyFilter;
            packet[6] = @ownOffersOnly;
            WriteFixedString(packet.Slice(7, 20), filter);
        });
    }

    /// <summary>
    /// Sends the request to show a page of the ledger of the bank.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="page">The page to show, starting at 0.</param>
    /// <remarks>Not part of the original protocol (0xFB, 0x09).</remarks>
    [UnmanagedCallersOnly(EntryPoint = "ConnectionManager_SendBankLedger")]
    public static void SendBankLedger(int handle, byte @page)
    {
        SendBankPacket(handle, "bank ledger", 0x09, 5, packet => packet[4] = @page);
    }

    /// <summary>
    /// Writes a string into a field of a fixed length, padded with zeros and cut off when it is
    /// longer than the field.
    /// </summary>
    /// <param name="target">The field.</param>
    /// <param name="value">The string.</param>
    private static void WriteFixedString(Span<byte> target, string value)
    {
        target.Clear();
        if (string.IsNullOrEmpty(value))
        {
            return;
        }

        // The last byte stays zero, so the server always reads a terminated string.
        Encoding.UTF8.GetBytes(value.AsSpan(), target[..^1]);
    }

    /// <summary>
    /// Writes and sends a packet of the bank, which all share the code 0xFB and differ by sub code.
    /// </summary>
    /// <param name="handle">The handle of the connection.</param>
    /// <param name="what">What is sent, for the log of a failure.</param>
    /// <param name="subCode">The sub code of the request.</param>
    /// <param name="length">The length of the packet.</param>
    /// <param name="fill">Writes the fields behind the header.</param>
    private static void SendBankPacket(int handle, string what, byte subCode, int length, PacketWriter fill)
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
                packet[2] = 0xFB;
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

    private static void SendListingRequest(int handle, string what, byte subCode, byte* listingId)
    {
        if (listingId is null)
        {
            return;
        }

        var id = new ReadOnlySpan<byte>(listingId, ListingIdLength).ToArray();
        SendBankPacket(handle, what, subCode, 20, packet => id.CopyTo(packet[4..]));
    }

    /// <summary>
    /// Fills the fields of a packet of the bank behind its header.
    /// </summary>
    /// <param name="packet">The packet.</param>
    private delegate void PacketWriter(Span<byte> packet);
}
