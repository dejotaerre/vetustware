# Translating VETUSTware

The interface defaults to **English**. **Spanish** is included. Choose a language
in **Settings → Interface language**; the change takes effect immediately and is
saved in the installation's `config/interface.json`. This does not change game
editions, emulation settings, saves, or the contents of the shared library.

Translations are UTF-8 JSON files in `translations/`. No C++ knowledge, Qt
Linguist, compilation, or external translation service is required.

## Add a language

1. Copy `translations/en.json` to a new file, such as `translations/fr.json`.
2. Set `language` to the filename without `.json` (`fr` in this example). Use
   two or three lowercase letters, optionally followed by underscore-separated
   region or script codes, such as `pt_BR` or `zh_TW`.
3. Set `name` to the language's own name, such as `Français`.
4. Translate the **values** in `messages`. Keep the English keys unchanged.
5. Validate the file from the repository root:

   ```sh
   python3 helpers/check_translations.py
   ```

6. Copy your JSON into the installed application's `translations/` directory
   (normally `~/vetustware/translations/`) and reopen VETUSTware. The language
   appears automatically in Settings. No rebuild is needed. You can also validate an installed copy with
   `python3 ~/vetustware/helpers/check_translations.py --allow-incomplete`.
7. Submit a pull request containing the new JSON file. Include screenshots of
   the main screen and Settings in your language, plus any wording that needs
   a second opinion. Code and translation contributions use the project's MIT
   license; third-party game content is separate.

Example excerpt (a complete file has many more messages):

```json
{
  "language": "fr",
  "name": "Français",
  "messages": {
    "Save": "Enregistrer",
    "Settings": "Paramètres",
    "Issues · %1 · PDF": "Numéros · %1 · PDF"
  }
}
```

## Translation rules

- Preserve placeholders such as `%1` and `%2`. You may reorder them to fit your
  language, but do not remove, duplicate, or rename them.
- Preserve leading/trailing spaces in message fragments and intentional `\n`
  line breaks. Preserve shortcut names and DOS commands (`Ctrl+F9`, `DIR`, `EXIT`).
- Keep product names and paths unchanged. Translate descriptions, not game
  names, filenames, engine identifiers, or configuration values.
- Use concise wording in buttons. Test both the regular and advanced settings
  layouts, game details, magazines, extras, and the PDF viewer.
- A missing, empty, or invalid-placeholder translation falls back to English.
  Use `--allow-incomplete` with the checker while working; complete translations
  are required by the repository's default checks. Malformed JSON files are not
  offered in the language selector.
- Game descriptions, original manuals, magazine descriptions and third-party
  file names belong to the library. They are not automatically translated.
  The original DOSBox interface and original game menus have their own languages.
- Qt's standard dialogs use the corresponding Qt translation when installed.
  Custom VETUSTware translations do not replace the desktop's native file dialog.

## For developers

Use English source messages:

```qml
text: qsTranslate("Vetust", "Save")
```

```cpp
#include "i18n.h"
const auto label = uiText("Issues · %1 · PDF").arg(count);
```

Add every source message to `en.json` (key and value identical) and translate it
in `es.json` and other maintained catalogs. Prefer complete messages with
placeholders to concatenating fragments. For counts, labels such as
`Issues · %1 · PDF` avoid assuming English plural rules.

The JSON translator uses Qt's translation hooks; QML bindings are refreshed
through `QQmlEngine::retranslate()`, and backend-generated labels are rebuilt
without changing stored IDs or language choices for individual games.
References: [QTranslator](https://doc.qt.io/qt-6/qtranslator.html) and
[QQmlEngine::retranslate](https://doc.qt.io/qt-6/qqmlengine.html#retranslate).

The installer and release packager include `translations/*.json`. Updating the
application preserves the selected language and locally added language files;
an existing packaged translation is updated, with the previous copy backed up.

Run the complete checks before submitting:

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build -j4
ctest --test-dir native/build --output-on-failure
```
