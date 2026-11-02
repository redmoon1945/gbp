"""Build quick_tutorial-<lang>.fodt (flat ODT) from texts.py + diagrams.py."""
import base64
import re
import sys
from xml.sax.saxutils import escape

import diagrams as D
import example as EX
import texts
import texts_more  # noqa: F401  (adds the texts of the new sections)

ACC, ACC_L, INC, EXP, INK, MUT = "#1F5E99", "#E8F0F9", "#23864F", "#C0392B", "#2B2F36", \
    "#6B7280"
CONTENT_W = 17.99  # cm, Letter 21.59 - 2 x 1.8


def inline(s):
    """Mini markup -> ODF spans."""
    out = []
    for tok in re.split(r"(\*\*.+?\*\*|\[\[.+?\]\]|\{\{.+?\}\})", s):
        if not tok:
            continue
        if tok.startswith("**"):
            out.append(f'<text:span text:style-name="B">{escape(tok[2:-2])}</text:span>')
        elif tok.startswith("[["):
            out.append(f'<text:span text:style-name="Menu">{escape(tok[2:-2])}</text:span>')
        elif tok in ("{{◀}}", "{{▶}}"):
            out.append(f'<text:span text:style-name="BtnSym">\u00a0{tok[2:-2]}\u00a0'
                       f'</text:span>')
        elif tok.startswith("{{"):
            out.append(f'<text:span text:style-name="Btn"> {escape(tok[2:-2])}'
                       f' </text:span>')
        else:
            out.append(escape(tok))
    return "".join(out)


def p(style, s):
    return f'<text:p text:style-name="{style}">{inline(s)}</text:p>'


def image(svg, width_cm, name):
    m = re.search(r'width="(\d+)" height="(\d+)"', svg)
    h_cm = width_cm * int(m.group(2)) / int(m.group(1))
    data = base64.b64encode(svg.encode()).decode()
    return (f'<text:p text:style-name="Fig"><draw:frame draw:style-name="Img" draw:name="{name}"'
            f' text:anchor-type="as-char" svg:width="{width_cm:.2f}cm" '
            f'svg:height="{h_cm:.2f}cm" draw:z-index="0"><draw:image '
            f'draw:mime-type="image/svg+xml"><office:binary-data>{data}</office:binary-data>'
            f'</draw:image></draw:frame></text:p>')


def table(name, cols, rows, style="Tbl"):
    """cols: list of (width_cm); rows: list of list of (cell_style, xml)."""
    x = [f'<table:table table:name="{name}" table:style-name="{style}">']
    for i, w in enumerate(cols):
        x.append(f'<table:table-column table:style-name="{name}.C{i}"/>')
    for r in rows:
        x.append('<table:table-row table:style-name="Row">')
        for cs, content in r:
            x.append(f'<table:table-cell table:style-name="{cs}" office:value-type="string">'
                     f'{content}</table:table-cell>')
        x.append("</table:table-row>")
    x.append("</table:table>")
    col_styles = "".join(
        f'<style:style style:name="{name}.C{i}" style:family="table-column">'
        f'<style:table-column-properties style:column-width="{w}cm"/></style:style>'
        for i, w in enumerate(cols))
    return "".join(x), col_styles


PAGE_BREAK_BEFORE = {2, 3, 4, 5, 6, 7, 9, 10}


def section(n, title):
    """Section heading : number badge + title, as a 2-cell table."""
    brk = "SecTbl" if n in PAGE_BREAK_BEFORE else "SecTblFirst"
    xml, cs = table(f"Sec{n}", [1.3, CONTENT_W - 1.3],
                    [[("SecNum", f'<text:p text:style-name="SecNumP">{n}</text:p>'),
                      ("SecTitle", f'<text:h text:style-name="H1" text:outline-level="1">'
                                   f'{escape(title)}</text:h>')]], brk)
    return xml, cs


def callout(name, kind, s):
    xml, cs = table(name, [CONTENT_W], [[(f"Call{kind}", p("CallP", s))]])
    return xml, cs


