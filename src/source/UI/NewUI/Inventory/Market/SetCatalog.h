//*****************************************************************************
// File: SetCatalog.h
//*****************************************************************************

#pragma once

#include "Core/Platform/WinCompat.h"

#include <string>
#include <vector>

namespace UI::Market
{
/// <summary>One set of armour, as the dropdown which narrows the market offers it.</summary>
struct ArmorSet
{
    /// <summary>What the set is called, which is what the names of its pieces have in common.</summary>
    std::wstring Name;

    /// <summary>The number its pieces share within their groups, which is what a search sends.</summary>
    BYTE Number = 0;

    /// <summary>Which families of character classes may wear it, as one bit per family.</summary>
    int ClassMask = 0;
};

/// <summary>Reads the sets of armour out of the item table of the client.</summary>
/// <returns>One entry per set which has a piece, in the order the sets are numbered.</returns>
/// <remarks>
/// A set carries its name nowhere of its own: its five pieces carry it, each with the word for the
/// piece around it - "Dragon Helm" beside "Dragon Armor", or "Mu Rong" beside "Ao Rong". The name
/// of the set is therefore what the names of its pieces have in common, which is the one way of
/// reading it which does not depend on the language the client was started in, and which follows
/// the item list of the server instead of a table written out a second time here.
/// </remarks>
std::vector<ArmorSet> ReadArmorSets();
} // namespace UI::Market
