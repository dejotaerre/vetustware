# Local libraries

**English** · [Español](BIBLIOTECAS.md)

## Choose collection folders

In **Settings → Libraries and storage**, select the DOSBox, ScummVM, Atari 2600, Arcade and Magazines folders you want to use. They can be on different drives. **Detect folders** can identify several collections in an organized library at once. Each collection carries its own `metadata/` folder. Changing a location tells VETUSTware where to look; copy or move the files yourself first. Folder names such as Games or Juegos have no special meaning.

An organized library can use this layout:

```text
library.json
games/{dosbox,scummvm,atari2600,mame}/
games/{dosbox,scummvm,atari2600,mame}/metadata/
games/dosbox/resources/audio/{roland-mt32,soundfonts}/
games/dosbox/resources/dos/
games/mame/resources/
magazines/
magazines/metadata/
```

Each engine folder can be archived separately with its metadata and resources. Extracting a ZIP may use more disk space if the original contained hard links. The organized layout requires a development build that supports it. The published 0.3.4 preview uses the earlier library formats. The importer also recognizes earlier organized libraries with Spanish folder names.

## DOS games and custom editions

In the organized layout, DOS games live in `games/dosbox/archives/`, with custom editions in `versiones-especiales.zip`. Under `games/dosbox/metadata/`, `perfiles.zip` holds the custom catalog, `recursos.zip` holds images and manuals, `perfiles-originales.zip` holds importable profiles and `extras/` holds supplementary material. The shared-resource map `recursos-compartidos.json` refers to `recursos.zip`.

Roland ROMs and SoundCanvas banks belong in the DOS collection’s `resources/audio/`. Console tools belong in `resources/dos/`. The importer interprets historical names inside original profiles and ZIPs without requiring matching directories outside the archives.

### Original eXoDOS layout

The following layout has been tested with compatible eXoDOS collections. Support can vary between collection versions.

```text
library/
├── Content/
│   ├── XODOSMetadata.zip
│   └── !DOSmetadata_linux.zip
└── eXo/
    ├── eXoDOS/    # Game ZIP archives
    └── mt32/      # Sound resources required by some profiles
```

`XODOSMetadata.zip` supplies the catalog, images and manuals. The Linux metadata package supplies DOSBox profiles. VETUSTware extracts game files as needed.

A custom library prepared for 0.3.0 can add:

```text
Personalizaciones/
├── perfiles.zip
├── juegos.zip
└── herramientas.zip   (optional)
```

Choosing **Play** or opening a DOS console prepares the selected edition. If the library supplies DOS tools, the console prepares them locally. Game profiles contain DOSBox commands and mounts, so use libraries and profiles from sources you trust. The repository supplies the application separately from the content. Missing game files are not fetched online.

### Personal data and recovery

Favorites, preferences and play history belong to each installation. Playable DOS copies and saves default to `data/games/`. You can choose another location in advanced storage settings. Playing a game writes to its working copy. If you move that directory, copy all files, including hidden ones, before changing its path.

The library can restore an initial edition when the working copy is missing. Back up the working copy separately to keep later saves or modifications. An existing incomplete folder is preserved for review. Editing a local installation does not update a shared snapshot. Export new customizations separately; this version has no general-purpose library exporter.

### Shared resources and duplicate files

In legacy libraries, `Content/recursos-compartidos.json` maps historical names for identical images and manuals to members of `XODOSMetadata.zip`. It uses `format: 1` and an `archives` object. Its `XODOSMetadata.zip` mapping gives the retained `entry`, `bytes` and `sha256` for each historical name. Targets must exist in that archive. External paths and alias chains are rejected.

The catalog in `Personalizaciones/perfiles.zip` refers directly to retained members. When rebuilding from XML, the importer also reads the map to preserve image IDs and the association between historical names and games. Keep the map with its ZIP.

MAME artwork can be unpacked into system-named folders containing layouts and images. Identical files may be hard-linked. Break the link before editing one copy independently. Copying without preserving hard links retains the content but can increase disk use.

### Supplementary documents and media (0.3.1 and later)

```text
Complementos/
├── metadata-original.zip
├── variantes-locales.zip
├── conservacion.json
├── extras.json
├── SHA256SUMS
└── LEEME.txt
```

The two ZIPs preserve original documentation and historical technical variants. Active profiles still come from `Personalizaciones/perfiles.zip`. The optional `conservacion.json` inventory records original file coverage using relative paths and SHA-256 hashes.

`extras.json` uses `format: 1` and a `games` object keyed by catalog IDs. Each document specifies `name`, `kind`, `bytes`, `sha256`, `archive` (relative to the library) and `entry` (the ZIP member). An entry can reuse an existing archive. VETUSTware verifies and extracts only the selected document or media file. Scripts, executables and paths outside the library are rejected. HTML that references other files may need additional resources. Opening an external format requires a suitable viewer.

## Magazines

In the organized layout, `magazines/metadata/revistas.json` holds the catalog and `metadata/covers/` holds cover images. Media paths and cover paths are relative to the magazines folder. Version 0.3.2 introduced the earlier `Revistas/revistas.json` format. Current builds can still read root-level catalogs and older shared metadata layouts.

Format 1 has two lists:

