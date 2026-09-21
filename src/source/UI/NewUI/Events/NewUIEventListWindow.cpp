//*****************************************************************************
// File: NewUIEventListWindow.cpp
//*****************************************************************************

#include "stdafx.h"

#include "UI/NewUI/Events/NewUIEventListWindow.h"

#include "Audio/DSPlaySound.h"
#include "I18N/All.h"
#include "Network/Server/EventStore.h"
#include "Network/Server/ServerListManager.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Chat/Chat.h"
#include "UI/NewUI/NewUICommon.h"
#include "UI/NewUI/NewUISystem.h"

#include <algorithm>

namespace
{
/// <summary>The body of the window, dark enough for the world behind it to stay behind it.</summary>
constexpr unsigned int PANEL_COLOR = 0xEE0E0F12u;

/// <summary>The bronze edge of the window and the darker line just inside it.</summary>
constexpr unsigned int PANEL_EDGE_COLOR = 0xFF6B5A3Cu;
constexpr unsigned int PANEL_INNER_COLOR = 0xFF241F16u;

/// <summary>The row the cursor is over, and the row whose description is written out.</summary>
constexpr unsigned int HOVERED_ROW_COLOR = 0x22FFFFFFu;
constexpr unsigned int SELECTED_ROW_COLOR = 0x40FFC83Cu;

/// <summary>The plate of a button, in its three states, and its edge.</summary>
constexpr unsigned int BUTTON_UP_COLOR = 0xFF433D31u;
constexpr unsigned int BUTTON_OVER_COLOR = 0xFF564E3Eu;
constexpr unsigned int BUTTON_DOWN_COLOR = 0xFF2B261Du;
constexpr unsigned int BUTTON_EDGE_COLOR = 0xFF8A7444u;
constexpr unsigned int BUTTON_EDGE_ON_COLOR = 0xFFFFC83Cu;

/// <summary>The line which separates the column headings from the rows under them.</summary>
constexpr unsigned int HEADING_LINE_COLOR = 0x804A5566u;

/// <summary>A colour text is written in.</summary>
struct TextColor
{
    int red;
    int green;
    int blue;
    int alpha;
};

/// <summary>The name of the window.</summary>
constexpr TextColor TITLE_TEXT{240, 220, 164, 255};

/// <summary>The names of the columns.</summary>
constexpr TextColor HEADING_TEXT{143, 184, 232, 255};

/// <summary>A row which is neither open nor running.</summary>
constexpr TextColor ROW_TEXT{220, 214, 200, 255};

/// <summary>An event which can be entered right now.</summary>
constexpr TextColor OPEN_TEXT{126, 206, 134, 255};

/// <summary>An event which runs, but whose entrance is shut.</summary>
constexpr TextColor RUNNING_TEXT{255, 210, 76, 255};

/// <summary>An event which never starts by itself.</summary>
constexpr TextColor UNSCHEDULED_TEXT{150, 146, 138, 255};

/// <summary>The description of the selected event, and the number of the page.</summary>
constexpr TextColor LABEL_TEXT{200, 194, 180, 255};

/// <summary>The line which says that there is nothing to show.</summary>
constexpr TextColor EMPTY_LIST_TEXT{180, 180, 180, 255};

/// <summary>What the frame keeps for itself, and what stands between two things side by side.</summary>
constexpr int EDGE = 10;
constexpr int GAP = 6;

/// <summary>How tall the strip which holds the name of the window is.</summary>
constexpr int TITLE_HEIGHT = 28;

/// <summary>How tall one row of the list is, and how tall the row of buttons at the bottom is.</summary>
constexpr int ROW_HEIGHT = 22;
constexpr int BUTTON_HEIGHT = 20;

/// <summary>How wide the buttons at the bottom are.</summary>
constexpr int PAGE_BUTTON_WIDTH = 52;
constexpr int REFRESH_BUTTON_WIDTH = 68;

/// <summary>How wide the button which joins an event is.</summary>
constexpr int JOIN_BUTTON_WIDTH = 46;
constexpr int JOIN_BUTTON_HEIGHT = 18;

/// <summary>How far under the top of its row a line of the list sits.</summary>
constexpr int ROW_TEXT_TOP = 3;

/// <summary>How far under a heading the line which underlines it is drawn.</summary>
constexpr int HEADING_LINE_TOP = 16;

/// <summary>How often the window may ask the server for the list, in milliseconds.</summary>
constexpr DWORD MINIMUM_REQUEST_INTERVAL = 1000;

/// <summary>How many seconds an hour has, from which a countdown decides how it is written.</summary>
constexpr unsigned int SECONDS_PER_HOUR = 3600;
constexpr unsigned int SECONDS_PER_MINUTE = 60;

/// <summary>What a cell holds when the row has nothing to put in it.</summary>
constexpr const wchar_t* NOTHING = L"—";

/// <summary>Sets the colour the text which follows is written in.</summary>
void UseTextColor(const TextColor& color)
{
    g_pRenderText->SetTextColor(color.red, color.green, color.blue, color.alpha);
}

/// <summary>Draws the four edges of a rectangle, which is the only border this window needs.</summary>
void RenderBorder(int x, int y, int width, int height, unsigned int color, int thickness = 1)
{
    const float fx = static_cast<float>(x);
    const float fy = static_cast<float>(y);
    const float fw = static_cast<float>(width);
    const float fh = static_cast<float>(height);
    const float ft = static_cast<float>(thickness);

    RenderColorQuadARGB(fx, fy, fw, ft, color);
    RenderColorQuadARGB(fx, fy + fh - ft, fw, ft, color);
    RenderColorQuadARGB(fx, fy + ft, ft, fh - 2.f * ft, color);
    RenderColorQuadARGB(fx + fw - ft, fy + ft, ft, fh - 2.f * ft, color);
}

/// <summary>Gets the message which says what came of a join which did not succeed.</summary>
const wchar_t* GetJoinFailureText(const Net::Event::JoinResult result)
{
    switch (result)
    {
    case Net::Event::JoinResult::NotOpen:
        return I18N::Game::EventJoinNotOpen;
    case Net::Event::JoinResult::CharacterLevelTooHigh:
        return I18N::Game::EventJoinLevelTooHigh;
    case Net::Event::JoinResult::CharacterLevelTooLow:
        return I18N::Game::EventJoinLevelTooLow;
    case Net::Event::JoinResult::Full:
        return I18N::Game::EventJoinFull;
    case Net::Event::JoinResult::NotEnoughMoney:
        return I18N::Game::EventJoinNotEnoughMoney;
    case Net::Event::JoinResult::PlayerKillerCantEnter:
        return I18N::Game::EventJoinPlayerKiller;
    case Net::Event::JoinResult::UnknownEvent:
        return I18N::Game::EventJoinUnknown;
    case Net::Event::JoinResult::NotJoinable:
        return I18N::Game::EventJoinNotJoinable;
    default:
        return I18N::Game::EventJoinFailed;
    }
}
} // namespace

