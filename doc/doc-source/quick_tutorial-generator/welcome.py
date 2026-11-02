"""Build welcome-<lang>.fodt (flat ODT), with the same look as the Quick Tutorial."""
from xml.sax.saxutils import escape

import build as B
import diagrams as D

EN = dict(
    lang="en", country="CA",
    product="Graphical Budget Planner 1.9.0",
    file_title="Welcome !",
    tagline="Plan your future cash balance, simply.",
    version="",
    footer="Graphical Budget Planner 1.9.0 · Welcome",
    intro="It looks like this is the first time GBP starts on this computer (no settings were "
          "found). Here is how to get going.",
    s_start="Start here",
    cards=[("Look at an example",
            "[[Scenario → Open example]] loads a complete sample budget. Explore it freely : "
            "your own files are not touched."),
           ("Follow the Quick Tutorial",
            "[[Help → Quick tutorial]] : in about 15 minutes, from an empty window to your "
            "first cash balance forecast."),
           ("Create your own scenario",
            "[[Scenario → New...]], then follow the 6 steps of the Quick Tutorial. Remember "
            "to save : [[Scenario → Save]].")],
    s_help="Getting help",
    tip_text="**Hover over any button or field** : a tooltip explains it. Tooltips can be "
             "turned off in [[Tools → Options...]] ({{Show tooltips}}).",
    tt_field="Start amount as of today 2026-10-28 :",
    tt_amount="2,000.00",
    tt_lines=["Baseline amount from which starts the Cash",
              "Balance curve. As the curve always begins",
              "from **tomorrow**, the baseline amount is the total",
              "cash balance you have, as determined by your",
              "own means, as of **today**."],
    help_lead="The [[Help]] menu gives you :",
    help_rows=[("Quick tutorial", "The essentials, with diagrams and a worked example."),
               ("User manual", "Every window and every option, in full detail."),
               ("Change log", "What is new in each version of GBP.")],
    s_about="About GBP",
    s_open="Free and open source",
    open_text="GBP is free and open-source software. Source code and new versions : "
              "**https://github.com/redmoon1945/gbp**",
    s_feedback="Send feedback",
    feedback="Questions, ideas or problems : open an issue on GitHub "
             "(**https://github.com/redmoon1945/gbp/issues**), or write to "
             "**redmoon1945@protonmail.com**.",
    enjoy="We hope you will enjoy GBP !",
)

FR = dict(
    lang="fr", country="CA",
    product="Graphical Budget Planner 1.9.0",
    file_title="Bienvenue !",
    tagline="Planifiez votre solde futur, simplement.",
    version="",
    footer="Graphical Budget Planner 1.9.0 · Bienvenue",
    intro="Il semble que GBP démarre pour la première fois sur cet ordinateur (aucun réglage "
          "n'a été trouvé). Voici comment commencer.",
    s_start="Pour commencer",
    cards=[("Regarder un exemple",
            "[[Scénario → Ouvrir exemple]] charge un budget complet. Explorez-le : vos "
            "fichiers ne sont pas touchés."),
           ("Suivre le tutoriel rapide",
            "[[Aide → Tutoriel rapide]] : en 15 minutes environ, d'une fenêtre vide à votre "
            "première prévision de solde."),
           ("Créer votre scénario",
            "[[Scénario → Nouveau...]], puis suivez les 6 étapes du tutoriel rapide. Pensez "
            "à sauvegarder : [[Scénario → Sauvegarder]].")],
    s_help="Obtenir de l'aide",
    tip_text="**Survolez un bouton ou un champ** : une infobulle l'explique. Elles se "
             "désactivent dans [[Outils → Options...]] ({{Montrer les infobulles}}).",
    tt_field="Montant de départ au 2026-10-28 :",
    tt_amount="2 000,00",
    tt_lines=["Montant de référence (i.e. de départ) à partir duquel",
              "démarre la courbe des flux de trésorerie. Puisque la",
              "courbe débute toujours à partir de **demain**, le montant",
              "de référence doit être interprété comme la somme totale",
              "des liquidités disponibles (que vous déterminez vous-mêmes",
              "par vos propres moyens) en date d'aujourd'hui."],
    help_lead="Le menu [[Aide]] vous donne :",
    help_rows=[("Tutoriel rapide", "L'essentiel, avec des schémas et un exemple complet."),
               ("Manuel de l'utilisateur", "Chaque fenêtre et chaque option, en détail."),
               ("Journal des changements", "Les nouveautés de chaque version (en "
                "anglais).")],
    s_about="À propos de GBP",
    s_open="Libre et gratuit",
    open_text="GBP est un logiciel libre et gratuit. Code source et nouvelles versions : "
              "**https://github.com/redmoon1945/gbp**",
    s_feedback="Donner votre avis",
    feedback="Questions, idées ou problèmes : ouvrez un « issue » sur GitHub "
             "(**https://github.com/redmoon1945/gbp/issues**), ou écrivez à "
             "**redmoon1945@protonmail.com**.",
    enjoy="Nous espérons que vous apprécierez GBP !",
)


