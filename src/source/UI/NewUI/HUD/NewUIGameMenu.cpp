// <copyright file="NewUIGameMenu.cpp" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#include "stdafx.h"

#include "UI/NewUI/HUD/NewUIGameMenu.h"

#include "Audio/DSPlaySound.h"
#include "Character/CharacterManager.h"
#include "GameLogic/Commands/ChatCommandCatalog.h"
#include "I18N/All.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Textures/ZzzTexture.h"
#include "UI/Legacy/UIControls.h"
#include "UI/NewUI/Dialogs/NewUICustomMessageBox.h"
#include "UI/NewUI/HUD/NewUIMiniMap.h"
#include "UI/NewUI/Inventory/NewUIBankWindow.h"
#include "UI/NewUI/NewUISystem.h"
#include "UI/NewUI/Widgets/UIPanelStyle.h"
#include "World/MapInfra/MapManager.h"

using namespace SEASON3B;

namespace
{
using namespace UI::PanelStyle;

/// <summary>
/// The window hangs off the row of buttons of the hud: its right edge is the right edge of the
/// screen and its bottom edge is the top of that row, whichever shape it is in. The old window menu
/// stood in exactly this corner, so nothing else has to move out of the way.
/// </summary>
constexpr int MENU_ANCHOR_RIGHT = REFERENCE_WIDTH;
constexpr int MENU_ANCHOR_BOTTOM = REFERENCE_HEIGHT - 51;

/// <summary>The quick grid: three boxes across, two down.</summary>
constexpr int QUICK_MARGIN = 6;
constexpr int QUICK_COLUMNS = 3;
constexpr int QUICK_ROWS = 2;
constexpr int QUICK_TILE_WIDTH = 44;
constexpr int QUICK_TILE_HEIGHT = 40;

/// <summary>The full panel: four boxes across, three down, under a title and a row of tabs.</summary>
constexpr int FULL_MARGIN = 8;
constexpr int FULL_COLUMNS = 4;
constexpr int FULL_ROWS = 3;
constexpr int FULL_TILE_WIDTH = 54;
constexpr int FULL_TILE_HEIGHT = 46;
constexpr int FULL_TITLE_HEIGHT = 22;
constexpr int FULL_TAB_HEIGHT = 18;

/// <summary>How much of the panel is left under the last row of boxes.</summary>
constexpr int FULL_BOTTOM_PADDING = 12;

/// <summary>What stands between two boxes, and between two tabs.</summary>
constexpr int TILE_GAP = 4;

/// <summary>The two small buttons of the title bar.</summary>
constexpr int TITLE_BUTTON_HEIGHT = 14;
constexpr int TITLE_BUTTON_TOP = 4;
constexpr int CLOSE_BUTTON_WIDTH = 14;
constexpr int COLLAPSE_BUTTON_WIDTH = 40;

/// <summary>Where the name of the window is written inside the title bar.</summary>
constexpr int TITLE_TEXT_TOP = 5;

/// <summary>How tall the line under a picture is, and how far it sits off the bottom of its box.</summary>
constexpr int LABEL_HEIGHT = 12;
constexpr int FULL_LABEL_BOTTOM = 14;
constexpr int QUICK_LABEL_BOTTOM = 13;

/// <summary>How big a picture is drawn, and how far under the top of its box it starts.</summary>
constexpr int FULL_ICON_SIZE = 22;
constexpr int FULL_ICON_TOP = 6;
constexpr int QUICK_ICON_SIZE = 20;
constexpr int QUICK_ICON_TOP = 4;

/// <summary>The atlas of pictures: eight boxes across, four down, each of them thirty two square.</summary>
constexpr float ICON_ATLAS_COLUMNS = 8.f;
constexpr float ICON_ATLAS_ROWS = 4.f;

/// <summary>Which box of the atlas each entry takes its picture from.</summary>
enum IconCell
{
    ICON_BANK = 0,
    ICON_MARKET,
    ICON_MOVE_MAP,
    ICON_QUESTS,
    ICON_HELPER,
    ICON_FRIENDS,
    ICON_GUILD,
    ICON_MINI_MAP,
    ICON_CHARACTER,
    ICON_INVENTORY,
    ICON_MASTER_LEVEL,
    ICON_PET,
    ICON_PARTY,
    ICON_GENS,
    ICON_OPTIONS,
    ICON_HELP,
    ICON_COMMANDS,
    ICON_EXIT,
    ICON_SHOW_ALL,
};

/// <summary>The colour a picture is tinted with, which is the colour its name is written in.</summary>
constexpr unsigned int ICON_COLOR = 0xFFDCD6C8u;
constexpr unsigned int ICON_HOVERED_COLOR = 0xFFFFE9A8u;
constexpr unsigned int ICON_OPEN_COLOR = 0xFFFFD24Cu;
constexpr unsigned int ICON_DISABLED_COLOR = 0xFF6F7787u;

/// <summary>Which window an entry opens, or INTERFACE_END when it opens none.</summary>
DWORD GetEntryInterface(CNewUIGameMenu::Entry entry)
{
    using Entry = CNewUIGameMenu::Entry;

    switch (entry)
    {
    case Entry::Bank:
    case Entry::Market:
        return INTERFACE_BANK;
    case Entry::MoveMap:
        return INTERFACE_MOVEMAP;
    case Entry::Quests:
        return INTERFACE_MYQUEST;
    case Entry::Helper:
        return INTERFACE_MUHELPER;
    case Entry::Friends:
        return INTERFACE_FRIEND;
    case Entry::Guild:
        return INTERFACE_GUILDINFO;
    case Entry::MiniMap:
        return INTERFACE_MINI_MAP;
    case Entry::Character:
        return INTERFACE_CHARACTER;
    case Entry::Inventory:
        return INTERFACE_INVENTORY;
    case Entry::MasterLevel:
        return INTERFACE_MASTER_LEVEL;
    case Entry::Pet:
        return INTERFACE_PET;
    case Entry::Party:
        return INTERFACE_PARTY;
    case Entry::Gens:
        return INTERFACE_GENSRANKING;
    case Entry::Options:
        return INTERFACE_OPTION;
    case Entry::Help:
        return INTERFACE_HELP;
    case Entry::Commands:
        return INTERFACE_COMMAND_LIST;
    case Entry::Exit:
    case Entry::ShowAll:
        break;
    }

    return INTERFACE_END;
}
} // namespace