SEASON3B::CNewUIEventListWindow::CNewUIEventListWindow() = default;

SEASON3B::CNewUIEventListWindow::~CNewUIEventListWindow()
{
    Release();
}

bool SEASON3B::CNewUIEventListWindow::Create(CNewUIManager* pNewUIMng, int x, int y)
{
    if (pNewUIMng == nullptr)
    {
        return false;
    }

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(INTERFACE_EVENT_LIST, this);

    InitButton(&m_buttons[BTN_PREV], &I18N::Game::EventPreviousPage);
    InitButton(&m_buttons[BTN_NEXT], &I18N::Game::EventNextPage);
    InitButton(&m_buttons[BTN_REFRESH], &I18N::Game::EventRefresh);

    for (CNewUIButton& button : m_joinButtons)
    {
        InitButton(&button, &I18N::Game::EventJoin);
    }

    SetPos(x, y);

    Show(false);

    return true;
}

void SEASON3B::CNewUIEventListWindow::Release()
{
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }
}

void SEASON3B::CNewUIEventListWindow::InitButton(CNewUIButton* pButton, const wchar_t* const* captionSlot)
{
    // The button gets no image: this window draws the plate under every caption itself, because the
    // images of the original dialogs are cut for one size and nothing else.
    pButton->ChangeText(captionSlot);
    pButton->ChangeTextBackColor(RGBA(255, 255, 255, 0));
    pButton->ChangeTextColor(RGBA(240, 228, 200, 255));
}