def card_icon(i):
    """Small drawing on top of each 'Start here' card."""
    w, h = 120, 70
    b = [f'<rect x="0" y="0" width="{w}" height="{h}" rx="10" fill="{D.ACCENT_LIGHT}"/>']
    if i == 0:      # a little curve in a window
        b.append(f'<rect x="22" y="12" width="76" height="46" rx="5" fill="#FFFFFF" '
                 f'stroke="{D.ACCENT}" stroke-width="2"/>')
        b.append(f'<polyline points="30,46 42,46 42,34 56,34 56,40 70,40 70,24 90,24" '
                 f'fill="none" stroke="{D.ACCENT}" stroke-width="2.5"/>')
    elif i == 1:    # an open book
        b.append(f'<path d="M60,18 C48,12 32,12 22,16 L22,56 C32,52 48,52 60,58 Z" '
                 f'fill="#FFFFFF" stroke="{D.ACCENT}" stroke-width="2"/>')
        b.append(f'<path d="M60,18 C72,12 88,12 98,16 L98,56 C88,52 72,52 60,58 Z" '
                 f'fill="#FFFFFF" stroke="{D.ACCENT}" stroke-width="2"/>')
        for k in range(3):
            y = 26 + k * 8
            b.append(f'<line x1="30" y1="{y}" x2="52" y2="{y - 1}" stroke="#B9C1CC" '
                     f'stroke-width="2"/>')
            b.append(f'<line x1="68" y1="{y - 1}" x2="90" y2="{y}" stroke="#B9C1CC" '
                     f'stroke-width="2"/>')
    else:           # a new document with a plus
        b.append(f'<path d="M38,10 L72,10 L84,22 L84,60 L38,60 Z" fill="#FFFFFF" '
                 f'stroke="{D.ACCENT}" stroke-width="2"/>')
        b.append(f'<circle cx="84" cy="52" r="13" fill="{D.INCOME}"/>')
        b.append('<line x1="84" y1="45" x2="84" y2="59" stroke="#FFFFFF" stroke-width="3"/>')
        b.append('<line x1="77" y1="52" x2="91" y2="52" stroke="#FFFFFF" stroke-width="3"/>')
    return D._svg(w, h, "".join(b))


