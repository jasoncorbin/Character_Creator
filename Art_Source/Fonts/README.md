# Font sources — Fredoka & Nunito

Static instances cut from the Google Fonts **variable** originals (upstream ships variable-only).

- Source: `github.com/google/fonts/ofl/{fredoka,nunito}`
- Cut with `fontTools.varLib.instancer` at wght 400 / 600 / 700.
- ⚠ Do NOT import the variable TTFs directly: Fredoka's default is **wght 300** and Nunito's is
  **wght 200**, so UE (which uses only a variable font's default instance) would give you
  Light / ExtraLight while calling them Regular.
- Licence: SIL Open Font License 1.1 — see `OFL-Fredoka.txt` / `OFL-Nunito.txt`. Retained here
  because the OFL requires the licence to accompany the fonts.

Imported to `/Game/RPG/UI/Fonts/`.
