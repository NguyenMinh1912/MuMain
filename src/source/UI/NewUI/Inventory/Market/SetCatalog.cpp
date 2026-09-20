//*****************************************************************************
// File: SetCatalog.cpp
//*****************************************************************************

#include "stdafx.h"

#include "UI/NewUI/Inventory/Market/SetCatalog.h"

#include "Core/Globals/_define.h"
#include "Data/GameData/ItemData/ItemStructs.h"

#include <algorithm>

extern ITEM_ATTRIBUTE* ItemAttribute;

namespace
{
/// <summary>The groups the five pieces of a set of armour are in: helm, armour, pants, gloves, boots.</summary>
constexpr int FIRST_ARMOR_GROUP = 7;
constexpr int LAST_ARMOR_GROUP = 11;

/// <summary>What stands between two words of the name of an item.</summary>
constexpr wchar_t WORD_SEPARATOR = L' ';

/// <summary>Cuts a name into the words it is made of.</summary>
std::vector<std::wstring> SplitWords(const std::wstring& name)
{
    std::vector<std::wstring> words;
    size_t start = 0;
    while (start < name.size())
    {
        const size_t end = name.find(WORD_SEPARATOR, start);
        const std::wstring word = name.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
        if (!word.empty())
        {
            words.push_back(word);
        }

        if (end == std::wstring::npos)
        {
            break;
        }

        start = end + 1;
    }

    return words;
}

/// <summary>
/// Keeps the words which two names both have, in the order the first one has them.
/// </summary>
/// <remarks>
/// It is the word for the piece which differs between the names of one set, and it stands in front
/// of the name of the set in one language and behind it in another. Keeping what they share needs
/// to know neither.
/// </remarks>
std::wstring CommonWords(const std::wstring& name, const std::wstring& other)
{
    const std::vector<std::wstring> otherWords = SplitWords(other);
    std::wstring common;

    for (const std::wstring& word : SplitWords(name))
    {
        if (std::find(otherWords.begin(), otherWords.end(), word) == otherWords.end())
        {
            continue;
        }

        if (!common.empty())
        {
            common += WORD_SEPARATOR;
        }

        common += word;
    }

    return common;
}

/// <summary>Gets which families of character classes may wear an item, as one bit per family.</summary>
int GetClassMask(const ITEM_ATTRIBUTE& attribute)
{
    int mask = 0;
    for (int family = 0; family < MAX_CLASS; ++family)
    {
        if (attribute.RequireClass[family] != 0)
        {
            mask |= 1 << family;
        }
    }

    return mask;
}
} // namespace

std::vector<UI::Market::ArmorSet> UI::Market::ReadArmorSets()
{
    std::vector<ArmorSet> sets;
    if (ItemAttribute == nullptr)
    {
        return sets;
    }

    for (int number = 0; number < MAX_ITEM_INDEX; ++number)
    {
        ArmorSet set;
        set.Number = static_cast<BYTE>(number);
        std::wstring firstPieceName;

        for (int group = FIRST_ARMOR_GROUP; group <= LAST_ARMOR_GROUP; ++group)
        {
            const ITEM_ATTRIBUTE& piece = ItemAttribute[group * MAX_ITEM_INDEX + number];
            if (piece.Name[0] == L'\0')
            {
                continue;
            }

            set.ClassMask |= GetClassMask(piece);
            set.Name = firstPieceName.empty() ? piece.Name : CommonWords(set.Name, piece.Name);
            if (firstPieceName.empty())
            {
                firstPieceName = piece.Name;
            }
        }

        if (firstPieceName.empty())
        {
            continue;
        }

        if (set.Name.empty())
        {
            // A set whose pieces share no word at all is named after the first of them, which is
            // still closer to what a player calls it than a number would be.
            set.Name = firstPieceName;
        }

        sets.push_back(std::move(set));
    }

    return sets;
}
