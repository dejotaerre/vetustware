# Home featured games

The Home banner rotates through a curated set of recognizable DOS classics
available in the user's catalog. It is an editorial selection, **not a sales
ranking or a claim that every title is among the best-selling games**.

## References

The selection was informed by these retrospectives, consulted on November 6, 2026:

- [PCWorld: The 10 Greatest MS-DOS Games of All Time](https://www.pcworld.com/article/477269/the-10-greatest-ms-dos-games-of-all-time.html)
  (2011): includes Day of the Tentacle, Doom, Civilization, X-COM and other DOS classics.
- [GamesNostalgia: Top MS-DOS Games](https://gamesnostalgia.net/ms-dos): includes
  both Monkey Island games, Day of the Tentacle, Lemmings, Warcraft II and Dune II.
- [Computer Gaming World, November 1996, issue 148](https://www.cgwmuseum.org/galleries/index.php?id=148&pub=2&year=1996):
  the historical “150 Best Games of All Time” retrospective. Its recognition of
  The Secret of Monkey Island is also recorded in
  [MobyGames' game entry](https://www.mobygames.com/game/616/the-secret-of-monkey-island/).

The shortlist also includes editorial additions from familiar LucasArts and
Sierra adventures, action games, platformers and strategy games to keep the
rotation varied. It does not reproduce any source's ranking or order.

## Behavior

- The list and edition aliases are in `classicSpotlights()` in
  [`native/home.cpp`](../native/home.cpp). Exact, case-insensitive title matching
  avoids accidentally selecting demos, unofficial levels or unrelated sequels.
- Only titles found in the local catalog participate. There are no network
  requests when opening Home.
- Alternate editions share one slot; aliases are listed in preferred order.
  This selects an existing game entry, without changing its selected language,
  launch profile or saves.
- Already played games and favorites remain eligible.
- Returning to Home chooses a new random selection, favoring existing local
  recommendations where possible. The last 12 featured IDs are remembered
  across restarts. Small collections reuse the least recently featured titles.
- The responsive row shows one, two or three cards, limited by the number of
  eligible games. Resizing preserves the visible selection where possible.
- A catalog with no recognized classics falls back to its own games, so Home
  still works for custom collections.
- Other Home shelves, including discovery and activity-based recommendations,
  retain their existing behavior.

There is no telemetry, popularity tracking or automatic download of games.
