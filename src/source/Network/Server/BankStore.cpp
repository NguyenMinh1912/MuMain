// <copyright file="BankStore.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "stdafx.h"
#include "BankStore.h"

namespace Net::Bank
{
Store& Store::Instance()
{
    static Store instance;
    return instance;
}

void Store::SetBalance(const Currency currency, const int64_t amount)
{
    const auto index = static_cast<size_t>(currency);
    if (index >= m_balances.size())
    {
        return;
    }

    m_balances[index] = amount;
}

int64_t Store::GetBalance(const Currency currency) const
{
    const auto index = static_cast<size_t>(currency);
    return index < m_balances.size() ? m_balances[index] : 0;
}

void Store::SetOffers(std::vector<Offer> offers, const BYTE page, const BYTE pageCount)
{
    m_offers = std::move(offers);
    m_offerPage = page;
    m_offerPageCount = pageCount > 0 ? pageCount : 1;
    ++m_offerGeneration;
}

void Store::SetLedger(std::vector<LedgerEntry> entries, const BYTE page)
{
    m_ledger = std::move(entries);
    m_ledgerPage = page;
}

void Store::SetLastResult(const Operation operation, const ResultCode result)
{
    m_lastOperation = operation;
    m_lastResult = result;
    m_hasUnseenResult = true;
}

bool Store::TakeNewResult(Operation& operation, ResultCode& result)
{
    if (!m_hasUnseenResult)
    {
        return false;
    }

    operation = m_lastOperation;
    result = m_lastResult;
    m_hasUnseenResult = false;
    return true;
}

void Store::Clear()
{
    m_balances.fill(0);
    m_offers.clear();
    m_ledger.clear();
    m_offerPage = 0;
    m_offerPageCount = 1;
    ++m_offerGeneration;
    m_ledgerPage = 0;
    m_lastOperation = Operation::Deposit;
    m_lastResult = ResultCode::Success;
    m_hasUnseenResult = false;
}
} // namespace Net::Bank