- `series` entries include `id`, `title`, `description`, `kind` (`pdf` or `dos`) and `count`. Optional fields include `cover`, `indexId` for a PDF index and `launchId` for a DOS magazine.
- `entries` records include `id`, `series`, `title`, `kind` and `path`. PDFs may include `cover`, `bytes` and `isIndex`. An index does not duplicate an issue in the grid. A DOS entry includes `revision`, a SHA-256 digest of its contents inventory.

PDFs open directly. The reader uses the local catalog for issue navigation. Original indexes may contain historical or external links. Cover images are local and external file paths are rejected.

The supported DOS magazine profiles are `BBD` and `GameBytes`, each with a root-level `run.bat`. VETUSTware atomically prepares a complete copy at `data/magazines/<folder>-<revision>` and preserves any existing incomplete copy. DOSBox runs the magazine menu using global emulation, monitor and image settings. Library shell scripts are not executed. Magazine sessions stay out of recently played games.

The whole magazine folder can move with the library. Optional `conservacion.json` and `SHA256SUMS` files document integrity. Local working copies may contain reader changes and belong with personal backups.

## ScummVM

Place each extracted game in its own subfolder. First access scans the collection, including nested subfolders. Use **Update catalog** after adding games. Detection is per folder to keep editions distinct. If ScummVM reports several matches, select the edition. An unmatched folder stays visible for review. Successful detection alone cannot guarantee that an edition runs.

In the organized layout, local art and manuals go in `games/scummvm/metadata/<game-folder>/`. A game folder can also provide `manual.pdf` and `portada.jpg`, `cover.jpg`, `portada.png` or `cover.png`. Local cover art takes priority. A known equivalent DOS game can supply catalog art as a fallback. Entries without art show their title. The internal PDF reader supports spreads and keyboard navigation.

**Manage in ScummVM** opens the native manager with the application's dedicated configuration. You can edit game-specific graphics, sound and other settings there. Removing a manager entry leaves the game files in place and a rescan can detect it again. VETUSTware lists games in the configured collection.

| Personal data | Location under the installation |
| --- | --- |
| Configuration | `config/scummvm.ini` |
| New saves | `data/scummvm/saves/<identifier>` |
| Configuration backups | `backups/scummvm` |
| Logs | `logs/scummvm*.log` |

Rescanning preserves settings and leaves game files alone. A standalone ScummVM installation keeps its own settings and saves. A custom save path chosen in the manager is honored. VETUSTware can use the system executable or a private one at `runtime/scummvm/scummvm` (`scummvm.exe` on Windows). See [private runtimes](RUNTIMES.md). The executable naming convention does not imply a complete Windows port.

Use **Hide game** in a folder's details to hide it from the default view and search. **Show hidden** includes hidden entries and lets you restore them. The choice is personal, persists in `config/scummvm.ini` and follows a collection when moved.

To use a different drive, choose the folder containing the games and their `metadata/` under **Settings → Libraries and storage**. This rescans without moving files. Folders that retain their names keep their settings, hidden flags and saves. The optional `scummvm` path in `config/storage.json` is retained if the drive is disconnected.

## Atari 2600 and Stella

Choose the folder under **Libraries and storage** or **Atari settings**.

```text
atari2600/
├── roms/             # .bin and .a26 files, including subfolders
└── metadata/
    ├── boxes/
    ├── snapshots/
    ├── covers.json
    └── catalog.json
```

The index groups variants by name and identical copies by MD5. It reads ROMs without modifying them or save states. Stella’s local database supplies known manufacturers, controllers and formats. Some titles come from filenames and may need review. Art from another edition of the same title can serve as a fallback.

To generate a portable catalog for your own library:

```sh
python3 helpers/atari_index.py --library /path/to/atari2600 \
  --output /path/to/atari2600/metadata/catalog.json --stella /usr/bin/stella
```

**Update catalog** writes a personal index inside the installation. The shared folder stays portable. VETUSTware settings are in `config/atari.json`. Stella uses `data/stella` for settings, saves, captures and persistent memory. Set controllers in Stella, including paddles for games that need them. Display selection follows the global monitor setting. Preserved Stella 3.2.2 states remain separate from new states and may be incompatible with newer versions.

### Box art and snapshots

Cards show box art or a missing-cover indicator. Gameplay snapshots remain available in game details. Images retain their proportions. Store boxes in `metadata/boxes/` and snapshots in `metadata/snapshots/`. The following `covers.json` example associates an image with a ROM hash:

```json
{"format":1,"variants":{"ROM_MD5":"boxes/box-name.png"}}
```

Image paths are relative to Atari's `metadata/`. Several variants of one title can share a box. Associations survive moving the collection and rescanning. Refresh the catalog after edits. Paths outside the collection are rejected. A newer shared catalog has priority over a local cache.

The prepared collection includes `box-matching.json`, which records reviewed matches, title equivalents and unresolved or ambiguous cases. Approximate name matches do not automatically assign a box.

## Arcade and MAME 0.241

Choose the MAME folder in **Libraries and storage** or **Arcade settings**. The catalog groups regional variants and verifies ROMs against MAME 0.241. The collection can include flyers, cabinet photographs, marquees, screenshots, initial settings and initial saves. Artwork paths in the catalog are relative to the metadata folder. The Atari and MAME indexers accept `--metadata`; the MAME launcher uses the same option to locate artwork.

MAME auxiliary resources travel in `resources/`. The runtime uses its matching 0.241 plugins rather than loading legacy plugins from a collection. New sessions write to `data/mame/0.241` in the personal installation. See the [Arcade guide](ARCADE.md) for verification and migration details.
