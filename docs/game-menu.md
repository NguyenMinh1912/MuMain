# The game menu

The menu button of the hud - the last button of the row, beside the friend list - opens every window
of the game from one place. `U` opens the same thing.

It opens in two steps. The first is a **quick grid** of six boxes which stands over the button: five
features and, last, **All**. The second is the **full panel** behind All, which holds every window
the game has, sorted into four groups.

Both stand in the bottom right corner, over the row of buttons and never over the health and mana
orbs. Opening a window from a box closes the menu, because the menu has then said what it had to
say.

## The quick grid

| Box | What it opens |
|---|---|
| **Bank** | The bank of the account |
| **Market** | The bank, on the tab of the market |
| **Move** | The list of maps to move to |
| **Quests** | The quests of the character |
| **Helper** | MU Helper |
| **All** | The full panel - it opens no window of its own, so it is drawn in bronze rather than in the colour of the others |

## The full panel

Four tabs. A box which is disabled is dimmed and stays where it is: the feature behind it is not
available to this character, on this map, or on this server.

| Tab | Boxes |
|---|---|
| **Featured** | Bank · Market · Move · Quests · Helper · Friends · Guild · Map |
| **Character** | Info · Items · Master · Pet · Quests · Helper |
| **Social** | Friends · Guild · Party · Gens |
| **System** | Options · Help · Commands · Map · Move · Exit |

**Back** returns to the quick grid without closing the menu. **X** closes it.

## What a box does

A box does exactly what the shortcut of that window does, with the same conditions:

- **Friends** says that level 6 is needed below it, and cannot be reached in Chaos Castle.
- **Master** is disabled for a class which has no master level.
- **Commands** is disabled on a server which does not publish its chat commands.
- **Map** is disabled while the minimap of the map has not been read.
- **Items** does nothing while an item is being sold to an npc, which is the rule the `I` key follows.
- **Exit** opens the system menu, so leaving the game did not change.

Two ways into one window therefore cannot disagree: the box is the shortcut made visible.

## Keys

| Key | What it does |
|---|---|
| `U` | Opens and closes the menu |
| `Esc` | Steps back one layer: the panel gives way to the quick grid, the quick grid closes |

A click beside the quick grid closes it. A click beside the panel does not, because the panel is a
window and not a popup. A click on the hud closes neither, so the menu button keeps working.

Opening a window which takes over the screen - the inventory, a shop, the vault - closes the menu
with the rest.

## The pictures

The boxes draw their pictures out of one atlas, `Data\Interface\newui_menu_icons.OZT`, which
`tools/make_menu_icons.py` writes. A client whose data is a release behind has no atlas: the menu
then shows the names alone rather than refusing to start.
