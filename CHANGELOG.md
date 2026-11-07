# VETUSTware changelog

**English** · [Español](CHANGELOG.es.md)

## Unreleased

- Private Stella/ScummVM runtimes with manual GitHub update checks, compatibility and integrity validation, installation, and previous-version restoration. Initial system importer supports Arch/CachyOS x86_64 and keeps settings and saves separate.

- Clicking the cover in DOS, ScummVM and Atari game details launches the selected edition, using the same availability checks as Play.

- ScummVM and Atari 2600 headers include responsive artwork for ScummVM and the Atari console.

- Stella uses native Wayland when the selected monitor can be identified, avoiding
  XWayland fullscreen placement and image-size errors with SDL compatibility layers.

- Atari cards now show box art, with snapshots retained in a separate view
  inside game details. Explicit portable ROM-to-cover associations survive rescans.

- Atari 2600 library with Stella: portable ROM/artwork catalog, grouped variants,
  favorites, hidden games, configurable folder, fullscreen and picture style.
- Stella uses an isolated personal directory for new states and controller settings;
  preserved historical states remain separate from current saves.

- Added Brazilian Portuguese (Português (Brasil), pt_BR) with all 407 interface messages translated.

- Home shows the loaded DOS catalog total instead of a fixed number, formatted in the interface language.

- Game details use a top-right close icon, matching the PDF viewer, in both DOS and ScummVM.

- ScummVM games folder can be selected independently in Library and folders, with an automatic library subfolder default.

- ScummVM games can be hidden or shown again by folder. Visibility preferences persist across sessions.

- ScummVM folders can include a local cover and a PDF manual, opened with the internal reader.

- Added a ScummVM library tab with folder-by-folder detection, edition selection,
  catalog artwork where available, search, direct launch and the native manager.
- ScummVM settings and new saves are kept within VETUSTware, independently of
  DOSBox and the user's standalone ScummVM configuration.

### Arcade

- Fixed fullscreen games opening on the wrong monitor under Wayland by matching the selected display to the native Wayland output.

- Added a dedicated Arcade library pinned to MAME 0.241, with audited ROM sets, grouped regional editions, categories, favorites and hidden games.
- Added flyers, cabinet photographs, marquees, control panels, title screens and game histories to the game details view.
- Added configurable library location, global monitor selection, fullscreen and cabinet artwork controls; preserved seed settings and saves are imported without overwriting personal progress.
- Added English, Spanish and Brazilian Portuguese interface strings, catalog/launch tests and Arcade documentation. No release has been published for these changes.

## 0.3.4 — 2026-10-31

- Left/Right arrow keys navigate pages or spreads while the PDF reader is open.
- PDF reader: optional two-page spreads, with a standalone cover followed by pages 2–3, 4–5, and navigation/fit controls for the whole spread.

## 0.3.3 — 2026-10-31

- English interface by default, with Spanish selectable live in Settings.
- Persistent interface language independent of game editions and emulation settings.
- Community JSON translations, validation helper and English contributor guide.
- English README with new screenshots; Spanish README and screenshots preserved.
- English installation instructions included in the download.
- Installer preserves interface language and locally added translation files on update.

## 0.3.2 — 2026-10-31

- Added Magazines with collections, local covers, search and keyboard navigation. The library supplies the publications separately from the application.
- PDF magazines and original indexes open in the integrated reader.
- Big Blue Disk and Game Bytes open their original DOSBox menus from local working copies. They use global emulation, monitor and image settings and stay out of recently played games.
- DOSBox Staging defaults to Sharp rendering. Classic monitor (CRT) can be selected globally or per game while preserving the aspect ratio.
- The PDF viewer centers portrait pages, starts each document on its first page and closes with X or Escape.
- Updated source and binary packages, magazine format documentation, and portability and data-preservation tests.

## 0.3.1 — 2026-10-31

- Game details can open supplementary documents and media from the library.
- Added an integrated PDF reader and SHA-256 verification when extracting extras.

## 0.3.0

- First public release of the native application and clean Linux packages.
