# Quick Tutorial generator

Builds the Quick Tutorial (English and French) : `quick_tutorial-en.odt/.pdf` and
`quick_tutorial-fr.odt/.pdf`, and the Welcome document (`welcome-en.odt/.pdf` and
`welcome-fr.odt/.pdf`), with the same look.

## Files

| File            | Content                                                              |
|-----------------|----------------------------------------------------------------------|
| `texts.py`      | Texts of the original sections, EN and FR                            |
| `texts_more.py` | Texts of the sections added later (Edit scenario window, worked example, Common questions...), number formats |
| `diagrams.py`   | The SVG diagrams (one set per language)                              |
| `example.py`    | Worked example : the events and the daily balance, computed from the dates |
| `build.py`      | Builds `quick_tutorial-<lang>.fodt` (flat ODT) from all of the above |
| `welcome.py`    | Builds `welcome-<lang>.fodt` : its texts, drawings and layout (uses `build.py`) |

Mini markup in the texts : `**bold**`, `[[Menu → Item]]` (menu path), `{{Button}}` (button or
field name). UI labels must be exactly those of the `.ui` files and of `src/gbp_fr.ts`.

## Build

Requires Python 3 and LibreOffice, and the fonts Noto Sans, Noto Sans Display and DejaVu Sans.
Run from an empty work directory (the files are written in the current directory) :

```
python3 <path>/quick_tutorial-generator/build.py
for l in en fr; do
    soffice --headless --convert-to odt quick_tutorial-$l.fodt
    soffice --headless --convert-to pdf quick_tutorial-$l.odt
done
```

If LibreOffice is already open, add `-env:UserInstallation=file://$PWD/lo_profile` to the
`soffice` commands.

Then copy the results :

- `quick_tutorial-<lang>.odt` → `doc/doc-source/`
- `quick_tutorial-<lang>.pdf` → `doc/` and `src/resources/`

For the Welcome document, run `welcome.py` instead of `build.py`, convert `welcome-<lang>.fodt`
the same way, and copy `welcome-<lang>.odt` → `doc/doc-source/` and `welcome-<lang>.pdf` →
`src/resources/`. It must stay on 1 page (it is built into both versions of GBP).

After a change, check that each language still has the expected number of pages
(`pdfinfo quick_tutorial-<lang>.pdf`) : a section that becomes too long moves to the next page.
The sections that start on a new page are set by `PAGE_BREAK_BEFORE` in `build.py`.
