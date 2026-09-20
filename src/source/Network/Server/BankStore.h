// <copyright file="BankStore.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include "BankProtocol.h"

namespace Net::Bank
{
/// <summary>
/// What the server last told this client about the bank of its account and about the market.
/// </summary>
/// <remarks>
/// The bank dialog draws from here and never keeps its own copy, so a balance which changed
/// because a transfer arrived is on screen the moment the packet was parsed, no matter which tab
/// the player is looking at.
/// </remarks>
class Store
{
public:
    /// <summary>Gets the one store of this client.</summary>
    static Store& Instance();

    /// <summary>Sets what the bank holds of one currency.</summary>
    void SetBalance(Currency currency, int64_t amount);

    /// <summary>Gets what the bank holds of one currency.</summary>
    int64_t GetBalance(Currency currency) const;

    /// <summary>Replaces the listed page of the market.</summary>
    void SetOffers(std::vector<Offer> offers, BYTE page, BYTE pageCount);

    /// <summary>Gets the offers of the listed page.</summary>
    const std::vector<Offer>& GetOffers() const
    {
        return m_offers;
    }

    /// <summary>Gets the number of the listed page, starting at 0.</summary>
    BYTE GetOfferPage() const
    {
        return m_offerPage;
    }

    /// <summary>Gets how many pages the current search has.</summary>
    BYTE GetOfferPageCount() const
    {
        return m_offerPageCount;
    }

    /// <summary>Gets how often the listed page of the market has been replaced.</summary>
    /// <remarks>
    /// The dialog keeps an item of its own for every offer so that it can draw its picture, and it
    /// has to build those again whenever another page arrives. Comparing this number is what tells
    /// it that one did, without the parser having to reach into the dialog.
    /// </remarks>
    unsigned int GetOfferGeneration() const
    {
        return m_offerGeneration;
    }

    /// <summary>Replaces the listed page of the ledger.</summary>
    void SetLedger(std::vector<LedgerEntry> entries, BYTE page);

    /// <summary>Gets the entries of the listed page of the ledger.</summary>
    const std::vector<LedgerEntry>& GetLedger() const
    {
        return m_ledger;
    }

    /// <summary>Gets the number of the listed ledger page, starting at 0.</summary>
    BYTE GetLedgerPage() const
    {
        return m_ledgerPage;
    }

    /// <summary>Remembers how the last request ended.</summary>
    void SetLastResult(Operation operation, ResultCode result);

    /// <summary>
    /// Takes the answer which arrived since this was last asked, if one did.
    /// </summary>
    /// <param name="operation">Receives the request the answer belongs to.</param>
    /// <param name="result">Receives how it ended.</param>
    /// <returns><c>true</c> when there was an answer nobody had seen yet.</returns>
    /// <remarks>
    /// The dialog polls this instead of the parser telling it directly, so that reading a packet
    /// stays free of anything which draws.
    /// </remarks>
    bool TakeNewResult(Operation& operation, ResultCode& result);

    /// <summary>Gets the request the last answer belonged to.</summary>
    Operation GetLastOperation() const
    {
        return m_lastOperation;
    }

    /// <summary>Gets how the last request ended.</summary>
    ResultCode GetLastResult() const
    {
        return m_lastResult;
    }

    /// <summary>Forgets everything, for a logout or a character change.</summary>
    void Clear();

private:
    std::array<int64_t, static_cast<size_t>(Currency::Count)> m_balances{};
    std::vector<Offer> m_offers;
    std::vector<LedgerEntry> m_ledger;
    BYTE m_offerPage = 0;
    BYTE m_offerPageCount = 1;
    unsigned int m_offerGeneration = 0;
    BYTE m_ledgerPage = 0;
    Operation m_lastOperation = Operation::Deposit;
    ResultCode m_lastResult = ResultCode::Success;
    bool m_hasUnseenResult = false;
};
} // namespace Net::Bank