SEASON3B::CNewUIGameMenu::CNewUIGameMenu()
    : m_pNewUIMng(nullptr), m_mode(Mode::Quick), m_group(Group::Highlight), m_hoveredTile(-1), m_iconsLoaded(false)
{
    m_Pos.x = 0;
    m_Pos.y = 0;
    BuildLayout();
}

SEASON3B::CNewUIGameMenu::~CNewUIGameMenu()
{
    Release();
}

bool SEASON3B::CNewUIGameMenu::Create(CNewUIManager* pNewUIMng)
{
    if (pNewUIMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(INTERFACE_GAME_MENU, this);

    InitButton(&m_abtn[BTN_TAB_HIGHLIGHT], &I18N::Game::MenuGroupHighlight);
    InitButton(&m_abtn[BTN_TAB_CHARACTER], &I18N::Game::MenuGroupCharacter);
    InitButton(&m_abtn[BTN_TAB_SOCIAL], &I18N::Game::MenuGroupSocial);
    InitButton(&m_abtn[BTN_TAB_SYSTEM], &I18N::Game::MenuGroupSystem);
    InitButton(&m_abtn[BTN_COLLAPSE], &I18N::Game::MenuCollapse);
    InitButton(&m_abtn[BTN_CLOSE], &I18N::Game::MenuClose);

    LoadImages();
    BuildLayout();

    Show(false);

    return true;
}

void SEASON3B::CNewUIGameMenu::Release()
{
    UnloadImages();

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CNewUIGameMenu::LoadImages()
{
    // The atlas is asked for without the check which pops an error box: a client whose data is one
    // release behind has no atlas, and a menu of names is worth more than a message box on startup.
    m_iconsLoaded =
        LoadBitmap(L"Interface\\newui_menu_icons.tga", BITMAP_GAME_MENU_ICONS, GL_LINEAR, GL_CLAMP_TO_EDGE, false);
}

void SEASON3B::CNewUIGameMenu::UnloadImages()
{
    if (m_iconsLoaded)
    {
        DeleteBitmap(BITMAP_GAME_MENU_ICONS);
        m_iconsLoaded = false;
    }
}

void SEASON3B::CNewUIGameMenu::InitButton(CNewUIButton* pButton, const wchar_t* const* captionSlot)
{
    // No image: this window draws the plate under every caption itself, the way the bank does.
    pButton->ChangeText(captionSlot);
    pButton->ChangeTextBackColor(RGBA(255, 255, 255, 0));
    pButton->ChangeTextColor(RGBA(240, 228, 200, 255));
}

RECT SEASON3B::CNewUIGameMenu::GetWindowRect() const
{
    RECT rect = {};

    if (m_mode == Mode::Quick)
    {
        const int width = QUICK_MARGIN * 2 + QUICK_COLUMNS * QUICK_TILE_WIDTH + (QUICK_COLUMNS - 1) * TILE_GAP;
        const int height = QUICK_MARGIN * 2 + QUICK_ROWS * QUICK_TILE_HEIGHT + (QUICK_ROWS - 1) * TILE_GAP;

        rect.left = MENU_ANCHOR_RIGHT - width;
        rect.top = MENU_ANCHOR_BOTTOM - height;
        rect.right = MENU_ANCHOR_RIGHT;
        rect.bottom = MENU_ANCHOR_BOTTOM;
        return rect;
    }

    const int width = FULL_MARGIN * 2 + FULL_COLUMNS * FULL_TILE_WIDTH + (FULL_COLUMNS - 1) * TILE_GAP;
    const int height = FULL_TITLE_HEIGHT + FULL_TAB_HEIGHT + FULL_MARGIN + FULL_ROWS * FULL_TILE_HEIGHT +
                       (FULL_ROWS - 1) * TILE_GAP + FULL_BOTTOM_PADDING;

    rect.left = MENU_ANCHOR_RIGHT - width;
    rect.top = MENU_ANCHOR_BOTTOM - height;
    rect.right = MENU_ANCHOR_RIGHT;
    rect.bottom = MENU_ANCHOR_BOTTOM;
    return rect;
}

void SEASON3B::CNewUIGameMenu::BuildLayout()
{
    const RECT rect = GetWindowRect();
    m_Pos.x = rect.left;
    m_Pos.y = rect.top;

    ApplyLayoutToButtons();
}

void SEASON3B::CNewUIGameMenu::ApplyLayoutToButtons()
{
    const RECT rect = GetWindowRect();
    const int width = rect.right - rect.left;
    const int tabWidth = (width - FULL_MARGIN * 2 - (FULL_COLUMNS - 1) * TILE_GAP) / FULL_COLUMNS;
    const int tabTop = rect.top + FULL_TITLE_HEIGHT;

    for (int i = 0; i < FULL_COLUMNS; ++i)
    {
        m_abtn[BTN_TAB_HIGHLIGHT + i].ChangeButtonInfo(rect.left + FULL_MARGIN + i * (tabWidth + TILE_GAP), tabTop,
                                                       tabWidth, FULL_TAB_HEIGHT);
    }

    const int closeLeft = rect.right - FULL_MARGIN - CLOSE_BUTTON_WIDTH;
    m_abtn[BTN_CLOSE].ChangeButtonInfo(closeLeft, rect.top + TITLE_BUTTON_TOP, CLOSE_BUTTON_WIDTH, TITLE_BUTTON_HEIGHT);
    m_abtn[BTN_COLLAPSE].ChangeButtonInfo(closeLeft - TILE_GAP - COLLAPSE_BUTTON_WIDTH, rect.top + TITLE_BUTTON_TOP,
                                          COLLAPSE_BUTTON_WIDTH, TITLE_BUTTON_HEIGHT);
}

std::span<const SEASON3B::CNewUIGameMenu::EntryInfo> SEASON3B::CNewUIGameMenu::GetShownEntries() const
{
    // What the quick grid holds is fixed for now; letting the player choose it is what the pinning
    // of the next step is about. The box "All" always sits last, so it keeps its place when the
    // five before it change.
    static const EntryInfo QUICK[] = {
        {Entry::Bank, ICON_BANK, &I18N::Game::MenuBank},
        {Entry::Market, ICON_MARKET, &I18N::Game::MenuMarket},
        {Entry::MoveMap, ICON_MOVE_MAP, &I18N::Game::MenuMoveMap},
        {Entry::Quests, ICON_QUESTS, &I18N::Game::MenuQuests},
        {Entry::Helper, ICON_HELPER, &I18N::Game::MenuHelper},
        {Entry::ShowAll, ICON_SHOW_ALL, &I18N::Game::MenuAll},
    };

    static const EntryInfo HIGHLIGHT[] = {
        {Entry::Bank, ICON_BANK, &I18N::Game::MenuBank},
        {Entry::Market, ICON_MARKET, &I18N::Game::MenuMarket},
        {Entry::MoveMap, ICON_MOVE_MAP, &I18N::Game::MenuMoveMap},
        {Entry::Quests, ICON_QUESTS, &I18N::Game::MenuQuests},
        {Entry::Helper, ICON_HELPER, &I18N::Game::MenuHelper},
        {Entry::Friends, ICON_FRIENDS, &I18N::Game::MenuFriends},
        {Entry::Guild, ICON_GUILD, &I18N::Game::MenuGuild},
        {Entry::MiniMap, ICON_MINI_MAP, &I18N::Game::MenuMiniMap},
    };

    static const EntryInfo CHARACTER[] = {
        {Entry::Character, ICON_CHARACTER, &I18N::Game::MenuCharacter},
        {Entry::Inventory, ICON_INVENTORY, &I18N::Game::MenuInventory},
        {Entry::MasterLevel, ICON_MASTER_LEVEL, &I18N::Game::MenuMasterLevel},
        {Entry::Pet, ICON_PET, &I18N::Game::MenuPet},
        {Entry::Quests, ICON_QUESTS, &I18N::Game::MenuQuests},
        {Entry::Helper, ICON_HELPER, &I18N::Game::MenuHelper},
    };

    static const EntryInfo SOCIAL[] = {
        {Entry::Friends, ICON_FRIENDS, &I18N::Game::MenuFriends},
        {Entry::Guild, ICON_GUILD, &I18N::Game::MenuGuild},
        {Entry::Party, ICON_PARTY, &I18N::Game::MenuParty},
        {Entry::Gens, ICON_GENS, &I18N::Game::MenuGens},
    };

    static const EntryInfo SYSTEM[] = {
        {Entry::Options, ICON_OPTIONS, &I18N::Game::MenuOptions},
        {Entry::Help, ICON_HELP, &I18N::Game::MenuHelp},
        {Entry::Commands, ICON_COMMANDS, &I18N::Game::MenuCommands},
        {Entry::MiniMap, ICON_MINI_MAP, &I18N::Game::MenuMiniMap},
        {Entry::MoveMap, ICON_MOVE_MAP, &I18N::Game::MenuMoveMap},
        {Entry::Exit, ICON_EXIT, &I18N::Game::MenuExit},
    };

    if (m_mode == Mode::Quick)
    {
        return std::span<const EntryInfo>(QUICK, QUICK_ENTRY_COUNT);
    }

    switch (m_group)
    {
    case Group::Character:
        return std::span<const EntryInfo>(CHARACTER, std::size(CHARACTER));
    case Group::Social:
        return std::span<const EntryInfo>(SOCIAL, std::size(SOCIAL));
    case Group::System:
        return std::span<const EntryInfo>(SYSTEM, std::size(SYSTEM));
    case Group::Highlight:
        break;
    }

    return std::span<const EntryInfo>(HIGHLIGHT, std::size(HIGHLIGHT));
}

SEASON3B::CNewUIGameMenu::GridGeometry SEASON3B::CNewUIGameMenu::GetGridGeometry() const
{
    GridGeometry geometry = {};

    if (m_mode == Mode::Quick)
    {
        geometry.originX = m_Pos.x + QUICK_MARGIN;
        geometry.originY = m_Pos.y + QUICK_MARGIN;
        geometry.columns = QUICK_COLUMNS;
        geometry.tileWidth = QUICK_TILE_WIDTH;
        geometry.tileHeight = QUICK_TILE_HEIGHT;
        return geometry;
    }

    geometry.originX = m_Pos.x + FULL_MARGIN;
    geometry.originY = m_Pos.y + FULL_TITLE_HEIGHT + FULL_TAB_HEIGHT + FULL_MARGIN;
    geometry.columns = FULL_COLUMNS;
    geometry.tileWidth = FULL_TILE_WIDTH;
    geometry.tileHeight = FULL_TILE_HEIGHT;
    return geometry;
}

RECT SEASON3B::CNewUIGameMenu::GetTileRect(int index) const
{
    // Drawing a box and clicking a box read the same numbers, so a box can never be drawn in one
    // place and answer in another.
    const GridGeometry geometry = GetGridGeometry();
    const int column = index % geometry.columns;
    const int row = index / geometry.columns;

    RECT rect = {};
    rect.left = geometry.originX + column * (geometry.tileWidth + TILE_GAP);
    rect.top = geometry.originY + row * (geometry.tileHeight + TILE_GAP);
    rect.right = rect.left + geometry.tileWidth;
    rect.bottom = rect.top + geometry.tileHeight;
    return rect;
}

bool SEASON3B::CNewUIGameMenu::IsEntryEnabled(Entry entry)
{
    switch (entry)
    {
    case Entry::MasterLevel:
        return gCharacterManager.IsMasterLevel(Hero->Class) == true && Hero->Class != CLASS_TEMPLENIGHT;
    case Entry::Commands:
        return GameLogic::Commands::Catalog().IsAvailable();
    case Entry::MiniMap:
        return g_pNewUIMiniMap != nullptr && g_pNewUIMiniMap->m_bSuccess;
    case Entry::Friends:
        return gMapManager.InChaosCastle() == false;
    default:
        break;
    }

    return true;
}

bool SEASON3B::CNewUIGameMenu::IsEntryOpen(Entry entry)
{
    if (entry == Entry::Market)
    {
        return g_pNewUISystem->IsVisible(INTERFACE_BANK) && g_pBankWindow != nullptr && g_pBankWindow->IsMarketPage();
    }

    if (entry == Entry::Bank)
    {
        return g_pNewUISystem->IsVisible(INTERFACE_BANK) &&
               (g_pBankWindow == nullptr || g_pBankWindow->IsMarketPage() == false);
    }

    const DWORD window = GetEntryInterface(entry);
    if (window == INTERFACE_END)
    {
        return false;
    }

    return g_pNewUISystem->IsVisible(window);
}

void SEASON3B::CNewUIGameMenu::Activate(Entry entry)
{
    PlayBuffer(SOUND_CLICK01);

    if (entry == Entry::ShowAll)
    {
        m_mode = Mode::Full;
        m_hoveredTile = -1;
        BuildLayout();
        return;
    }

    // Every branch below does what the shortcut of that window does, guards included: a box of the
    // menu is that shortcut made visible, and two ways into one window which disagree are a bug
    // waiting to be reported.
    switch (entry)
    {
    case Entry::Bank:
        g_pNewUISystem->Toggle(INTERFACE_BANK);
        break;

    case Entry::Market:
        if (g_pNewUISystem->IsVisible(INTERFACE_BANK) == false)
        {
            g_pNewUISystem->Show(INTERFACE_BANK);
        }

        if (g_pBankWindow)
        {
            g_pBankWindow->ShowMarketPage();
        }
        break;

    case Entry::MoveMap:
        g_pNewUISystem->Toggle(INTERFACE_MOVEMAP);
        break;

    case Entry::Quests:
        g_pNewUISystem->Toggle(INTERFACE_MYQUEST);
        break;

    case Entry::Helper:
        g_pNewUISystem->Toggle(INTERFACE_MUHELPER);
        break;

    case Entry::Friends:
        if (CharacterAttribute->Level < 6)
        {
            if (g_pSystemLogBox->CheckChatRedundancy(I18N::Game::YouMustBeAtLeastLevel6ToUseTheMyFriendFunction) ==
                FALSE)
            {
                g_pSystemLogBox->AddText(I18N::Game::YouMustBeAtLeastLevel6ToUseTheMyFriendFunction,
                                         SEASON3B::TYPE_SYSTEM_MESSAGE);
            }
        }
        else
        {
            g_pNewUISystem->Toggle(INTERFACE_FRIEND);
        }
        break;

    case Entry::Guild:
        g_pNewUISystem->Toggle(INTERFACE_GUILDINFO);
        break;

    case Entry::MiniMap:
        g_pNewUISystem->Toggle(INTERFACE_MINI_MAP);
        break;

    case Entry::Character:
        g_pNewUISystem->Toggle(INTERFACE_CHARACTER);
        break;

    case Entry::Inventory:
        if (g_pNPCShop->IsSellingItem() == false)
        {
            g_pNewUISystem->Toggle(INTERFACE_INVENTORY);
        }
        break;

    case Entry::MasterLevel:
        g_pNewUISystem->Toggle(INTERFACE_MASTER_LEVEL);
        break;

    case Entry::Pet:
        g_pNewUISystem->Toggle(INTERFACE_PET);
        break;

    case Entry::Party:
        g_pNewUISystem->Toggle(INTERFACE_PARTY);
        break;

    case Entry::Gens:
        // Asking for the standing is what decides whether there is anything to show.
        if (g_pNewUIGensRanking->SetGensInfo())
        {
            g_pNewUISystem->Toggle(INTERFACE_GENSRANKING);
        }
        break;

    case Entry::Options:
        g_pNewUISystem->Show(INTERFACE_OPTION);
        break;

    case Entry::Help:
        g_pNewUISystem->Toggle(INTERFACE_HELP);
        break;

    case Entry::Commands:
        g_pNewUISystem->Toggle(INTERFACE_COMMAND_LIST);
        break;

    case Entry::Exit:
        g_pNewUISystem->Hide(INTERFACE_GAME_MENU);
        SEASON3B::CreateMessageBox(MSGBOX_LAYOUT_CLASS(SEASON3B::CSystemMenuMsgBoxLayout));
        return;

    case Entry::ShowAll:
        break;
    }

    // A box did what it stands for, so the menu has said everything it had to say.
    g_pNewUISystem->Hide(INTERFACE_GAME_MENU);
}

bool SEASON3B::CNewUIGameMenu::ProcessButtons()
{
    for (int i = 0; i < FULL_COLUMNS; ++i)
    {
        if (m_abtn[BTN_TAB_HIGHLIGHT + i].UpdateMouseEvent())
        {
            m_group = static_cast<Group>(i);
            m_hoveredTile = -1;
            PlayBuffer(SOUND_CLICK01);
            return true;
        }
    }

    if (m_abtn[BTN_COLLAPSE].UpdateMouseEvent())
    {
        m_mode = Mode::Quick;
        m_hoveredTile = -1;
        BuildLayout();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_abtn[BTN_CLOSE].UpdateMouseEvent())
    {
        g_pNewUISystem->Hide(INTERFACE_GAME_MENU);
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    return false;
}

bool SEASON3B::CNewUIGameMenu::ProcessTiles()
{
    const std::span<const EntryInfo> entries = GetShownEntries();

    m_hoveredTile = -1;

    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        const RECT rect = GetTileRect(i);
        if (SEASON3B::CheckMouseIn(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top) == false)
        {
            continue;
        }

        m_hoveredTile = i;

        if (SEASON3B::IsRelease(VK_LBUTTON))
        {
            if (IsEntryEnabled(entries[i].entry))
            {
                Activate(entries[i].entry);
            }
            else
            {
                // A box which cannot be used says so by staying where it is, and by the click
                // sounding like nothing happened - because nothing did.
                PlayBuffer(SOUND_CLICK01);
            }
        }

        return true;
    }

    return false;
}

bool SEASON3B::CNewUIGameMenu::UpdateMouseEvent()
{
    if (IsVisible() == false)
    {
        return true;
    }

    if (m_mode == Mode::Full && ProcessButtons())
    {
        return false;
    }

    if (ProcessTiles())
    {
        return false;
    }

    const RECT rect = GetWindowRect();
    if (SEASON3B::CheckMouseIn(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top))
    {
        // The window swallows what happens over it, so a click beside a box does not walk the
        // character to the other side of the map.
        return false;
    }

    // The quick grid is a popup, so a click beside it puts it away; the full panel is a window and
    // stays. A click on the hud itself is left alone either way, or the menu button would close the
    // menu here and open it again a moment later.
    const bool overHud =
        SEASON3B::CheckMouseIn(0, MENU_ANCHOR_BOTTOM, REFERENCE_WIDTH, REFERENCE_HEIGHT - MENU_ANCHOR_BOTTOM);
    if (m_mode == Mode::Quick && SEASON3B::IsRelease(VK_LBUTTON) && overHud == false)
    {
        g_pNewUISystem->Hide(INTERFACE_GAME_MENU);
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIGameMenu::UpdateKeyEvent()
{
    if (IsVisible() == false)
    {
        return true;
    }

    if (SEASON3B::IsPress(VK_ESCAPE) == false)
    {
        return true;
    }

    // Escape steps back one layer at a time: the panel gives way to the quick grid, and the quick
    // grid gives way to the game.
    if (m_mode == Mode::Full)
    {
        m_mode = Mode::Quick;
        m_hoveredTile = -1;
        BuildLayout();
    }
    else
    {
        g_pNewUISystem->Hide(INTERFACE_GAME_MENU);
    }

    PlayBuffer(SOUND_CLICK01);
    return false;
}

bool SEASON3B::CNewUIGameMenu::Update()
{
    return true;
}

void SEASON3B::CNewUIGameMenu::OpeningProcess()
{
    // The button always opens the shape the player last left, which is the quick grid unless he
    // went looking for something in the panel and closed it there.
    m_hoveredTile = -1;
    BuildLayout();
}

void SEASON3B::CNewUIGameMenu::ClosingProcess()
{
    m_hoveredTile = -1;
}

bool SEASON3B::CNewUIGameMenu::Render()
{
    if (IsVisible() == false)
    {
        return true;
    }

    EnableAlphaTest();

    RenderFrame();

    if (m_mode == Mode::Full)
    {
        RenderTabs();
    }

    RenderTiles();

    DisableAlphaBlend();
    return true;
}

void SEASON3B::CNewUIGameMenu::RenderFrame()
{
    const RECT rect = GetWindowRect();
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;

    RenderColorQuadARGB(static_cast<float>(rect.left), static_cast<float>(rect.top), static_cast<float>(width),
                        static_cast<float>(height), PANEL_COLOR);
    RenderBorder(rect.left, rect.top, width, height, PANEL_EDGE_COLOR, 2);
    RenderBorder(rect.left + 2, rect.top + 2, width - 4, height - 4, PANEL_INNER_COLOR, 1);

    if (m_mode == Mode::Full)
    {
        RenderColorQuadARGB(static_cast<float>(rect.left + FULL_MARGIN),
                            static_cast<float>(rect.top + FULL_TITLE_HEIGHT - 1),
                            static_cast<float>(width - FULL_MARGIN * 2), 1.f, HEADING_LINE_COLOR);
    }

    EndRenderColor();

    if (m_mode == Mode::Full)
    {
        g_pRenderText->SetFont(g_hFontBold);
        g_pRenderText->SetBgColor(0);
        UseTextColor(TITLE_TEXT);
        g_pRenderText->RenderText(rect.left, rect.top + TITLE_TEXT_TOP, I18N::Game::Menu, width, 0, RT3_SORT_CENTER);
    }
}

void SEASON3B::CNewUIGameMenu::RenderButton(CNewUIButton& button, bool highlighted)
{
    const POINT& pos = button.GetPos();
    const POINT& size = button.GetSize();
    const BUTTON_STATE state = button.GetBTState();

    unsigned int plate = BUTTON_UP_COLOR;
    if (state == BUTTON_STATE_DOWN)
    {
        plate = BUTTON_DOWN_COLOR;
    }
    else if (state == BUTTON_STATE_OVER)
    {
        plate = BUTTON_OVER_COLOR;
    }

    RenderColorQuadARGB(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(size.x),
                        static_cast<float>(size.y), plate);
    RenderBorder(pos.x, pos.y, size.x, size.y, highlighted ? BUTTON_EDGE_ON_COLOR : BUTTON_EDGE_COLOR, 1);
    EndRenderColor();

    button.ChangeTextColor(highlighted ? RGBA(255, 210, 76, 255) : RGBA(232, 220, 192, 255));
    button.Render();
}

void SEASON3B::CNewUIGameMenu::RenderTabs()
{
    for (int i = 0; i < FULL_COLUMNS; ++i)
    {
        RenderButton(m_abtn[BTN_TAB_HIGHLIGHT + i], static_cast<int>(m_group) == i);
    }

    RenderButton(m_abtn[BTN_COLLAPSE], false);
    RenderButton(m_abtn[BTN_CLOSE], false);
}

void SEASON3B::CNewUIGameMenu::RenderTiles()
{
    const std::span<const EntryInfo> entries = GetShownEntries();

    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        const EntryInfo& info = entries[i];
        const bool enabled = IsEntryEnabled(info.entry);

        RenderTile(info, GetTileRect(i), m_hoveredTile == i, IsEntryOpen(info.entry), enabled);
    }
}

void SEASON3B::CNewUIGameMenu::RenderTile(const EntryInfo& info, const RECT& rect, bool hovered, bool open,
                                          bool enabled)
{
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    const bool quick = m_mode == Mode::Quick;

    unsigned int plate = TILE_COLOR;
    unsigned int edge = TILE_LINE_COLOR;

    if (info.entry == Entry::ShowAll)
    {
        plate = TILE_ACCENT_COLOR;
        edge = BUTTON_EDGE_COLOR;
    }

    if (enabled == false)
    {
        plate = TILE_DISABLED_COLOR;
        edge = TILE_DISABLED_LINE_COLOR;
    }

    RenderColorQuadARGB(static_cast<float>(rect.left), static_cast<float>(rect.top), static_cast<float>(width),
                        static_cast<float>(height), plate);

    if (open)
    {
        RenderColorQuadARGB(static_cast<float>(rect.left), static_cast<float>(rect.top), static_cast<float>(width),
                            static_cast<float>(height), PICKED_FILL_COLOR);
        edge = PICKED_EDGE_COLOR;
    }
    else if (hovered && enabled)
    {
        RenderColorQuadARGB(static_cast<float>(rect.left), static_cast<float>(rect.top), static_cast<float>(width),
                            static_cast<float>(height), HOVERED_FILL_COLOR);
        edge = BUTTON_EDGE_COLOR;
    }

    RenderBorder(rect.left, rect.top, width, height, edge, 1);
    EndRenderColor();

    unsigned int iconColor = ICON_COLOR;
    TextColor labelColor = ROW_TEXT;

    if (enabled == false)
    {
        iconColor = ICON_DISABLED_COLOR;
        labelColor = DISABLED_TEXT;
    }
    else if (open)
    {
        iconColor = ICON_OPEN_COLOR;
        labelColor = HIGHLIGHT_TEXT;
    }
    else if (hovered)
    {
        iconColor = ICON_HOVERED_COLOR;
        labelColor = PICKED_ROW_TEXT;
    }

    if (m_iconsLoaded)
    {
        const int iconSize = quick ? QUICK_ICON_SIZE : FULL_ICON_SIZE;
        const int iconTop = rect.top + (quick ? QUICK_ICON_TOP : FULL_ICON_TOP);
        const int iconLeft = rect.left + (width - iconSize) / 2;
        const float u = static_cast<float>(info.iconCell % static_cast<int>(ICON_ATLAS_COLUMNS)) / ICON_ATLAS_COLUMNS;
        const float v = static_cast<float>(info.iconCell / static_cast<int>(ICON_ATLAS_COLUMNS)) / ICON_ATLAS_ROWS;

        RenderColorBitmap(BITMAP_GAME_MENU_ICONS, static_cast<float>(iconLeft), static_cast<float>(iconTop),
                          static_cast<float>(iconSize), static_cast<float>(iconSize), u, v, 1.f / ICON_ATLAS_COLUMNS,
                          1.f / ICON_ATLAS_ROWS, iconColor);
    }

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);
    UseTextColor(labelColor);

    const int labelTop = rect.bottom - (quick ? QUICK_LABEL_BOTTOM : FULL_LABEL_BOTTOM);
    g_pRenderText->RenderText(rect.left, labelTop, *info.label, width, LABEL_HEIGHT, RT3_SORT_CENTER);
}

float SEASON3B::CNewUIGameMenu::GetLayerDepth()
{
    return 10.0f;
}

float SEASON3B::CNewUIGameMenu::GetKeyEventOrder()
{
    return 10.0f;
}
