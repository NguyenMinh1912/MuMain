// <copyright file="NewUIGameMenu.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include "UI/NewUI/NewUIBase.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

#include <array>
#include <span>

namespace SEASON3B
{
/// <summary>
/// One way into every window of the game: boxes in a grid, opened by the menu button of the hud.
/// </summary>
/// <remarks>
/// Not part of the original client, and it owns no feature of its own. Every box opens a window
/// which already exists, the way the shortcut of that window opens it, with the same guards - a box
/// is a shortcut the player can see.
///
/// Two shapes, one window: the quick grid of three by two which the button opens, and the full
/// panel of four by three behind its box "All". They never stand open at the same time and they
/// draw the same boxes, so splitting them into two windows would only duplicate the drawing.
///
/// The window draws itself out of plain quads and one atlas of icons, like the bank. It survives
/// the atlas being absent: a box without its picture still shows its name.
/// </remarks>
class CNewUIGameMenu : public CNewUIObj
{
public:
    /// <summary>Which of the two shapes the window is showing.</summary>
    enum class Mode
    {
        /// <summary>Three by two boxes the player pinned, over the menu button.</summary>
        Quick,

        /// <summary>Four by three boxes in four groups, opened by the box "All".</summary>
        Full,
    };

    /// <summary>The groups the full panel is divided into, one tab each.</summary>
    enum class Group
    {
        Highlight,
        Character,
        Social,
        System,
    };

    /// <summary>
    /// What a box stands for. The window knows nothing about a box beyond this value: which window
    /// it opens, whether it can be used and what happens when it cannot are all decided from it.
    /// </summary>
    enum class Entry
    {
        Events,
        Bank,
        Market,
        MoveMap,
        Quests,
        Helper,
        Friends,
        Guild,
        MiniMap,
        Character,
        Inventory,
        MasterLevel,
        Pet,
        Party,
        Gens,
        Options,
        Help,
        Commands,
        Exit,

        /// <summary>Not a window: the box which turns the quick grid into the full panel.</summary>
        ShowAll,
    };

    CNewUIGameMenu();
    ~CNewUIGameMenu() override;

    bool Create(CNewUIManager* pNewUIMng);
    void Release();

    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;

    float GetLayerDepth() override;    //. 10.0f
    float GetKeyEventOrder() override; //. 10.0f

    /// <summary>Opens the window on the quick grid, which is what the menu button asks for.</summary>
    void OpeningProcess();

    /// <summary>Forgets what the cursor was over, so a reopened window starts clean.</summary>
    void ClosingProcess();

private:
    /// <summary>How many boxes the quick grid holds, the box "All" included.</summary>
    static constexpr int QUICK_ENTRY_COUNT = 6;

    /// <summary>What one box is, on either of the two shapes.</summary>
    struct EntryInfo
    {
        Entry entry;

        /// <summary>Which box of the atlas holds its picture.</summary>
        int iconCell;

        /// <summary>
        /// Where its name is kept, and not the name itself: the pointer behind it is replaced when
        /// the player changes the language, so the box has to read it again every time it draws.
        /// </summary>
        const wchar_t* const* label;
    };

    /// <summary>Where the boxes of one shape stand.</summary>
    struct GridGeometry
    {
        int originX;
        int originY;
        int columns;
        int tileWidth;
        int tileHeight;
    };

    void LoadImages();
    void UnloadImages();

    void BuildLayout();
    void ApplyLayoutToButtons();
    void InitButton(CNewUIButton* pButton, const wchar_t* const* captionSlot);

    std::span<const EntryInfo> GetShownEntries() const;
    GridGeometry GetGridGeometry() const;
    RECT GetTileRect(int index) const;
    RECT GetWindowRect() const;

    void RenderFrame();
    void RenderTabs();
    void RenderTiles();
    void RenderTile(const EntryInfo& info, const RECT& rect, bool hovered, bool open, bool enabled);
    void RenderButton(CNewUIButton& button, bool highlighted);

    bool ProcessButtons();
    bool ProcessTiles();

    /// <summary>Tells whether the window a box opens can be opened at all right now.</summary>
    static bool IsEntryEnabled(Entry entry);

    /// <summary>Tells whether the window a box opens is the one standing open.</summary>
    static bool IsEntryOpen(Entry entry);

    /// <summary>Does what the shortcut of that window does, and closes the menu after it.</summary>
    void Activate(Entry entry);

    CNewUIManager* m_pNewUIMng;
    POINT m_Pos;
    Mode m_mode;
    Group m_group;

    /// <summary>Which box the cursor is over, or -1.</summary>
    int m_hoveredTile;

    /// <summary>Whether the atlas of icons was there; without it the boxes show their names only.</summary>
    bool m_iconsLoaded;

    enum MENU_BUTTON
    {
        BTN_TAB_HIGHLIGHT = 0,
        BTN_TAB_CHARACTER,
        BTN_TAB_SOCIAL,
        BTN_TAB_SYSTEM,
        BTN_COLLAPSE,
        BTN_CLOSE,
        MENU_BUTTON_COUNT,
    };

    std::array<CNewUIButton, MENU_BUTTON_COUNT> m_abtn;
};
} // namespace SEASON3B
