# Contributing to VETUSTware

**English** · [Español](CONTRIBUTING.es.md)

Use GitHub Issues for bug reports and pull requests for changes. The initial Linux build was tested on CachyOS. Reports from other distributions help establish where the application runs.

## Report a problem

Include your distribution, Qt version, desktop environment, X11 or Wayland session, VETUSTware version, emulator and steps to reproduce the problem. For game-specific issues, give the title and edition.

The output of `vetustware --diagnostics` can help identify the cause. Review diagnostics, logs and screenshots for personal information such as local paths before posting them. Keep private saves, game files, disc images, ROMs and keys out of reports.

## Submit a code change

1. Build the application and run the tests using the [README instructions](README.md#build-from-source).
2. Keep the change focused on the issue it addresses.
3. Add a test when the behavior change needs one. Use synthetic fixtures for game data.
4. Write new source code comments in neutral Spanish.
5. Describe what changed and how you verified it.

Before submitting, check your diff for personal paths, logs, game libraries and local `data/`, `config/` or `runtime/` files. Contributions to the project's own code use its MIT license.

## Contribute a translation

The interface ships with English, Spanish and Brazilian Portuguese. To add or correct a language, edit a JSON file in `translations/`. The translation guide covers the file format, validation and how to preview your changes without rebuilding. [Translating VETUSTware](docs/TRANSLATING.md)
