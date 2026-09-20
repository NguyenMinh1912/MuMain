# The bank window

The bank keeps what an account owns across all of its characters - zen, the cash shop currencies,
jewels and items - and it is where a player sends value to somebody else or offers it on the
market of the server.

The window only appears on a server which runs the bank. Talking to the npc which has the bank
dialog opens it; a server may also enable a `/bank` command which opens the same window from
anywhere.

## The storage page

The boxes work exactly like the vault: drag an item from the inventory into the bank to deposit
it, drag it back to take it out.

Below the boxes one currency is shown at a time with what the bank holds of it. **Prev** and
**Next** step through them - zen, WCoinC, WCoinP, goblin points, and each jewel and refine stone.

| Button | What it does |
|---|---|
| **Deposit all** | Puts everything the character has of the shown currency into the bank |
| **Withdraw all** | Takes everything the bank holds of it back out |
| **Offer** | Puts the picked item on the market, and asks for the price |
| **Send item** | Sends the picked item to another account, and asks who gets it |
| **Send** | Sends the shown currency to another account, and asks who gets it and how much |

"All" is not an estimate the client makes: the amount a request carries is an upper bound, and the
server moves what the character actually has and what the bank may still hold. A deposit therefore
does not fail because a few zen were spent while the window was open.

### Picking an item

**Right click** an item in the bank to pick it - the left button drags, so it cannot also select.
The number of the picked box is shown next to the balance. The pick is cleared whenever the server
sends the boxes again, so a stale pick cannot address the wrong item after something moved.

**Offer**, **Send item** and the price dialog all check that the pick is still an item; when it is
not, the system log says to pick one first and nothing is sent.

## The market page

**Market** switches to the offers of all players. Each entry shows what is offered, its price and
its seller; click one to select it.

| Button | What it does |
|---|---|
| **Prev** / **Next** | Turns the page |
| **Buy** | Buys the selected offer |
| **Take back** | Ends an own offer and gets what it held back |

What an offer holds has already left the seller's bank, so a seller can be offline - the deal does
not need him. What is bought arrives in the bank, not in the inventory.

## The history page

**History** shows what the bank booked, newest first, ten to a page. Every movement of value writes
one entry, so a player - and a game master looking for a duplication exploit - can read where a
value came from and where it went.

| Column | What it shows |
|---|---|
| **Time** | When it was booked, in the time of this machine |
| **Type** | Why it was booked: a deposit, a fee, a sale, a returned offer, a correction |
| **Detail** | What moved - the name of an item, or who the other side was |
| **Amount** | How much, green when the bank gained it and red when it lost it |

Click an entry to read it in full underneath: a description is cut to fit its column but not there,
and the balance which followed the movement is written beside it.

A movement of **items** books no amount - the entry names the item and its amount column stays
empty - so no balance is claimed for it either.

| Button | What it does |
|---|---|
| **Prev** / **Next** | Turns the page |
| **Refresh** | Asks for the shown page again |

The server sends no count of pages and no page size, so the window counts the page it is on rather
than promising a total it would have to invent. The end is found by turning past it once: the
window sees the empty page and goes straight back to the last one, so nobody is left looking at
nothing, and **Next** stops offering itself from then on. Asking for the first page again forgets
the end, because a bank which has booked something since has a new one.

The history is asked for when the tab is opened and not before, so a bank which nobody looks at
costs the database nothing. Any successful request made while it is open refreshes it, because
every one of them books an entry.

## Jewels are counted

A jewel put into the bank is consumed and counted; taking one out creates the item again. A
thousand Blessings therefore take no boxes at all, and can be sent or sold as an amount. The number
of jewels in the world does not change either way.

## When something is refused

The server answers every request, and the window writes *"The bank refused the request"* into the
system log when the answer is not a success - not enough of the currency, no free box, a name
nobody has, a daily limit reached, or an offer somebody else bought first. The balances and boxes
on screen are refreshed with the same answer, so what is left is always what is shown.

## What the window still cannot do

Offering one currency for another (zen for WCoin, for example) exists on the server but has no
dialog yet: it needs a second currency to be picked, not another number.