def build(L):
    body, colstyles = [], []

    def add(res):
        xml, cs = res
        body.append(xml)
        colstyles.append(cs)

    # ---- cover band
    add(table("Cover", [CONTENT_W], [[("CoverCell",
        p("CoverProduct", L["product"]) + p("CoverTitle", L["file_title"])
        + p("CoverTag", L["tagline"]) + p("CoverVersion", L["version"]))]]))
    body.append(p("Lead", L["intro"]))

    def legend(name, items, start=1):
        rows = []
        for i, (t, s) in enumerate(items, start):
            rows.append([("LegNum", f'<text:p text:style-name="LegNumP">{i}</text:p>'),
                         ("LegBody", p("TdP", f"**{t}** — {s}"))])
        add(table(name, [1.0, CONTENT_W - 1.0], rows))

    def grid(name, head, data, widths, small=False):
        th, tb, td = ("TdSBold", "TdS", ("TdSA", "TdSB")) if small else \
            ("TdBold", "TdP", ("TdA", "TdB"))
        rows = [[("Th", p("ThP", x)) for x in head]] if head else []
        for k, r in enumerate(data):
            z = td[k % 2]
            rows.append([(z, p(th if i == 0 else tb, x)) for i, x in enumerate(r)])
        add(table(name, widths, rows))

    def mini_steps(name, items, marker=None):
        num = "MiniNum" if marker is None else "Check"
        rows = [[(num, f'<text:p text:style-name="{"MiniNumP" if marker is None else "CheckP"}">'
                       f'{i if marker is None else marker}</text:p>'),
                 ("MiniBody", p("TdP", s))] for i, s in enumerate(items, 1)]
        add(table(name, [0.9, CONTENT_W - 0.9], rows))

    n = 0

    def sec(key):
        nonlocal n
        n += 1
        add(section(n, L[key]))

    # ---- How GBP works
    sec("s1")
    body.append(p("Lead", L["s1_lead"]))
    body.append(image(D.concept(L), 15.5, "concept"))
    body.append(p("Body", L["s1_csd"]))
    body.append(p("H2", L["know_title"]))
    w3 = CONTENT_W / 3
    add(table("Know", [w3] * 3, [[("KnowCell", p("KnowTitle", t) + p("KnowBody", s))
                                  for t, s in L["know"]]], "TblGap"))

    # ---- First scenario
    sec("s2")
    add(callout("TipEx", "Tip", L["tip_example"]))
    rows = []
    for i, (t, s) in enumerate(L["steps"], 1):
        rows.append([("StepNum", f'<text:p text:style-name="StepNumP">{i}</text:p>'),
                     ("StepBody", p("StepTitle", t) + p("StepText", s))])
    add(table("Steps", [1.4, CONTENT_W - 1.4], rows))
    add(callout("WarnSave", "Warn", L["warn_save"]))

    # ---- Edit scenario window
    sec("s_es")
    body.append(p("Body", L["es_lead"]))
    body.append(image(D.edit_scenario(L), 14.6, "editscenario"))
    legend("EsLegend", L["es_legend"])

    # ---- Periodic or irregular
    sec("s3")
    body.append(image(D.kinds(L), CONTENT_W, "kinds"))
    grid("Kinds", L["kinds_head"], L["kinds_rows"], [3.2, 6.8, CONTENT_W - 10.0])
    body.append(p("H2", L["fields_title"]))
    grid("Fields", None, L["fields"], [4.6, CONTENT_W - 4.6])
    add(callout("TipCheck", "Tip", L["tip_check"]))

    # ---- Worked example
    sec("s_ex")
    body.append(p("Body", L["ex_lead"]))
    grid("Ex", L["ex_head"], L["ex_rows"], [3.4, 3.9, 2.2, 3.9, CONTENT_W - 13.4], True)
    body.append(p("Body", L["ex_curve"]))
    body.append(image(D.example_curve(L, EX.balances()), 16.4, "example"))
    body.append(p("H2", L["ex_reading_title"]))
    rows = [[("ExMarkOk" if m == "E" else "ExMark", f'<text:p text:style-name="LegNumP">{m}</text:p>'),
             ("LegBody", p("TdP", s))] for m, s in L["ex_reading"]]
    add(table("ExRead", [1.0, CONTENT_W - 1.0], rows))

    # ---- Main window
    sec("s4")
    body.append(image(D.main_window(L), 16.2, "mainwindow"))
    legend("Legend", L["legend"])

    # ---- Moving around
    sec("s5")
    grid("Mouse", L["mouse_head"], L["mouse"], [6.0, CONTENT_W - 6.0])
    add(callout("TipLost", "Tip", L["tip_lost"]))

    # ---- Tags
    sec("s6")
    body.append(p("Body", L["tags_lead"]))
    mini_steps("TagSteps", L["tags_steps"])

    # ---- Analysis
    sec("s7")
    body.append(p("Body", L["ana_lead"]))
    mini_steps("AnaSteps", L["ana_steps"])
    h = L["ana_head"]
    rows = [[("Th", p("ThP", x)) for x in h]]
    for k, (a_, b_, c_) in enumerate(L["ana"]):
        z = ("TdSA", "TdSB")[k % 2]
        thumb = image(D.analysis_thumb(L, k), 2.5, f"ana{k}").replace(
            'text:style-name="Fig"', 'text:style-name="FigCell"')
        rows.append([(z, thumb + p("TdSBold", a_)), (z, p("TdS", b_)), (z, p("TdS", c_))])
    add(table("Ana", [3.8, 6.6, CONTENT_W - 10.4], rows))

    # ---- Common questions
    sec("s_faq")
    rows = []
    for k, (q, a_) in enumerate(L["faq"]):
        rows.append([("FaqQ", p("FaqQP", "?")), ("FaqBody", p("TdBold", q) + p("TdP", a_))])
    add(table("Faq", [0.9, CONTENT_W - 0.9], rows))

    # ---- Good habits
    sec("s8")
    mini_steps("Habits", L["habits"], "✓")
    add(callout("More", "Info", f"**{L['more_title']}** — {L['more']}"))

    return doc(L, "".join(body), "".join(colstyles))