void SEASON3B::CNewUIEventListWindow::SetPos(int x, int y)
{
    m_Pos.x = x;
    m_Pos.y = y;
    BuildLayout();
    ApplyLayoutToButtons();
}

void SEASON3B::CNewUIEventListWindow::BuildLayout()
{
    Layout& layout = m_layout;
    layout.width = WINDOW_WIDTH;
    layout.height = WINDOW_HEIGHT;
    layout.rowHeight = ROW_HEIGHT;

    layout.titleTop = 8;
    layout.headingTop = TITLE_HEIGHT + GAP;
    layout.firstRowTop = layout.headingTop + HEADING_LINE_TOP + GAP;

    // The join button is the last column, and the three columns of text share what is left of the
    // width: the name takes about half of it, the state and the countdown a quarter each.
    const int joinColumnWidth = JOIN_BUTTON_WIDTH + GAP;
    const int textWidth = layout.width - 2 * EDGE - joinColumnWidth;

    layout.nameColumnX = EDGE;
    layout.nameColumnWidth = textWidth / 2;
    layout.stateColumnX = layout.nameColumnX + layout.nameColumnWidth;
    layout.stateColumnWidth = textWidth / 4;
    layout.remainingColumnX = layout.stateColumnX + layout.stateColumnWidth;
    layout.remainingColumnWidth = textWidth - layout.nameColumnWidth - layout.stateColumnWidth;

    const int joinX = layout.remainingColumnX + layout.remainingColumnWidth + GAP;
    for (int row = 0; row < ROWS_PER_PAGE; ++row)
    {
        const int top = layout.firstRowTop + row * layout.rowHeight;
        layout.joinButton[row].left = joinX;
        layout.joinButton[row].right = joinX + JOIN_BUTTON_WIDTH;
        layout.joinButton[row].top = top + (layout.rowHeight - JOIN_BUTTON_HEIGHT) / 2;
        layout.joinButton[row].bottom = layout.joinButton[row].top + JOIN_BUTTON_HEIGHT;
    }

    layout.descriptionTop = layout.firstRowTop + ROWS_PER_PAGE * layout.rowHeight + GAP;
    layout.pageRowTop = layout.height - EDGE - BUTTON_HEIGHT;

    layout.prevButton.left = EDGE;
    layout.prevButton.right = layout.prevButton.left + PAGE_BUTTON_WIDTH;
    layout.prevButton.top = layout.pageRowTop;
    layout.prevButton.bottom = layout.pageRowTop + BUTTON_HEIGHT;

    layout.nextButton.left = layout.prevButton.right + 2 * GAP + 40;
    layout.nextButton.right = layout.nextButton.left + PAGE_BUTTON_WIDTH;
    layout.nextButton.top = layout.prevButton.top;
    layout.nextButton.bottom = layout.prevButton.bottom;

    layout.refreshButton.right = layout.width - EDGE;
    layout.refreshButton.left = layout.refreshButton.right - REFRESH_BUTTON_WIDTH;
    layout.refreshButton.top = layout.prevButton.top;
    layout.refreshButton.bottom = layout.prevButton.bottom;
}

void SEASON3B::CNewUIEventListWindow::ApplyLayoutToButtons()
{
    const auto place = [this](CNewUIButton& button, const RECT& rect)
    {
        button.ChangeButtonInfo(m_Pos.x + rect.left, m_Pos.y + rect.top, rect.right - rect.left,
                                rect.bottom - rect.top);
    };

    place(m_buttons[BTN_PREV], m_layout.prevButton);
    place(m_buttons[BTN_NEXT], m_layout.nextButton);
    place(m_buttons[BTN_REFRESH], m_layout.refreshButton);

    for (int row = 0; row < ROWS_PER_PAGE; ++row)
    {
        place(m_joinButtons[row], m_layout.joinButton[row]);
    }
}

