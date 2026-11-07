# Third-party components and artwork

**English** · [Español](THIRD_PARTY.es.md)

The MIT license in this repository applies to VETUSTware’s own code, documentation and application icon. Dependencies, emulator runtimes and material supplied in a game library retain their respective licenses.

## Libraries and DOS emulators

The application package uses system libraries. It does not redistribute Qt, SDL, libzip, DOSBox, games, ROMs, MIDI sound banks, game covers, manuals, XTree or Norton Commander.

- Qt modules used include Core, Gui, Qml, Quick, QuickControls2, Sql, Concurrent, Pdf and PdfQuick. [Qt licensing](https://www.qt.io/licensing/)
- [SDL license](https://www.libsdl.org/license.php) · [libzip license](https://libzip.org/license/)
- [DOSBox Staging](https://www.dosbox-staging.org/) · [DOSBox](https://www.dosbox.com/) · [DOSBox-X](https://dosbox-x.com/)

Users supply eXoDOS collections and metadata locally. The application does not download games. The screenshots in `docs/images/` show material from a local library. Game images, trademarks and other third-party content shown there belong to their respective owners. The application package does not distribute the underlying game files.

## Optional emulator runtimes

**Stella and ScummVM.** VETUSTware can import private runtimes from native system packages or install compatible runtime packages published separately. Each runtime carries its own licenses, resources and provenance. Stella is GPL-licensed. VETUSTware invokes its executable and queries its local database with `-listrominfo`. Generated catalogs stay with the user's library and retain Stella metadata. The application package does not bundle Stella's ROM database or Atari ROMs. [Stella](https://stella-emu.github.io/) · [Runtime preparation and distribution](docs/RUNTIMES.md)

**MAME 0.241.** Arcade requires a matching runtime. MAME uses GPL-2.0-or-later and some components carry additional notices. The locally prepared runtime records its signed Arch package origin and private dependency versions in `PROVENANCE.json` and retains license files. Before distributing it, provide the corresponding sources and notices for every bundled component. Library ROMs, CHDs and artwork are separate third-party material. [Arcade library and runtime](docs/ARCADE.md)

## Section artwork

- **Atari 2600.** The user supplied the console photograph in `native/assets/atari2600-header.png`. Its redistribution license has not been identified. The image and trademarks are outside the scope of the code's MIT license.
- **ScummVM.** The illustration in `native/assets/scummvm-header.png` was AI-generated for the interface using a thematic reference supplied by the user. It is unofficial artwork. The ScummVM name and trademarks belong to their owners.
- **Arcade.** The header in `native/assets/arcade-header.png` was composed and cropped with AI using Pac-Man, Donkey Kong and Street Fighter II cabinet images from the user's `mame/cabinets/` collection. The reference images and marks belong to their owners. Their redistribution licenses have not been identified.
