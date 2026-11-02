# Texts of the sections added for the 10-page version (same mini markup as texts.py).
import re

import texts

NNBSP = " "   # CLDR "fr" grouping separator


def fmt_en(v):
    return f"{v:,}".replace("-", "−")


def fmt_fr(v):
    return f"{v:,}".replace(",", NNBSP).replace("-", "−")


EN_MORE = dict(
    fmt=fmt_en,
    months=["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov",
            "Dec"],

    # ---- the Edit scenario window
    s_es="The Edit scenario window",
    es_lead="Most of the work is done here. Open it with [[Scenario → Edit...]].",
    es_title="Edit current scenario*",
    es_name="Name :", es_desc="Description :", es_infl="Annual inflation :",
    es_dur="Duration of calculation :", es_name_val="My budget", es_full="Full View...",
    es_const="Constant", es_var="Variable", es_viewedit="View/Edit...", es_years="year(s)",
    es_group="Cash stream definitions", es_filters="Filters :",
    es_income="Income", es_expense="Expense",
    es_checks=["Periodic", "Irregular", "Enabled", "Disabled", "By tags"], es_set="Set...",
    es_cols=["Type", "Name", "Amount", "Info"],
    es_rows=[("Periodic", "Salary", "3,200.00",
              "Every 1 month in [2026-11-15,Scenario defined] - Growth: None"),
             ("Irregular", "Year-end bonus", "", "1,500.00 on 2026-12-18")],
    es_rows_inc=[False, False],
    es_buttons=["New Periodic", "New Irregular", "Edit...", "Duplicate", "Delete", "Enable",
                "Disable", "Color...", "Select all", "Unselect all"],
    es_tags="Manage tags...", es_close="Close", es_cancel="Cancel", es_apply="Apply",
    es_legend=[
        ("Name", "The name of the scenario, shown above the curve. It is not the file name."),
        ("Annual inflation", "A {{Constant}} rate, or a {{Variable}} one, year by year "
         "({{View/Edit...}}). Each income or expense can then follow it (its "
         "{{Advanced}} tab)."),
        ("Duration of calculation", "How many years ahead GBP computes the events : 25 by "
         "default, from 1 to 100."),
        ("Income or Expense", "The list shows either the incomes or the expenses. This "
         "choice also sets the type of the next item you create."),
        ("Filters", "Show only the periodic or irregular items, the enabled or disabled "
         "ones, or those with some tags ({{By tags}}, {{Set...}})."),
        ("The list", "One line per item. Double-click a line to edit it, or select lines "
         "and use the buttons below : {{Edit...}}, {{Duplicate}}, {{Delete}}, {{Enable}}, "
         "{{Disable}}, {{Color...}}."),
        ("New Periodic, New Irregular", "Create an item of the type chosen in 4. Shortcuts "
         ": CTRL+SHIFT+P and CTRL+SHIFT+I."),
        ("Manage tags...", "Create tags and link them to items (see the Tags section)."),
        ("⚠, Cancel and Apply", "The orange ⚠ (and the * in the title) means that some "
         "changes are not applied yet. {{Apply}} sends them to the curve and keeps the "
         "window open; {{Cancel}} forgets them. When there is nothing to apply, only "
         "{{Close}} is shown. **Cancel and Close do not ask : changes that are not applied are "
         "lost.**")],
    warn_close="**Cancel and Close do not ask :** changes that are not applied are lost. "
               "Look for the ⚠ before leaving the window.",

    # ---- worked example
    s_ex="A worked example",
    ex_lead="A small budget, typed as shown on the previous pages. Today is 2026-10-28 and "
            "your accounts hold **800** in all : that is the start amount.",
    ex_head=("Name", "Type", "Amount", "Rhythm", "Starts on"),
    ex_rows=[("Salary", "Periodic income", "3,200", "Month × 1", "2026-11-15"),
             ("Rent", "Periodic expense", "1,450", "Month × 1", "2026-11-01"),
             ("Groceries", "Periodic expense", "650", "End-of-Month × 1", "2026-11-30"),
             ("Car insurance", "Periodic expense", "1,100", "Year × 1", "2027-01-10"),
             ("Car repair", "Irregular expense", "900", "once", "2027-03-14"),
             ("Vacation", "Irregular expense", "3,000", "once", "2027-07-05")],
    ex_curve="The resulting curve over the next 12 months ({{1Y}}). Click a point, for "
             "example A : {{Selected point info}} shows its date, balance and events.",
    ex_start_lbl="800 today",
    ex_reading_title="What the curve tells you",
    ex_reading=[
        ("A", "**Nov 1 : −650.** The rent is paid before the first salary : you would be "
              "short of 650. Have at least 650 more on Nov 1, or pay the rent later."),
        ("B", "**Jan 10 : 450.** Rent, then car insurance, before the salary : the lowest "
              "point of the year after November, as on Dec 1."),
        ("C", "**Mar 14 : car repair, −900.** The balance stays positive (1,750)."),
        ("D", "**Jul 5 : vacation, −3,000.** Affordable : 3,150 are left."),
        ("E", "**Oct 31, 2027 : 9,000.** Every month adds 1,100 (3,200 − 1,450 − 650).")],
    ex_tip="**Try it :** click point A on the curve. {{Selected point info}} shows the "
           "date, the balance (−650.00), and the event of that day (Rent).",

    # ---- analysis gallery
    ana_tabs=["Relative weight", "Period - chart", "Period - list", "Period - heatmap",
              "Tags", "Compare CSD"],

    # ---- common questions
    s_faq="Common questions",
    faq=[
        ("I changed something, but the curve did not move.",
         "Press {{Apply}}. An orange ⚠ next to the buttons (and a * in the window title) "
         "means that some changes are not applied yet."),
        ("My start amount went back to 0.",
         "That is normal : it is not saved, and goes back to 0 each time a scenario is "
         "created or opened. Type today's cash again."),
        ("The dots of the curve disappeared.",
         "There are too many points in view to show them : an ⓘ appears next to "
         "{{Points}}. Zoom in, or raise {{Max number of points shown}} in "
         "[[Tools → Options...]]."),
        ("An income or expense is missing from the curve.",
         "It may be disabled (check the {{Enabled}} and {{Disabled}} filters of the Edit "
         "scenario window), or start after the end of the calculation. {{View "
         "occurrences...}}, in its window, lists the dates it generates."),
        ("The curve stops too early.",
         "Raise {{Duration of calculation}} in the Edit scenario window (25 years by "
         "default, up to 100). Press {{Max}} to see the whole calculation."),
        ("Can I change the currency ?",
         "No : it is chosen once, when the scenario is created. "
         "[[Scenario → Properties...]] shows it.")],
    fields_end="By default, the end of the calculation (the scenario's {{Duration of "
               "calculation}}). Choose {{Custom}} to give an earlier last date.",
)