float SEASON3B::CNewUIEventListWindow::GetLayerDepth()
{
    // The same depth the bank stands at: over the windows the dock knows, and under the chat, the
    // system log, the tooltip of an item, the menu, the options and the hud frame.
    return 5.9f;
}

void SEASON3B::CNewUIEventListWindow::OpeningProcess()
{
    m_page = 0;
    m_selectedRow = -1;
    m_lastRequestTick = 0;
    RequestEventList();
}

void SEASON3B::CNewUIEventListWindow::ClosingProcess()
{
    m_page = 0;
    m_selectedRow = -1;
}

void SEASON3B::CNewUIEventListWindow::RequestEventList()
{
    const DWORD now = GetTickCount();
    if (m_lastRequestTick != 0 && now - m_lastRequestTick < MINIMUM_REQUEST_INTERVAL)
    {
        return;
    }

    m_lastRequestTick = now;
    SocketClient->ToGameServer()->SendEventList();
}

int SEASON3B::CNewUIEventListWindow::GetEventCount() const
{
    return static_cast<int>(Net::Event::Store::Instance().GetEvents().size());
}

const Net::Event::Entry* SEASON3B::CNewUIEventListWindow::GetEventInRow(const int row) const
{
    const std::vector<Net::Event::Entry>& events = Net::Event::Store::Instance().GetEvents();
    const int index = m_page * ROWS_PER_PAGE + row;
    if (row < 0 || row >= ROWS_PER_PAGE || index < 0 || index >= static_cast<int>(events.size()))
    {
        return nullptr;
    }

    return &events[index];
}

unsigned int SEASON3B::CNewUIEventListWindow::GetCountdown(const Net::Event::Entry& entry) const
{
    const unsigned int elapsed = Net::Event::Store::Instance().GetSecondsSinceReceived();
    const unsigned int total =
        entry.State == Net::Event::RunState::Running || entry.State == Net::Event::RunState::EntranceOpen
            ? entry.SecondsRemaining
            : entry.SecondsUntilStart;

    return total > elapsed ? total - elapsed : 0;
}

bool SEASON3B::CNewUIEventListWindow::CanJoinRow(const int row) const
{
    const Net::Event::Entry* entry = GetEventInRow(row);
    return entry != nullptr && entry->Join == Net::Event::JoinMode::Enter;
}

const wchar_t* SEASON3B::CNewUIEventListWindow::GetStateText(const Net::Event::Entry& entry)
{
    switch (entry.State)
    {
    case Net::Event::RunState::EntranceOpen:
        return I18N::Game::EventStateEntranceOpen;
    case Net::Event::RunState::Running:
        return I18N::Game::EventStateRunning;
    case Net::Event::RunState::Waiting:
        return I18N::Game::EventStateWaiting;
    default:
        return I18N::Game::EventStateUnscheduled;
    }
}

