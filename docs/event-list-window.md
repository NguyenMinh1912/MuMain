# The event list window

The window lists the events this game server runs: what is open right now, what starts next, and in
how long. It is the only place which shows all of them at once - an entrance npc only ever knows
about its own event, and only if you walk to it.

The player opens it from the menu at the bottom right, entry **Events**. Escape closes it again.
Nothing else is opened or closed with it: it lists what is going on and gets out of the way.

## What a row says

| Column | What it shows |
|---|---|
| **Event** | The name of the event, in the language of the player |
| **State** | *Not scheduled*, *Starts in*, *Entrance open* or *Running* |
| **Remaining** | The countdown, `mm:ss` below an hour and `h:mm:ss` above it |
| | **Join**, for a row which can be entered right now |

The countdown counts down to the next start, and while the event runs, to its end. An event whose
timetable is empty never starts by itself, so its countdown is a dash rather than a time nobody can
rely on.

The colour of the state says the same thing again for somebody scanning the list: green while the
entrance stands open, amber while the event runs behind a closed entrance, grey for an event which
is not scheduled at all.

The rows are ordered by what happens next - open first, then running, then by countdown, and the
unscheduled ones last. Eight stand on a page; **Prev** and **Next** turn it and **Refresh** asks for
the list again.

Clicking a row writes what the event does underneath the list, and how many players are inside it
when it has an inside.

## Joining

**Join** appears only on the rows the server marked as joinable, which today means a mini game whose
entrance stands open - Blood Castle, Devil Square, Chaos Castle, Kanturu. Pressing it puts the
character inside: no walking to the gatekeeper, no talking to an npc, and no ticket picked by hand.

The ticket, the level range, the entrance fee, the player kill state and whether the entrance is
still open are all checked by the server, by exactly the same entrance which the npc uses. The level
of the event is the one which fits the character, so a player never picks the wrong Blood Castle.

On success the window closes itself and the warp follows. Otherwise the reason is written into the
system log - the entrance closed, the level too low or too high, the event full, the fee unpaid, a
player killer - and the list is asked for again, because a refusal usually means what was on screen
had gone stale.

## Why the countdowns are right

The server sends durations, not points in time: how many seconds until the next start, and how many
are left of the current run. The window counts down from the moment the packet was read. A machine
whose clock is an hour off therefore still shows the right countdown, and a machine whose clock
jumps does not jump with it.

When a countdown on screen reaches zero the window asks for the list again, which is also the moment
the states change. Nothing is pushed: a player watching the list is within a second of the truth and
costs the server one small packet a minute.

## What the client does not know

The client knows nothing about which events exist. The server sends a name, a state, a countdown and
a handle per event, and the window draws what it was given. An event added to the server later
appears here with no change to the client at all, and so does its name in the language of the player,
because the server reads it out of its own resources.

The list is the state of the game server the player is on, and of no other. That is the truth, and it
is what the join acts on.