FR_MORE = dict(
    fmt=fmt_fr,
    months=["janv.", "févr.", "mars", "avr.", "mai", "juin", "juil.", "août", "sept.", "oct.",
            "nov.", "déc."],

    s_es="La fenêtre d'édition du scénario",
    es_lead="L'essentiel du travail se fait ici. Ouvrez-la avec [[Scénario → Éditer...]].",
    es_title="Éditer le scénario courant*",
    es_name="Nom :", es_desc="Description :", es_infl="Inflation annuelle :",
    es_dur="Durée de calcul :", es_name_val="Mon budget", es_full="Agrandir...",
    es_const="Constante", es_var="Variable", es_viewedit="Éditer/Voir...",
    es_years="année(s)",
    es_group="Définitions de flux monétaire", es_filters="Filtres :",
    es_income="Revenu", es_expense="Dépense",
    es_checks=["Périodique", "Irrégulier", "Activé", "Désactivé", "Étiquettes"],
    es_set="Éditer...",
    es_cols=["Type", "Nom", "Montant", "Info"],
    es_rows=[("Périodique", "Salaire", "3 200,00",
              "Tous les 1 mois dans [2026-11-15,Défini par le scénario] - Croissance: Aucune"),
             ("Irrégulier", "Prime de fin d'année", "", "1 500,00 le 2026-12-18")],
    es_rows_inc=[False, False],
    es_buttons=["+ périodique", "+ irrégulier", "Éditer...", "Dupliquer", "Supprimer",
                "Activer", "Désactiver", "Couleur...", "Tout sélectionner",
                "Tout désélectionner"],
    es_tags="Gérer les étiquettes...", es_close="Fermer", es_cancel="Annuler",
    es_apply="Appliquer",
    es_legend=[
        ("Nom", "Le nom du scénario, affiché au-dessus de la courbe. Ce n'est pas le nom du "
         "fichier."),
        ("Inflation annuelle", "Un taux {{Constante}}, ou {{Variable}}, année par année "
         "({{Éditer/Voir...}}). Chaque revenu ou dépense peut ensuite la suivre (son "
         "onglet {{Avancé}})."),
        ("Durée de calcul", "Combien d'années à l'avance GBP calcule les évènements : 25 "
         "par défaut, de 1 à 100."),
        ("Revenu ou Dépense", "La liste montre soit les revenus, soit les dépenses. Ce "
         "choix fixe aussi le type du prochain élément que vous créez."),
        ("Filtres", "Montrer seulement les éléments périodiques ou irréguliers, activés ou "
         "désactivés, ou ceux qui ont certaines étiquettes ({{Étiquettes}}, "
         "{{Éditer...}})."),
        ("La liste", "Une ligne par élément. Double-cliquez une ligne pour l'éditer, ou "
         "sélectionnez des lignes et utilisez les boutons du dessous : {{Éditer...}}, "
         "{{Dupliquer}}, {{Supprimer}}, {{Activer}}, {{Désactiver}}, {{Couleur...}}."),
        ("+ périodique, + irrégulier", "Créer un élément du type choisi en 4. Raccourcis : "
         "CTRL+MAJ+P et CTRL+MAJ+I."),
        ("Gérer les étiquettes...", "Créer des étiquettes et les lier aux éléments (voir la "
         "section Étiquettes)."),
        ("⚠, Annuler et Appliquer", "Le ⚠ orange (et le * dans le titre) signifie que des "
         "changements ne sont pas encore appliqués. {{Appliquer}} les envoie à la courbe "
         "et garde la fenêtre ouverte; {{Annuler}} les oublie. Quand il n'y a rien à "
         "appliquer, seul {{Fermer}} est affiché. **Annuler et Fermer ne demandent rien : les "
         "changements non appliqués sont perdus.**")],
    warn_close="**Annuler et Fermer ne demandent rien :** les changements non appliqués "
               "sont perdus. Cherchez le ⚠ avant de quitter la fenêtre.",

    s_ex="Un exemple complet",
    ex_lead="Un petit budget, saisi comme expliqué dans les pages précédentes. Nous sommes "
            "le 2026-10-28 et vos comptes contiennent **800** en tout : c'est le montant "
            "de départ.",
    ex_head=("Nom", "Type", "Montant", "Rythme", "Débute le"),
    ex_rows=[("Salaire", "Revenu périodique", "3 200", "Mois × 1", "2026-11-15"),
             ("Loyer", "Dépense périodique", "1 450", "Mois × 1", "2026-11-01"),
             ("Épicerie", "Dépense périodique", "650", "Fin-de-mois × 1", "2026-11-30"),
             ("Assurance auto", "Dépense périodique", "1 100", "Année × 1", "2027-01-10"),
             ("Réparation auto", "Dépense irrégulière", "900", "une fois", "2027-03-14"),
             ("Vacances", "Dépense irrégulière", "3 000", "une fois", "2027-07-05")],
    ex_curve="La courbe obtenue sur les 12 prochains mois ({{1Y}}). Cliquez un point, par "
             "exemple A : {{Point sélectionné}} montre sa date, son solde et ses évènements.",
    ex_start_lbl="800 aujourd'hui",
    ex_reading_title="Ce que la courbe vous dit",
    ex_reading=[
        ("A", "**1er nov. : −650.** Le loyer est payé avant le premier salaire : il vous "
              "manquerait 650. Prévoyez au moins 650 de plus le 1er novembre, ou payez le "
              "loyer plus tard."),
        ("B", "**10 janv. : 450.** Le loyer, puis l'assurance auto, avant le salaire : le "
              "point le plus bas de l'année après novembre, comme le 1er décembre."),
        ("C", "**14 mars : réparation auto, −900.** Le solde reste positif (1 750)."),
        ("D", "**5 juil. : vacances, −3 000.** C'est possible : il reste 3 150."),
        ("E", "**31 oct. 2027 : 9 000.** Chaque mois ajoute 1 100 (3 200 − 1 450 − 650).")],
    ex_tip="**Essayez :** cliquez le point A sur la courbe. {{Point sélectionné}} montre la "
           "date, le solde (−650,00) et l'évènement de ce jour (Loyer).",

    ana_tabs=["Poid relatif", "Période - graphique", "Période - liste",
              "Période - carte thermique", "Étiquettes", "Comparer DFM"],

    s_faq="Questions fréquentes",
    faq=[
        ("J'ai changé quelque chose, mais la courbe n'a pas bougé.",
         "Appuyez sur {{Appliquer}}. Un ⚠ orange à côté des boutons (et un * dans le titre "
         "de la fenêtre) signifie que des changements ne sont pas encore appliqués."),
        ("Mon montant de départ est revenu à 0.",
         "C'est normal : il n'est pas sauvegardé, et revient à 0 chaque fois qu'un scénario "
         "est créé ou ouvert. Saisissez de nouveau l'argent disponible aujourd'hui."),
        ("Les points de la courbe ont disparu.",
         "Il y a trop de points dans la vue pour les afficher : un ⓘ apparaît à côté de "
         "{{Points}}. Zoomez, ou augmentez {{Nombre max. de points affichés}} dans "
         "[[Outils → Options...]]."),
        ("Un revenu ou une dépense manque sur la courbe.",
         "Il est peut-être désactivé (vérifiez les filtres {{Activé}} et {{Désactivé}} de "
         "la fenêtre d'édition du scénario), ou débute après la fin du calcul. {{Voir les "
         "occurrences...}}, dans sa fenêtre, liste les dates qu'il génère."),
        ("La courbe s'arrête trop tôt.",
         "Augmentez {{Durée de calcul}} dans la fenêtre d'édition du scénario (25 ans par "
         "défaut, jusqu'à 100). Appuyez sur {{Max}} pour voir tout le calcul."),
        ("Puis-je changer la monnaie ?",
         "Non : elle est choisie une seule fois, à la création du scénario. "
         "[[Scénario → Propriétés...]] l'affiche.")],
    fields_end="Par défaut, la fin du calcul ({{Durée de calcul}} du scénario). Choisissez "
               "{{Spécifique}} pour donner une dernière date plus tôt.",
)


def _fr_spaces(v):
    """CLDR 'fr' : narrow no-break space between groups of digits ('3 200' -> '3 200')."""
    if isinstance(v, str):
        return re.sub(r"(\d) (\d{3})(?!\d)", "\\1" + NNBSP + "\\2", v)
    if isinstance(v, (list, tuple)):
        return type(v)(_fr_spaces(x) for x in v)
    return v


texts.EN.update(EN_MORE)
texts.FR.update(FR_MORE)
for k in list(texts.FR):
    texts.FR[k] = _fr_spaces(texts.FR[k])
# "Ends no later than" : the end date is the scenario's one unless "Custom" is chosen
for L in (texts.EN, texts.FR):
    L["fields"] = [(a, L["fields_end"]) if i == 3 else (a, b)
                   for i, (a, b) in enumerate(L["fields"])]