def tooltip(L):
    """The start amount field with its tooltip, as it appears when hovered."""
    box_h = 22 + 20 * len(L["tt_lines"])
    fx = 24 + len(L["tt_field"]) * 7.9 + 10
    # tooltip box as wide as its longest line (bold parts are a little wider)
    tw = int(max(len(t.replace("**", "")) for t in L["tt_lines"]) * 7.4 + 30)
    w, h = max(int(fx + 150), 24 + tw + 24), 76 + box_h + 16
    b = [f'<rect x="0" y="0" width="{w}" height="{h}" rx="10" fill="#F6F7F9" '
         f'stroke="#D5DBE3"/>']
    b.append(D._t(24, 44, L["tt_field"], 15, D.INK))
    b.append(f'<rect x="{fx}" y="24" width="130" height="30" rx="4" fill="#FFFFFF" '
             f'stroke="{D.INCOME}" stroke-width="2"/>')
    b.append(D._t(fx + 10, 45, L["tt_amount"], 14.5, D.INK))
    # mouse pointer on the field
    px, py = fx + 92, 40
    b.append(f'<path d="M{px},{py} l0,24 l6,-6 l5,11 l4,-2 l-5,-11 l8,0 Z" fill="#FFFFFF" '
             f'stroke="#1F2937" stroke-width="1.5"/>')
    # tooltip box
    tx, ty = 24, 76
    b.append(f'<rect x="{tx + 3}" y="{ty + 3}" width="{tw}" height="{box_h}" rx="4" '
             f'fill="#000000" opacity="0.12"/>')
    b.append(f'<rect x="{tx}" y="{ty}" width="{tw}" height="{box_h}" rx="4" fill="#FFFFE1" '
             f'stroke="#9AA4B2"/>')
    for k, line in enumerate(L["tt_lines"]):
        # **bold** parts
        parts = line.split("**")
        x = tx + 12
        spans = "".join(
            f'<tspan font-weight="{"bold" if j % 2 else "normal"}">{escape(s)}</tspan>'
            for j, s in enumerate(parts))
        b.append(f'<text x="{x}" y="{ty + 24 + k * 20}" font-size="14" fill="{D.INK}">'
                 f'{spans}</text>')
    return D._svg(w, h, "".join(b))


def heading(name, title):
    """Section heading : a colored title, lighter than the Quick Tutorial's sections."""
    return (f'<text:h text:style-name="H2W" text:outline-level="1">{escape(title)}</text:h>',
            "")


def build_welcome(L):
    body, colstyles = [], []

    def add(res):
        xml, cs = res
        body.append(xml)
        colstyles.append(cs)

    add(B.table("Cover", [B.CONTENT_W], [[("CoverCellW",
        B.p("CoverProduct", L["product"]) + B.p("CoverTitle", L["file_title"])
        + B.p("CoverTag", L["tagline"]))]]))
    body.append(B.p("Body", f"{L['intro']} **{L['enjoy']}**"))

    add(heading("HStart", L["s_start"]))
    w3 = B.CONTENT_W / 3
    cells = []
    for i, (t, s) in enumerate(L["cards"]):
        icon = B.image(card_icon(i), 1.6, f"icon{i}").replace('text:style-name="Fig"',
                                                             'text:style-name="FigCell"')
        cells.append(("KnowCell", icon + B.p("KnowTitle", f"{i + 1}. {t}")
                      + B.p("KnowBody", s)))
    add(B.table("Cards", [w3] * 3, [cells], "TblGap"))

    add(heading("HHelp", L["s_help"]))
    # left : the tooltips, right : the Help menu
    left = B.p("KnowBody", L["tip_text"]) + B.image(tooltip(L), 8.0, "tooltip").replace(
        'text:style-name="Fig"', 'text:style-name="FigCell"')
    right = B.p("KnowTitle", L["help_lead"]) + "".join(
        B.p("KnowBody", f"**{a}** — {b2}") for a, b2 in L["help_rows"])
    add(B.table("Help", [10.3, B.CONTENT_W - 10.3],
                [[("KnowCell", left), ("KnowCell", right)]], "TblGap"))

    # one compact box : open source + feedback
    add(B.table("About", [B.CONTENT_W], [[("CallInfo",
        B.p("KnowBody", f"**{L['s_open']}** — {L['open_text']}")
        + B.p("KnowBody", f"**{L['s_feedback']}** — {L['feedback']}"))]]))
    # LibreOffice adds an empty paragraph after a final table : make it tiny, so that it does
    # not create a second page
    body.append('<text:p text:style-name="Tiny"/>')
    return B.doc(L, "".join(body), "".join(colstyles))


if __name__ == "__main__":
    for L in (EN, FR):
        with open(f"welcome-{L['lang']}.fodt", "w", encoding="utf-8") as f:
            f.write(build_welcome(L))
    print("ok")
