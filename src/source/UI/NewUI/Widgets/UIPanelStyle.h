// <copyright file="UIPanelStyle.h" company="MUnique">
// Licensed under the MIT License. See LICENSE file in the project root for full license information.
// </copyright>

#pragma once

#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Legacy/UIControls.h"

/// <summary>
/// The look the windows share which draw themselves out of plain quads instead of out of the
/// textures of the original dialogs.
/// </summary>
/// <remarks>
/// The bank was the first of them, and these values are the ones it settled on. A window which
/// borrows the textures of the vault gets a frame cut for 190 pixels, which leaves half of a wider
/// window bare; a window which draws itself has to look like the others, which is what this header
/// is for. Anything only one window needs stays in that window.
/// </remarks>
namespace UI::PanelStyle
{
/// <summary>The body of a window, dark enough for the world behind it to stay behind it.</summary>
constexpr unsigned int PANEL_COLOR = 0xEE0E0F12u;

/// <summary>The bronze edge of a window and the darker line just inside it.</summary>
constexpr unsigned int PANEL_EDGE_COLOR = 0xFF6B5A3Cu;
constexpr unsigned int PANEL_INNER_COLOR = 0xFF241F16u;

/// <summary>The area boxes stand in, and the lines between them.</summary>
constexpr unsigned int GRID_BACK_COLOR = 0x99000000u;
constexpr unsigned int GRID_FRAME_COLOR = 0xFF4A4235u;
constexpr unsigned int TILE_COLOR = 0xFF161A20u;
constexpr unsigned int TILE_LINE_COLOR = 0xFF2F3640u;

/// <summary>What is picked, and what the cursor is over.</summary>
constexpr unsigned int PICKED_FILL_COLOR = 0x40FFC83Cu;
constexpr unsigned int PICKED_EDGE_COLOR = 0xFFFFC83Cu;
constexpr unsigned int HOVERED_FILL_COLOR = 0x22FFFFFFu;

/// <summary>The plate of a button, in its three states, and its edge.</summary>
constexpr unsigned int BUTTON_UP_COLOR = 0xFF433D31u;
constexpr unsigned int BUTTON_OVER_COLOR = 0xFF564E3Eu;
constexpr unsigned int BUTTON_DOWN_COLOR = 0xFF2B261Du;
constexpr unsigned int BUTTON_EDGE_COLOR = 0xFF8A7444u;
constexpr unsigned int BUTTON_EDGE_ON_COLOR = 0xFFFFC83Cu;

/// <summary>The line which separates a heading from the rows under it.</summary>
constexpr unsigned int HEADING_LINE_COLOR = 0x804A5566u;

/// <summary>
/// A box which does not open a window of its own but a page of the same one, so that it reads as
/// part of the frame rather than as one more feature.
/// </summary>
constexpr unsigned int TILE_ACCENT_COLOR = 0xFF2A2519u;

/// <summary>A box the player cannot use yet, and the line around it.</summary>
constexpr unsigned int TILE_DISABLED_COLOR = 0xFF12151Au;
constexpr unsigned int TILE_DISABLED_LINE_COLOR = 0xFF252A32u;

/// <summary>A colour text is written in.</summary>
struct TextColor
{
    int red;
    int green;
    int blue;
    int alpha;
};

/// <summary>The name of a window.</summary>
constexpr TextColor TITLE_TEXT{240, 220, 164, 255};

/// <summary>A heading over a list, and the names of its columns.</summary>
constexpr TextColor HEADING_TEXT{143, 184, 232, 255};

/// <summary>What is picked, and what a price is written in.</summary>
constexpr TextColor HIGHLIGHT_TEXT{255, 210, 76, 255};

/// <summary>The row which is picked, which stands on a plate of its own.</summary>
constexpr TextColor PICKED_ROW_TEXT{255, 233, 168, 255};

/// <summary>A row which is not picked.</summary>
constexpr TextColor ROW_TEXT{220, 214, 200, 255};

/// <summary>A label, and the number of the page.</summary>
constexpr TextColor LABEL_TEXT{200, 194, 180, 255};

/// <summary>Text which is read after the one beside it.</summary>
constexpr TextColor MUTED_TEXT{180, 176, 166, 255};

/// <summary>The line which says that there is nothing to show.</summary>
constexpr TextColor EMPTY_LIST_TEXT{180, 180, 180, 255};

/// <summary>What a box says which the player cannot use yet.</summary>
constexpr TextColor DISABLED_TEXT{111, 119, 135, 255};

/// <summary>Sets the colour the text which follows is written in.</summary>
inline void UseTextColor(const TextColor& color)
{
    g_pRenderText->SetTextColor(color.red, color.green, color.blue, color.alpha);
}

/// <summary>Draws the four edges of a rectangle, which is the only border these windows need.</summary>
inline void RenderBorder(int x, int y, int width, int height, unsigned int color, int thickness = 1)
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
} // namespace UI::PanelStyle