def cell(name, bg=None, pad="0.18cm", border="none", extra=""):
    bgp = f' fo:background-color="{bg}"' if bg else ""
    return (f'<style:style style:name="{name}" style:family="table-cell">'
            f'<style:table-cell-properties{bgp} fo:padding="{pad}" fo:border="{border}" '
            f'{extra if "vertical-align" in extra else "style:vertical-align=\"top\" " + extra}'
            f'/></style:style>')


def pstyle(name, size="10pt", color=INK, bold=False, align="start", above="0cm",
           below="0.15cm", font="Noto Sans", italic=False, extra_p="", extra_t="",
           parent="Standard", line="125%"):
    w = ' fo:font-weight="bold" style:font-weight-complex="bold"' if bold else ""
    it = ' fo:font-style="italic"' if italic else ""
    return (f'<style:style style:name="{name}" style:family="paragraph" '
            f'style:parent-style-name="{parent}">'
            f'<style:paragraph-properties fo:text-align="{align}" fo:margin-top="{above}" '
            f'fo:margin-bottom="{below}" fo:line-height="{line}" {extra_p}/>'
            f'<style:text-properties style:font-name="{font}" fo:font-size="{size}" '
            f'fo:color="{color}"{w}{it} {extra_t}/></style:style>')


def doc(L, body, colstyles):
    lang = f'fo:language="{L["lang"]}" fo:country="{L["country"]}"'
    styles = "".join([
        pstyle("Standard", extra_t=lang),
        pstyle("Body", below="0.25cm"),
        pstyle("Lead", "11pt", below="0.3cm"),
        pstyle("Version", "8.5pt", MUT, align="end", above="0.1cm", below="0.35cm"),
        pstyle("H2", "12pt", ACC, True, above="0.35cm", below="0.2cm", font="Noto Sans Display"),
        pstyle("H2W", "15pt", ACC, True, above="0.25cm", below="0.1cm", font="Noto Sans Display",
               extra_p='fo:border-bottom="1.5pt solid #1F5E99" fo:padding-bottom="0.08cm"'),
        pstyle("Fig", align="center", above="0.15cm", below="0.25cm", line="100%"),
        pstyle("FigCell", align="start", above="0.05cm", below="0.1cm", line="100%"),
        pstyle("Tiny", "1pt", below="0cm", line="100%"),
        pstyle("CoverProduct", "11pt", "#CFE0F2", True, below="0.05cm"),
        pstyle("CoverTitle", "30pt", "#FFFFFF", True, below="0.1cm",
               font="Noto Sans Display", line="110%"),
        pstyle("CoverTag", "12.5pt", "#FFFFFF", below="0cm"),
        pstyle("CoverVersion", "8.5pt", "#CFE0F2", align="end", above="0.1cm", below="0cm"),
        pstyle("SecNumP", "18pt", "#FFFFFF", True, "center", below="0cm",
               font="Noto Sans Display", line="100%"),
        pstyle("KnowTitle", "10.5pt", ACC, True, below="0.1cm"),
        pstyle("KnowBody", "9.5pt", below="0cm"),
        pstyle("CallP", "10pt", below="0cm"),
        pstyle("StepNumP", "17pt", "#FFFFFF", True, "center", below="0cm",
               font="Noto Sans Display", line="100%"),
        pstyle("StepTitle", "11pt", ACC, True, below="0.06cm"),
        pstyle("StepText", "10pt", below="0cm"),
        pstyle("ThP", "9.5pt", "#FFFFFF", True, below="0cm"),
        pstyle("TdP", "9.5pt", below="0cm"),
        pstyle("TdBold", "9.5pt", INK, True, below="0cm"),
        pstyle("TdMuted", "9.5pt", MUT, below="0cm"),
        pstyle("TdS", "9pt", below="0cm", line="120%"),
        pstyle("TdSBold", "9pt", INK, True, below="0cm", line="120%"),
        pstyle("LegNumP", "10pt", "#FFFFFF", True, "center", below="0cm", line="100%"),
        pstyle("MiniNumP", "10pt", ACC, True, "center", below="0cm", line="100%"),
        pstyle("CheckP", "12pt", INC, True, "center", below="0cm", line="100%"),
        pstyle("FaqQP", "14pt", ACC, True, "center", below="0cm", line="100%",
               font="Noto Sans Display"),
        f'<style:style style:name="Footer" style:family="paragraph" style:parent-style-name='
        f'"Standard"><style:paragraph-properties fo:border-top="0.5pt solid #C9D1DC" '
        f'fo:padding-top="0.12cm"><style:tab-stops><style:tab-stop style:position='
        f'"{CONTENT_W}cm" style:type="right"/></style:tab-stops></style:paragraph-properties>'
        f'<style:text-properties style:font-name="Noto Sans" fo:font-size="8pt" '
        f'fo:color="{MUT}"/></style:style>',
        f'<style:style style:name="H1" style:family="paragraph" style:parent-style-name='
        f'"Standard" style:default-outline-level="1"><style:paragraph-properties '
        f'fo:margin-top="0cm" fo:margin-bottom="0cm" fo:line-height="100%"/>'
        f'<style:text-properties style:font-name="Noto Sans Display" fo:font-size="17pt" '
        f'fo:color="{ACC}" fo:font-weight="bold"/></style:style>',
        '<style:style style:name="B" style:family="text"><style:text-properties '
        'fo:font-weight="bold"/></style:style>',
        f'<style:style style:name="Menu" style:family="text"><style:text-properties '
        f'fo:font-weight="bold" fo:color="{ACC}"/></style:style>',
        '<style:style style:name="Btn" style:family="text"><style:text-properties '
        'style:font-name="Noto Sans" fo:font-size="8.8pt" fo:font-weight="bold" '
        'fo:color="#1F2937" fo:background-color="#E6EAF0"/></style:style>',
        '<style:style style:name="BtnSym" style:family="text"><style:text-properties '
        'style:font-name="DejaVu Sans" fo:font-size="8pt" fo:color="#1F2937" '
        'fo:background-color="#E6EAF0"/></style:style>',
    ])

    def tstyle(name, brk="auto", above="0.25cm", below="0.3cm"):
        return (f'<style:style style:name="{name}" style:family="table">'
                f'<style:table-properties style:width="{CONTENT_W}cm" table:align="left" '
                f'fo:margin-top="{above}" fo:margin-bottom="{below}" fo:break-before="{brk}" '
                f'style:may-break-between-rows="true"/></style:style>')

    line = "0.5pt solid #DDE3EA"
    auto = "".join([
        tstyle("Tbl"), tstyle("TblGap"),
        '<style:style style:name="Row" style:family="table-row"><style:table-row-properties '
        'fo:keep-together="always"/></style:style>',
        tstyle("SecTbl", "page", "0cm", "0.35cm"),
        tstyle("SecTblFirst", "auto", "0.3cm", "0.35cm"),
        cell("CoverCell", ACC, "0.5cm 0.6cm"),
        cell("CoverCellW", ACC, "0.35cm 0.6cm"),
        cell("SecNum", ACC, "0.12cm", extra='style:vertical-align="middle"'),
        cell("SecTitle", ACC_L, "0.12cm 0.3cm", extra='style:vertical-align="middle"'),
        cell("KnowCell", "#F4F7FB", "0.3cm", "0.5pt solid #FFFFFF",
             'fo:border-top="0.09cm solid #1F5E99"'),
        cell("CallTip", "#E9F6EF", "0.25cm 0.35cm", "none",
             f'fo:border-left="0.12cm solid {INC}"'),
        cell("CallWarn", "#FDF1E7", "0.25cm 0.35cm", "none",
             'fo:border-left="0.12cm solid #D9822B"'),
        cell("CallInfo", ACC_L, "0.25cm 0.35cm", "none",
             f'fo:border-left="0.12cm solid {ACC}"'),
        cell("StepNum", ACC, "0.2cm", "0.5pt solid #FFFFFF", 'style:vertical-align="middle"'),
        cell("StepBody", "#F7F9FC", "0.2cm 0.35cm", "0.5pt solid #FFFFFF"),
        cell("Th", ACC, "0.14cm 0.2cm", f"0.5pt solid {ACC}"),
        cell("TdA", "#FFFFFF", "0.13cm 0.2cm", line),
        cell("TdB", "#F4F7FB", "0.13cm 0.2cm", line),
        cell("TdSA", "#FFFFFF", "0.08cm 0.18cm", line),
        cell("TdSB", "#F4F7FB", "0.08cm 0.18cm", line),
        cell("LegNum", ACC, "0.1cm", "0.5pt solid #FFFFFF", 'style:vertical-align="middle"'),
        cell("ExMarkOk", INC, "0.1cm", "0.5pt solid #FFFFFF", 'style:vertical-align="middle"'),
        cell("ExMark", EXP, "0.1cm", "0.5pt solid #FFFFFF", 'style:vertical-align="middle"'),
        cell("FaqQ", ACC_L, "0.15cm", "none", 'fo:border-bottom="0.5pt solid #DDE3EA"'),
        cell("FaqBody", None, "0.15cm 0.25cm", "none",
             'fo:border-bottom="0.5pt solid #DDE3EA"'),
        cell("LegBody", None, "0.1cm 0.25cm", "none",
             'fo:border-bottom="0.5pt solid #DDE3EA"'),
        cell("MiniNum", ACC_L, "0.1cm", "0.5pt solid #FFFFFF"),
        cell("MiniBody", None, "0.1cm 0.25cm", "none",
             'fo:border-bottom="0.5pt solid #DDE3EA"'),
        cell("Check", None, "0.08cm", "none", 'fo:border-bottom="0.5pt solid #DDE3EA"'),
        '<style:style style:name="Img" style:family="graphic"><style:graphic-properties '
        'style:vertical-pos="top" style:vertical-rel="baseline" fo:border="none" '
        'fo:padding="0cm"/></style:style>',
        '<style:page-layout style:name="pm1"><style:page-layout-properties '
        'fo:page-width="21.59cm" fo:page-height="27.94cm" fo:margin-top="1.6cm" '
        'fo:margin-bottom="1.2cm" fo:margin-left="1.8cm" fo:margin-right="1.8cm" '
        'style:print-orientation="portrait"/><style:footer-style>'
        '<style:header-footer-properties fo:min-height="0.8cm" fo:margin-top="0.4cm"/>'
        '</style:footer-style></style:page-layout>',
        colstyles,
    ])
    footer = (f'<text:p text:style-name="Footer">{escape(L["footer"])}<text:tab/>'
              f'<text:page-number text:select-page="current">1</text:page-number></text:p>')
    ns = ('xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0" '
          'xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0" '
          'xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0" '
          'xmlns:table="urn:oasis:names:tc:opendocument:xmlns:table:1.0" '
          'xmlns:draw="urn:oasis:names:tc:opendocument:xmlns:drawing:1.0" '
          'xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0" '
          'xmlns:xlink="http://www.w3.org/1999/xlink" '
          'xmlns:dc="http://purl.org/dc/elements/1.1/" '
          'xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0" '
          'xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0"')
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<office:document {ns} office:version="1.3" office:mimetype="application/vnd.oasis.opendocument.text">
<office:meta><dc:title>{escape(L["product"] + " - " + L["file_title"])}</dc:title><dc:language>{L["lang"]}-{L["country"]}</dc:language></office:meta>
<office:font-face-decls>
<style:font-face style:name="Noto Sans" svg:font-family="'Noto Sans'" style:font-family-generic="swiss"/>
<style:font-face style:name="Noto Sans Display" svg:font-family="'Noto Sans Display'" style:font-family-generic="swiss"/>
<style:font-face style:name="DejaVu Sans" svg:font-family="'DejaVu Sans'" style:font-family-generic="swiss"/>
</office:font-face-decls>
<office:styles>{styles}</office:styles>
<office:automatic-styles>{auto}</office:automatic-styles>
<office:master-styles><style:master-page style:name="Standard" style:page-layout-name="pm1"><style:footer>{footer}</style:footer></style:master-page></office:master-styles>
<office:body><office:text>{body}</office:text></office:body>
</office:document>'''


if __name__ == "__main__":
    for L in (texts.EN, texts.FR):
        with open(f"quick_tutorial-{L['lang']}.fodt", "w", encoding="utf-8") as f:
            f.write(build(L))
    print("ok")