void SEASON3B::CNewUIEventListWindow::FormatCountdown(const unsigned int seconds, wchar_t* text,
                                                      const size_t textLength)
{
    if (textLength == 0)
    {
        return;
    }

    const unsigned int hours = seconds / SECONDS_PER_HOUR;
    const unsigned int minutes = (seconds % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE;
    const unsigned int rest = seconds % SECONDS_PER_MINUTE;

    if (hours > 0)
    {
        mu_swprintf(text, L"%u:%02u:%02u", hours, minutes, rest);
    }
    else
    {
        mu_swprintf(text, L"%02u:%02u", minutes, rest);
    }
}

void SEASON3B::CNewUIEventListWindow::RefreshFromStore()
{
    const unsigned int generation = Net::Event::Store::Instance().GetGeneration();
    if (generation == m_shownGeneration)
    {
        return;
    }

    m_shownGeneration = generation;

    // Another list may be shorter than the one which was on screen, so a page which no longer
    // exists is left rather than shown empty.
    const int pageCount = std::max(1, (GetEventCount() + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE);
    m_page = std::min(m_page, pageCount - 1);
    m_selectedRow = -1;
}

void SEASON3B::CNewUIEventListWindow::ProcessJoinResult()
{
    std::array<BYTE, Net::Event::EventIdLength> eventId{};
    Net::Event::JoinResult result = Net::Event::JoinResult::Failed;
    if (!Net::Event::Store::Instance().TakeNewJoinResult(eventId, result))
    {
        return;
    }

    // The name of the event is read out of the list which is on screen, so that the message says
    // what was joined rather than repeating an identifier nobody can read.
    std::wstring name;
    for (const Net::Event::Entry& entry : Net::Event::Store::Instance().GetEvents())
    {
        if (entry.EventId == eventId)
        {
            name = entry.Name;
            break;
        }
    }

    wchar_t message[256] = {0};
    if (result == Net::Event::JoinResult::Success)
    {
        mu_swprintf(message, I18N::Game::EventJoined, name.c_str());
        g_pChatListBox->AddText(L"", message, SEASON3B::TYPE_SYSTEM_MESSAGE);

        // The warp follows on its own; nothing is left to read in a list of what is open.
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_EVENT_LIST);
        return;
    }

    mu_swprintf(message, GetJoinFailureText(result), name.c_str());
    g_pChatListBox->AddText(L"", message, SEASON3B::TYPE_ERROR_MESSAGE);

    // A refusal usually means what was on screen had gone stale, so the list is asked for again.
    RequestEventList();
}

bool SEASON3B::CNewUIEventListWindow::Update()
{
    if (!IsVisible())
    {
        return true;
    }

    RefreshFromStore();
    ProcessJoinResult();

    // A countdown which has run out is the moment at which the states change, so that is when the
    // list is worth asking for again.
    for (int row = 0; row < ROWS_PER_PAGE; ++row)
    {
        const Net::Event::Entry* entry = GetEventInRow(row);
        if (entry != nullptr && entry->SecondsUntilStart != Net::Event::NoNextStart && GetCountdown(*entry) == 0)
        {
            RequestEventList();
            break;
        }
    }

    return true;
}

bool SEASON3B::CNewUIEventListWindow::UpdateKeyEvent()
{
    if (IsVisible() && SEASON3B::IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(SEASON3B::INTERFACE_EVENT_LIST);
        PlayBuffer(SOUND_CLICK01);
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIEventListWindow::UpdateMouseEvent()
{
    if (!IsVisible())
    {
        return true;
    }

    if (ProcessButtons())
    {
        return false;
    }

    if (ProcessRowSelection())
    {
        return false;
    }

    // The window swallows what happens over it, so a click beside a row does not walk the character
    // to the other side of the map.
    if (SEASON3B::CheckMouseIn(m_Pos.x, m_Pos.y, m_layout.width, m_layout.height))
    {
        return false;
    }

    return true;
}

bool SEASON3B::CNewUIEventListWindow::ProcessButtons()
{
    const int pageCount = std::max(1, (GetEventCount() + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE);

    if (m_buttons[BTN_PREV].UpdateMouseEvent())
    {
        if (m_page > 0)
        {
            --m_page;
            m_selectedRow = -1;
        }

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_buttons[BTN_NEXT].UpdateMouseEvent())
    {
        if (m_page + 1 < pageCount)
        {
            ++m_page;
            m_selectedRow = -1;
        }

        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    if (m_buttons[BTN_REFRESH].UpdateMouseEvent())
    {
        RequestEventList();
        PlayBuffer(SOUND_CLICK01);
        return true;
    }

    for (int row = 0; row < ROWS_PER_PAGE; ++row)
    {
        if (!CanJoinRow(row))
        {
            continue;
        }

        if (m_joinButtons[row].UpdateMouseEvent())
        {
            const Net::Event::Entry* entry = GetEventInRow(row);
            if (entry != nullptr)
            {
                SocketClient->ToGameServer()->SendEventJoin(entry->EventId.data());
                PlayBuffer(SOUND_CLICK01);
            }

            return true;
        }
    }

    return false;
}

bool SEASON3B::CNewUIEventListWindow::ProcessRowSelection()
{
    for (int row = 0; row < ROWS_PER_PAGE; ++row)
    {
        if (GetEventInRow(row) == nullptr)
        {
            continue;
        }

        const int top = m_Pos.y + m_layout.firstRowTop + row * m_layout.rowHeight;
        if (SEASON3B::CheckMouseIn(m_Pos.x + EDGE, top, m_layout.width - 2 * EDGE, m_layout.rowHeight))
        {
            if (SEASON3B::IsRelease(VK_LBUTTON))
            {
                m_selectedRow = row;
                PlayBuffer(SOUND_CLICK01);
                return true;
            }
        }
    }

    return false;
}

bool SEASON3B::CNewUIEventListWindow::Render()
{
    if (!IsVisible())
    {
        return true;
    }

    EnableAlphaTest();

    RenderFrame();
    RenderHeadings();
    RenderRows();
    RenderDescription();
    RenderPageRow();

    DisableAlphaBlend();

    return true;
}

void SEASON3B::CNewUIEventListWindow::RenderFrame()
{
    const int x = m_Pos.x;
    const int y = m_Pos.y;
    const int width = m_layout.width;
    const int height = m_layout.height;

    RenderColorQuadARGB(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width),
                        static_cast<float>(height), PANEL_COLOR);
    RenderBorder(x, y, width, height, PANEL_EDGE_COLOR, 1);
    RenderBorder(x + 1, y + 1, width - 2, height - 2, PANEL_INNER_COLOR, 1);
    EndRenderColor();

    g_pRenderText->SetFont(g_hFontBold);
    g_pRenderText->SetBgColor(0);
    UseTextColor(TITLE_TEXT);
    g_pRenderText->RenderText(x, y + m_layout.titleTop, I18N::Game::Events, width, 0, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIEventListWindow::RenderHeadings()
{
    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);
    UseTextColor(HEADING_TEXT);

    const int top = m_Pos.y + m_layout.headingTop;
    g_pRenderText->RenderText(m_Pos.x + m_layout.nameColumnX, top, I18N::Game::EventColumnName,
                              m_layout.nameColumnWidth, 0);
    g_pRenderText->RenderText(m_Pos.x + m_layout.stateColumnX, top, I18N::Game::EventColumnState,
                              m_layout.stateColumnWidth, 0);
    g_pRenderText->RenderText(m_Pos.x + m_layout.remainingColumnX, top, I18N::Game::EventColumnRemaining,
                              m_layout.remainingColumnWidth, 0, RT3_SORT_RIGHT);

    RenderColorQuadARGB(static_cast<float>(m_Pos.x + EDGE), static_cast<float>(top + HEADING_LINE_TOP),
                        static_cast<float>(m_layout.width - 2 * EDGE), 1.f, HEADING_LINE_COLOR);
    EndRenderColor();
}

void SEASON3B::CNewUIEventListWindow::RenderRows()
{
    if (GetEventCount() == 0)
    {
        g_pRenderText->SetFont(g_hFont);
        UseTextColor(EMPTY_LIST_TEXT);
        g_pRenderText->RenderText(m_Pos.x, m_Pos.y + m_layout.firstRowTop + m_layout.rowHeight,
                                  I18N::Game::EventListIsEmpty, m_layout.width, 0, RT3_SORT_CENTER);
        return;
    }

    for (int row = 0; row < ROWS_PER_PAGE; ++row)
    {
        const Net::Event::Entry* entry = GetEventInRow(row);
        if (entry == nullptr)
        {
            continue;
        }

        const int top = m_Pos.y + m_layout.firstRowTop + row * m_layout.rowHeight;
        const int left = m_Pos.x + EDGE;
        const int rowWidth = m_layout.width - 2 * EDGE;

        if (row == m_selectedRow)
        {
            RenderColorQuadARGB(static_cast<float>(left), static_cast<float>(top), static_cast<float>(rowWidth),
                                static_cast<float>(m_layout.rowHeight), SELECTED_ROW_COLOR);
            EndRenderColor();
        }
        else if (SEASON3B::CheckMouseIn(left, top, rowWidth, m_layout.rowHeight))
        {
            RenderColorQuadARGB(static_cast<float>(left), static_cast<float>(top), static_cast<float>(rowWidth),
                                static_cast<float>(m_layout.rowHeight), HOVERED_ROW_COLOR);
            EndRenderColor();
        }

        g_pRenderText->SetFont(g_hFont);
        g_pRenderText->SetBgColor(0);

        const int textTop = top + ROW_TEXT_TOP;
        UseTextColor(ROW_TEXT);
        g_pRenderText->RenderText(m_Pos.x + m_layout.nameColumnX, textTop, entry->Name.c_str(),
                                  m_layout.nameColumnWidth, 0);

        switch (entry->State)
        {
        case Net::Event::RunState::EntranceOpen:
            UseTextColor(OPEN_TEXT);
            break;
        case Net::Event::RunState::Running:
            UseTextColor(RUNNING_TEXT);
            break;
        case Net::Event::RunState::Unscheduled:
            UseTextColor(UNSCHEDULED_TEXT);
            break;
        default:
            UseTextColor(ROW_TEXT);
            break;
        }

        g_pRenderText->RenderText(m_Pos.x + m_layout.stateColumnX, textTop, GetStateText(*entry),
                                  m_layout.stateColumnWidth, 0);

        wchar_t countdown[32] = {0};
        if (entry->State == Net::Event::RunState::Unscheduled)
        {
            mu_swprintf(countdown, L"%s", NOTHING);
        }
        else
        {
            FormatCountdown(GetCountdown(*entry), countdown, std::size(countdown));
        }

        g_pRenderText->RenderText(m_Pos.x + m_layout.remainingColumnX, textTop, countdown,
                                  m_layout.remainingColumnWidth, 0, RT3_SORT_RIGHT);

        if (CanJoinRow(row))
        {
            RenderButton(m_joinButtons[row], true);
        }
    }
}

void SEASON3B::CNewUIEventListWindow::RenderDescription()
{
    const Net::Event::Entry* entry = GetEventInRow(m_selectedRow);
    if (entry == nullptr)
    {
        return;
    }

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);
    UseTextColor(LABEL_TEXT);

    const int top = m_Pos.y + m_layout.descriptionTop;
    g_pRenderText->RenderText(m_Pos.x + EDGE, top, entry->Description.c_str(), m_layout.width - 2 * EDGE, 0);

    if (entry->PlayerCount > 0)
    {
        wchar_t inside[64] = {0};
        mu_swprintf(inside, I18N::Game::EventPlayersInside, static_cast<int>(entry->PlayerCount));
        g_pRenderText->RenderText(m_Pos.x + EDGE, top + ROW_HEIGHT - 4, inside, m_layout.width - 2 * EDGE, 0);
    }
}

void SEASON3B::CNewUIEventListWindow::RenderPageRow()
{
    const int pageCount = std::max(1, (GetEventCount() + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE);

    RenderButton(m_buttons[BTN_PREV], m_page > 0);
    RenderButton(m_buttons[BTN_NEXT], m_page + 1 < pageCount);
    RenderButton(m_buttons[BTN_REFRESH], false);

    g_pRenderText->SetFont(g_hFont);
    g_pRenderText->SetBgColor(0);
    UseTextColor(LABEL_TEXT);

    wchar_t page[32] = {0};
    mu_swprintf(page, I18N::Game::EventPageOf, m_page + 1, pageCount);
    g_pRenderText->RenderText(m_Pos.x + m_layout.prevButton.right, m_Pos.y + m_layout.pageRowTop + 2, page,
                              m_layout.nextButton.left - m_layout.prevButton.right, 0, RT3_SORT_CENTER);
}

void SEASON3B::CNewUIEventListWindow::RenderButton(CNewUIButton& button, const bool highlighted)
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
