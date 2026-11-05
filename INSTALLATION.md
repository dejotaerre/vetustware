# VETUSTware 0.3.4 — installation

[Español](INSTALACION.md)

VETUSTware is a native Linux application for a compatible local DOS game library.
Its code is MIT-licensed. No browser or online account is required.

## Downloads and requirements

- `vetustware-0.3.4-linux-x86_64.tar.gz`: executable, installer, helpers, source,
  English/Spanish interface translations and documentation.
- `vetustware-0.3.4-fuentes.tar.gz`: source and documentation for a local build.
- `SHA256SUMS`: checksums for both archives. Run `sha256sum -c SHA256SUMS` in a
  directory containing both downloads to verify them.

The binary is a **Linux x86_64 preview**, built on CachyOS with Qt 6.12.
It requires compatible system libraries; it is not a universal Linux package.
See `BUILD_INFO.json` for the exact build environment.

Install Qt 6 (Quick, QuickControls2, Sql with SQLite support, Concurrent,
Pdf/PdfQuick, QML dialogs and QtQuick.Effects), libzip, SDL2 and Python 3 from your distribution.
A Linux desktop with X11 or XWayland and OpenGL is required. Install DOSBox
Staging, classic DOSBox or DOSBox-X separately.

The downloads contain no games, ISOs, artwork, manuals, magazines, emulators,
third-party DOS tools, saves or personal configuration. Supply a compatible
library separately. See [the library format (Spanish)](docs/BIBLIOTECAS.md).

## Install or update

Extract the binary package into a temporary folder and open a terminal there.
Before updating an existing installation, close VETUSTware and DOSBox.

```sh
ldd ./vetustware
python3 helpers/install.py --desktop
```

If `ldd` reports a missing library, install it first. If Qt, GLIBC or GLIBCXX
versions are incompatible, build from source below. The installer needs no sudo.

The default destination is `~/vetustware`. For an existing installation elsewhere,
add `--dest /path/to/installation`. Use a destination separate from the extracted
package. The installer backs up replaced program files and preserves settings,
favorites, catalog, saves, local magazine copies and existing emulator runtimes.
It also preserves the chosen interface language and locally added translation
files; bundled translation files are updated with the old copies backed up.

Open VETUSTware from the application menu or run:

```sh
"$HOME/vetustware/vetustware"
```

## Language, library and emulator

The interface starts in English. Choose **Settings → Interface language → Español**
for Spanish. The change is immediate and persists across sessions. It does not
translate games, imported descriptions, manuals or magazines.

At first launch, choose your compatible library folder, which may be on another
drive. Playable copies and saves use `data/games` inside the application folder
unless you choose another location. No folder named Games or Juegos is assumed.
The application reads the shared library and writes player changes to local copies.
Deleting a playable copy also deletes any saves stored only in that copy.

The installer detects DOSBox executables in PATH, preferring Staging. Check
**Settings → Engine**. For a nonstandard executable location, expand
**Advanced options → Custom executable**. Each game can use its own engine and
settings. Imported profiles do not guarantee every game's compatibility.

To contribute a language, see [Translating VETUSTware](docs/TRANSLATING.md).
The installed copy includes the guide as `TRANSLATING.md` and translations in
`translations/`. Additional JSON language files require no recompilation.

## PDF reader

Manuals, PDF extras and magazines share the same reader. Choose **Single page**
or **Two pages** in its toolbar. In two-page mode the cover is alone, followed
by pages 2–3, 4–5, and so on. A final unpaired page is shown alone.

Use **Left/Right arrow keys** or Previous/Next to move between pages or spreads.
The keys work while the reader is open, except while the layout menu is expanded.
**Fit page** and **Fit width** size the whole current view without distortion.
Changing the layout keeps the selected page. Each document opens at its cover;
the chosen layout lasts for the application session. Close with **X** or **Escape**.

## Build from source

Install a C++20 compiler, CMake 3.21 or newer, pkg-config, Python 3, and development
packages for Qt 6.5 or newer (including Qt Test), libzip and SDL2.

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build -j 4
ctest --test-dir native/build --output-on-failure
python3 helpers/install.py --desktop
```

The installer prefers the locally built executable in `native/build`. Tests use
synthetic fixtures and do not require a game collection.

## Data and diagnostics

Keep `config/` (settings), `data/` (catalog, user activity and local content),
`runtime/` (locally added tools) and any separately configured game folder when
moving an installation. Change library/game paths through Settings only after
copying the content; changing a path does not move files.

```sh
"$HOME/vetustware/vetustware" --diagnostics
```

Diagnostics contain local paths. Remove personal information before posting them.
Distribute the clean release archive, not a used installation containing your data.
See [THIRD_PARTY.md](THIRD_PARTY.md) for dependency and content licensing.

## Atari 2600 with Stella (development version)

Install Stella from your distribution: for example `sudo pacman -S stella`
on Arch/CachyOS, or `sudo apt install stella` on Debian/Ubuntu.
The integration has been tested with Stella 7.0 on Linux.

Place ROMs (`.bin` or `.a26`) in `atari2600/roms` and images in
`atari2600/snapshots` inside your library. Open **ATARI 2600**, then
**Update catalog**. A portable `catalog.json` can also accompany this folder.
Use **Atari settings** to choose another location. New preferences and saves
are stored in `config/atari.json` and `data/stella` under the installation,
not in the shared library. Back up these folders to retain your progress.

Use **Manage in Stella**, or press **Tab** during a game, for controller
mapping and individual game options. Stella 7 defaults: **F9** saves a state,
**F10** selects the next slot, **F11** loads it. Old states from other Stella
versions may be incompatible; preserved legacy files are not imported automatically.
Close Stella as well as VETUSTware before updating the installation.

## Private emulators

Settings → Emulators and updates manages Stella and ScummVM inside the application folder. The first local importer supports Arch/CachyOS x86_64. Update packages must be published separately; see [runtime management](docs/RUNTIMES.md).
