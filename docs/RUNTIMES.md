# Private runtimes and updates

Open **Settings → Emulators and updates**. VETUSTware can keep Stella and
ScummVM under its installation folder:

- `runtime/stella/stella`
- `runtime/scummvm/scummvm`

These launchers use the executable in `bin/`, private libraries in `lib/` and
packaged resources in `share/`. The app prefers them to system executables.
Settings, savegames and the shared game library remain outside those folders.
Copying an installed system version does not uninstall or change system packages.

The first importer supports Arch/CachyOS Linux x86_64. Its packages include a
conservative minimum glibc version and CPU requirements. They are **not universal
Linux binaries**: glibc, graphics drivers and desktop/audio services still come
from the host. Other distributions need separately built and tested packages.
Only package-owned resources are imported; unrelated files in `/usr/share` are
not copied. Dependency provenance and license notices travel with each runtime.

## Update behavior

Checks are manual; no background network access or automatic installation is
performed. The updater reads stable, non-draft releases of
`https://github.com/dejotaerre/vetustware`, looking for the asset
`vetustware-runtimes-v1.json`. It reports honestly when no compatible package is
published. An application release alone is not an emulator update.

Each package declares engine, version, packaging revision, architecture,
distribution family, minimum glibc and CPU flags, HTTPS download URL, exact size
and SHA-256. Downloads are accepted only from this repository's release assets
(with HTTPS redirects for GitHub storage). Integrity relies on that trusted HTTPS
source and hashes; this is not an independent signature infrastructure.

Before replacing a runtime, the updater rechecks the published offer, validates
size and SHA-256, extracts without links or unsafe paths, verifies every file,
and checks startup with isolated temporary settings. A failure leaves the current
runtime available. Updater operations hold an exclusive lock, block new game
launches and refuse to replace a running private emulator.

`runtime/.previous/<engine>` holds the previous installation. **Restore previous
version** swaps it with the current one. Older backups are retained in
`runtime/.history`. A transaction journal recovers interrupted renames. Runtime
rollback does not roll back savegames or emulator configuration formats.

## Preparing a release (maintainer)

On an isolated supported build system with the desired native packages installed:

```sh
python3 helpers/runtime_package.py --engine stella --output build/runtime-stella \
  --package-dir build/runtime-release --release-tag runtimes-YYYYMMDD
python3 helpers/runtime_package.py --engine scummvm --output build/runtime-scummvm \
  --package-dir build/runtime-release --release-tag runtimes-YYYYMMDD
```

Use empty output directories. Review `PROVENANCE.json`, provide the corresponding
source archives and required license notices for all redistributed components,
and test actual games, audio, controllers and monitor selection on supported
machines. These checks are additional to the startup probe. Do not label a package
as compatible with an untested distribution or weaken its hardware requirements.

Publish both `.tar.gz` packages and `vetustware-runtimes-v1.json` as assets of the
stable release named by `--release-tag`. Never reuse a published package URL for
different content; increment the packaging revision when rebuilding the same
upstream version. Nothing is published automatically by the helper or app.

The application package includes the manager and preparation tools, but no
emulator binaries. Runtimes can be imported locally or downloaded once compatible
packages have been published. The normal application installer preserves existing
runtimes, configurations and saves.

MAME is a separate pinned runtime under `runtime/mame-0.241`. Arcade validates the executable version before scanning or playing and does not offer MAME through the automatic version-update workflow. See `ARCADE.md` for ROM compatibility and preservation of personal data.
