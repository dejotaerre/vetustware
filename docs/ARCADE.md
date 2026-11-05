# Arcade · MAME 0.241

VETUSTware supports a separate Arcade library. It uses **MAME 0.241**, pinned to match the collection; it never silently falls back to a different system MAME or upgrades this engine through the general runtime updater.

## Library layout

Select the main library in Settings and place the collection in its `mame` subfolder, or choose another folder in **Arcade settings**. ROMs remain compressed and CHDs retain their machine subfolders:

```text
mame/
  roms/          ZIP/7z sets and machine-name/*.chd
  bios/          BIOS and device ROM sets
  samples/       Optional sound samples
  artwork/       Original cabinet artwork
  flyers/        Front artwork for cards
  snap/          Screenshots
  cabinets/      Cabinet photographs
  marquees/      Marquee images
  cpanel/        Control panels
  titles/        Title screens
  folders/       Optional catver.ini categories
  history/       Optional history.dat descriptions
  cfg/           Preserved initial settings
  nvram/         Preserved initial machine memory
  sta/           Preserved save states, if supplied
  catalog.json   Portable index, generated for 0.241
```

Filenames are case-sensitive on Linux. A BIOS archive called `qsound_hle.ZIP` must be named `qsound_hle.zip`; changing this filename does not change its contents.

## Catalog and verification

`helpers/mame_index.py` obtains machine metadata from the actual 0.241 executable using `-listxml`. It groups parents and clones, discovers candidate editions inside merged sets, and checks them with `-verifyroms`. BIOS/device entries are not shown as independent games.

The default view shows families with at least one verified edition whose emulation is not marked preliminary. A filter exposes incomplete and non-working families. Incomplete editions remain listed and cannot be launched from their card. “Best available dump” is distinguished from fully recovered ROMs. Successful ROM verification does not guarantee perfect emulation.

Artwork is matched by machine identifier, with a parent fallback. The flyer is preferred on cards, then cabinet photography or a screenshot. Descriptions and categories retain their original language and can be incomplete when their source is older than the engine.

**Update catalog** writes a new personal index and audit log under the installation's `data/arcade`; it does not alter the shared ROM collection. The library index uses relative media paths and can be copied with the collection. Favorites, hidden games and selected editions are personal settings in `config/arcade.json`.

## Engine and personal data

The native runtime must be placed in `runtime/mame-0.241`, with its `mame` launcher. The launcher helper checks the actual engine version and re-audits the chosen edition before starting. Runtime resources and plugins must match 0.241; arbitrary legacy plugins in a game collection are not loaded automatically.

Settings, NVRAM, states, disk differences and recordings live in `data/mame/0.241`. Original `cfg`, `nvram`, `sta`, `hi`, `inp`, `diff` and `comments` files are copied there only if the destination file does not already exist. Existing personal saves are never overwritten by seed files. New sessions do not write into the shared library. Copy this personal folder separately when migrating progress; sharing the library supplies its preserved initial settings and saves, not later personal sessions.

Arcade uses the monitor chosen in global settings, with fallback to an available screen if disconnected. For fullscreen on Wayland desktops, the selected output is matched by geometry to the native Wayland display, avoiding XWayland fullscreen placement errors. Windowed sessions keep X11 positioning, which Wayland does not allow applications to control. X11 also remains the fallback when no matching native output is available. Fullscreen and cabinet artwork are configurable in Arcade settings. Scaling preserves each machine's aspect ratio, including vertical games.

Typical controls: `5` inserts a coin, `1` starts, `Tab` opens MAME settings, and `Esc` exits. `Shift+F7` saves a state and `F7` loads it on supported machines. Controls can vary by machine.

## Runtime provenance and publication

The local 0.241 runtime is assembled from the signed historical native Arch package and private shared libraries, with provenance, licenses and file checksums. It is tested on the current CachyOS x86_64 installation and still needs host glibc, graphics drivers and desktop services. It is not a universal Linux package. DEB/RPM portability work remains deferred.

Public packaging must include the corresponding source and license notices required by the runtime components. ROMs, CHDs, artwork and user settings are never included in the application's source package. No emulator update should replace 0.241 without an explicit ROM compatibility review.

The collection keeps its original auxiliary files in `resources/` next to `roms/` and `metadata/`. They travel with the collection. MAME still loads the version-matched resources from its runtime; moving these preserved files does not enable legacy plugins.
