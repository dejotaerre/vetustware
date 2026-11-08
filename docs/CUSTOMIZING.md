# Customizing DOSBox game details

Open a DOSBox game's details and choose **Customize details**. This editor is exclusive to DOSBox.

- **Information:** edit the title, year, developer, publisher and genres (separated by semicolons). Edit each description independently using the language selector. This does not automatically translate the text or change the game's language.
- **Images:** choose a cover, a 3D box, an optional background and gallery images. Replace, remove or reorder gallery images. Normal images are accepted; VETUSTware blurs the details background when displaying it. Without a background, the cover is used.
- **Save:** apply the changes immediately to the catalog and details. Selected files are imported into the library, so their source copies are no longer needed.
- **Cancel**, **×** or **Escape:** discard edits since opening the editor. No images are imported until saving.
- **Restore original details:** after confirmation, remove the saved overrides for this game and return to the library's underlying metadata, including its translated descriptions.

## Storage and portability

The DOSBox collection's `metadata/customizations/<game-id>.json` contains only overridden values. The original catalog and description files remain unchanged. Imported images are stored as PNG files under `metadata/customizations/images/`, named by their content hash. JSON references are relative to this folder; original artwork remains referenced by its catalog asset identifier.

Copy the entire DOSBox collection, including `metadata`, to retain these edits. No game files, emulator profiles, identifiers or saves are changed. ScummVM's automatic DOS artwork lookup continues to use the base catalog, not these personal overrides.

Images are limited to 80 MiB per source file and 100 megapixels; images larger than 4096 pixels on either axis are resized proportionally, retaining transparency. The gallery supports up to 100 images. A save validates all inputs, imports any images, and atomically replaces the per-game JSON last. If validation fails, the saved details remain unchanged. The library must be connected and writable.

Restoring a game removes its JSON override. Previously imported image files are retained, because they can be shared by other customized games. Changing an original catalog asset identifier in a future library update can require reselecting that original image.
