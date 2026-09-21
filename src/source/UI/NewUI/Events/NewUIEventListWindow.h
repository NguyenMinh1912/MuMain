//*****************************************************************************
// File: NewUIEventListWindow.h
//*****************************************************************************

#pragma once

#include "Network/Server/EventProtocol.h"
#include "UI/NewUI/NewUIManager.h"
#include "UI/NewUI/Widgets/NewUIButton.h"

#include <array>
#include <vector>

namespace SEASON3B
{
/// <summary>
/// The events which this game server runs: what is open now, what starts next, and in how long.
/// </summary>
/// <remarks>
/// Not part of the original client. The player reaches it from the menu at the bottom right, and it
/// is the only place which shows every event at once - an entrance npc only ever knows about its own.
///
/// The window knows nothing about which events exist. It draws the names, the states and the
/// countdowns the server sent, and offers the join button for exactly those rows the server marked
/// as joinable, so an event added to the server later appears here without a change to the client.
///
/// The countdowns are counted down from the tick at which the list arrived rather than from a point
/// in time the server named, because the clock of this machine is not the clock of the server. When
/// one of them runs out the window asks for the list again, which is also what refreshes the states.
/// </remarks>
class CNewUIEventListWindow : public CNewUIObj
{
public:
    /// <summary>How many events stand on one page.</summary>
    static constexpr int ROWS_PER_PAGE = 8;

    /// <summary>How wide the window is, and how tall.</summary>
    /// <remarks>
    /// Both are defined here rather than beside the rest of the measurements, because whoever places
    /// the window has to know them to centre it.
    /// </remarks>
    static constexpr int WINDOW_WIDTH = 420;
    static constexpr int WINDOW_HEIGHT = 300;

    /// <summary>Gets how wide the window is, so that whoever places it can centre it.</summary>
    static constexpr int GetWindowWidth()
    {
        return WINDOW_WIDTH;
    }

    /// <summary>Gets how tall the window is.</summary>
    static constexpr int GetWindowHeight()
    {
        return WINDOW_HEIGHT;
    }

    CNewUIEventListWindow();
    ~CNewUIEventListWindow() override;

    bool Create(CNewUIManager* pNewUIMng, int x, int y);
    void Release();

    void SetPos(int x, int y);

    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool Update() override;
    bool Render() override;

    float GetLayerDepth() override; //. 5.9f

    /// <summary>Asks the server for the events.</summary>
    void OpeningProcess();

    /// <summary>Forgets the page and the selection.</summary>
    void ClosingProcess();

private:
    /// <summary>The buttons which stand in the row at the bottom of the window.</summary>
    enum EVENT_BUTTON
    {
        BTN_PREV = 0,
        BTN_NEXT,
        BTN_REFRESH,
        MAX_EVENT_BUTTON,
    };

    /// <summary>Where everything stands, worked out once from the position of the window.</summary>
    struct Layout
    {
        int width = 0;
        int height = 0;
        int titleTop = 0;
        int headingTop = 0;
        int firstRowTop = 0;
        int rowHeight = 0;
        int nameColumnX = 0;
        int nameColumnWidth = 0;
        int stateColumnX = 0;
        int stateColumnWidth = 0;
        int remainingColumnX = 0;
        int remainingColumnWidth = 0;
        int descriptionTop = 0;
        int pageRowTop = 0;
        RECT joinButton[ROWS_PER_PAGE]{};
        RECT prevButton{};
        RECT nextButton{};
        RECT refreshButton{};
    };

    void InitButton(CNewUIButton* pButton, const wchar_t* const* captionSlot);
    void BuildLayout();
    void ApplyLayoutToButtons();

    /// <summary>Takes the list the parser filled in, if another one arrived since the last frame.</summary>
    void RefreshFromStore();

    /// <summary>Writes what came of a join into the system log, if one was answered.</summary>
    void ProcessJoinResult();

    /// <summary>Asks the server for the list, at most as often as the server answers.</summary>
    void RequestEventList();

    bool ProcessButtons();
    bool ProcessRowSelection();

    void RenderFrame();
    void RenderHeadings();
    void RenderRows();
    void RenderDescription();
    void RenderPageRow();
    void RenderButton(CNewUIButton& button, bool highlighted);

    /// <summary>Gets how many events there are, which is what the paging follows.</summary>
    int GetEventCount() const;

    /// <summary>Gets the event which stands in the given row of the current page, or <c>nullptr</c>.</summary>
    const Net::Event::Entry* GetEventInRow(int row) const;

    /// <summary>Gets how many seconds the given event still counts down, already reduced.</summary>
    unsigned int GetCountdown(const Net::Event::Entry& entry) const;

    /// <summary>Gets whether the join button of the given row is offered at all.</summary>
    bool CanJoinRow(int row) const;

    /// <summary>Gets what the state column of the given event says.</summary>
    static const wchar_t* GetStateText(const Net::Event::Entry& entry);

    /// <summary>Writes a countdown as mm:ss, or as h:mm:ss once it no longer fits.</summary>
    static void FormatCountdown(unsigned int seconds, wchar_t* text, size_t textLength);

    CNewUIManager* m_pNewUIMng = nullptr;
    POINT m_Pos{};
    Layout m_layout{};
    std::array<CNewUIButton, MAX_EVENT_BUTTON> m_buttons{};
    std::array<CNewUIButton, ROWS_PER_PAGE> m_joinButtons{};

    /// <summary>The page which is shown, starting at 0.</summary>
    int m_page = 0;

    /// <summary>The row of the page whose description is written under the list, or -1.</summary>
    int m_selectedRow = -1;

    /// <summary>Which list the window last drew, so that it notices when another one arrives.</summary>
    unsigned int m_shownGeneration = 0;

    /// <summary>The tick at which the list was last asked for, so that it is not asked in a loop.</summary>
    DWORD m_lastRequestTick = 0;
};
} // namespace SEASON3B
