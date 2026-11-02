# Texts of the Quick Tutorial. Mini markup in paragraphs:
#   **bold**   [[Menu → Item]] (menu path)   {{Button}} (button / field name)

EN = dict(
    lang="en", country="CA",
    file_title="Quick Tutorial",
    product="Graphical Budget Planner 1.9.0",
    tagline="See where your money is heading, in about 15 minutes.",
    version="Version : 2026-10-28",
    footer="Graphical Budget Planner 1.9.0 · Quick Tutorial",
    intro="This tutorial takes you from an empty window to your first cash balance "
          "forecast. Read it in order the first time; afterwards, the last pages work as a "
          "reference card.",

    s1="How GBP works",
    s1_lead="GBP answers one question : **if my incomes and expenses go as planned, how much "
            "money will I have on any day in the future ?**",
    c_titles=["1. You describe", "2. GBP generates", "3. You see"],
    c_captions=["Incomes & expenses (CSD)", "Financial events", "Cash Balance curve"],
    c_chips=[(("Salary", "every month"), True), (("Rent", "every month"), False),
             (("Car repair", "once, in March"), False)],
    c_events=[("Jan 01", "Salary", "+3,200", True), ("Jan 01", "Rent", "−1,450", False),
              ("Feb 01", "Salary", "+3,200", True), ("Feb 01", "Rent", "−1,450", False),
              ("Mar 14", "Car repair", "−900", False)],
    c_start="start amount",
    c_time="time →",
    s1_csd="Each income or expense you describe is called a **Cash Stream Definition**, or "
           "**CSD**. From the CSDs, GBP computes every future **financial event** (a date and "
           "an amount), and adds them up day after day to draw the **Cash Balance curve**.",
    know_title="Good to know",
    know=[("Future only",
           "GBP projects from tomorrow on. It does not record past transactions : it is a "
           "planner, not a bookkeeping tool."),
          ("One scenario at a time",
           "A **scenario** file holds all your CSDs. Keep as many files as you like "
           "(\"Today\", \"If I retire at 60\"...), but only one is open at a time."),
          ("Start amount",
           "The curve starts from the cash you have **today**, typed at the bottom of the "
           "main window. It is **not** saved in the scenario file.")],
    tip_example="**Want to look around first ?** [[Scenario → Open example]] loads a complete "
                "sample budget. Nothing you do there can harm your own files.",

    s2="Your first scenario in 6 steps",
    steps=[
        ("Create the scenario",
         "[[Scenario → New...]], choose the currency, then press {{Select}}. "
         "The currency is permanent : it cannot be changed later."),
        ("Enter today's cash",
         "Right away, at the bottom of the main window, type the total cash you have today "
         "(all accounts together, found by your own means). The curve starts from it. "
         "Do this each time you create or open a scenario."),
        ("Name it",
         "[[Scenario → Edit...]] opens the Edit scenario window. Type a {{Name}} (it is "
         "not the file name). Optional : {{Annual inflation}} and {{Duration of "
         "calculation}} (how many years ahead, 25 by default)."),
        ("Add your incomes and expenses",
         "In the list, first choose {{Income}} or {{Expense}}, then press {{New Periodic}} "
         "for anything that repeats, or {{New Irregular}} for one-off or uneven amounts. "
         "Fill in the window that opens and press {{Apply}}. Repeat for each item."),
        ("Apply",
         "Back in the Edit scenario window, press {{Apply}}. The curve in the main window "
         "is updated immediately."),
        ("Save",
         "[[Scenario → Save]]. {{Apply}} only updates what is in memory : without "
         "saving, all changes are lost when you quit GBP."),
    ],
    warn_save="**Apply is not Save.** {{Apply}} shows the result on the curve; only "
              "[[Scenario → Save]] writes it to the file.",

    s3="Periodic or irregular ?",
    k_periodic="Periodic", k_periodic_sub="same amount, regular rhythm",
    k_irregular="Irregular", k_irregular_sub="dates and amounts you list",
    k_starts="Starts on", k_ends="Ends no later than", k_every="every Month × 1",
    k_list="List of financial events",
    kinds_head=("Type", "Use it for", "Examples"),
    kinds_rows=[("Periodic", "Amounts that repeat at a regular rhythm",
                 "Salary, rent, mortgage, insurance, subscriptions"),
                ("Irregular", "One-time or uneven amounts, on dates you choose",
                 "Bonus, gift, car repair, renovation, tax refund")],
    fields_title="The fields of a periodic item that matter most",
    fields=[("Amount", "The amount of each occurrence."),
            ("Period + Multiplier", "The rhythm : Day, Week, Month, End-of-Month or Year, "
             "times a multiplier. Month × 3 = every quarter, Week × 2 = every two weeks."),
            ("Starts on", "The date of the first occurrence."),
            ("Ends no later than", "Optional last date. Leave it empty to continue until "
             "the end of the calculation."),
            ("Advanced tab", "Optional growth of the amount : {{No growth}}, "
             "{{Follow scenario's inflation}}, or your own constant or variable rate.")],
    tip_check="**Check what you entered :** {{View occurrences...}}, in the item window, "
              "lists every date and amount GBP will generate for that item.",

    s4="The main window at a glance",
    mw_title="My budget.json — GBP",
    mw_menus=["Scenario", "Tools", "Help"],
    mw_chart_title="My budget",
    mw_p_point="Selected point info", mw_p_stats="Statistics", mw_p_range="Date range",
    mw_events=[("3,200.00 : Salary", "#23864F"),
               ("−1,450.00 : Rent", "#C0392B")],
    mw_point_lines=["Date : 2027-03-01", "Amount : 12,994.03", "Change : 1,750.00"],
    mw_stats_lines=[("Eventful days :", "612"), ("Total events :", "1,436"),
                    ("Points in view :", "20"), ("Points displayed :", "20")],
    mw_range_lines=[("From :", "2026-Oct-29"), ("To :", "2027-Oct-29"),
                    ("Delta :", "1y 0m 0d")],
    mw_start="Start amount as of today 2026-10-28 :",
    mw_amount="2,000.00",
    points="Points", guides="Guidelines", export="Export...",
    fit="Fit", eoy="EOY", custom="Custom",
    legend=[("Menus", "[[Scenario]] for files and editing, [[Tools]] for Analysis and "
             "Options, [[Help]] for this tutorial and the User Manual."),
            ("Time range buttons", "Show the next 1 month ... 25 years. {{Max}} : the whole "
             "calculation. {{Fit}} : the date range of all events. {{EOY}} : until the end "
             "of this year. {{Custom}} : dates you choose. {{◀}} {{▶}} : move one window "
             "left or right."),
            ("The Cash Balance curve", "Each point is the balance at the end of a day with "
             "at least one event. The flat line is zero : below it, you are short of cash."),
            ("Click a point", "It is highlighted and becomes the selected point."),
            ("Selected point info", "Date, balance, change for that day, and the list of "
             "all the events of that day."),
            ("Statistics", "{{Eventful days}} : how many days have at least one event. "
             "{{Total events}} : how many events in total. {{Points in view}} : points in "
             "the visible part of the chart, in green when they are shown, in red when there "
             "are too many to show them (zoom in; the limit is set in Options). {{Points "
             "displayed}} : how many are actually shown."),
            ("Date range", "First ({{From}}) and last ({{To}}) day in view, and the span "
             "between them ({{Delta}}, in years, months and days)."),
            ("Start amount", "Your cash as of today. Change it at any time : the curve "
             "moves up or down."),
            ("Display options", "{{Points}} shows or hides the dots (an ⓘ appears next "
             "to it when there are too many to show), {{Guidelines}} the grid. {{Export...}} saves all the curve's points to a CSV file.")],

    s5="Moving around the curve",
    mouse_head=("Do this", "To"),
    mouse=[("Mouse wheel", "Zoom in or out, around the mouse pointer"),
           ("SHIFT + mouse wheel", "Zoom on time only (horizontal)"),
           ("CTRL + mouse wheel", "Zoom on amounts only (vertical)"),
           ("Left-click and drag", "Move the curve in any direction"),
           ("Click a point", "See the details and the events of that day"),
           ("{{◀}} or {{▶}}", "Jump one full window back or forward in time"),
           ("A time range button", "Reset the view (1M ... 25Y, Max, Fit, EOY, Custom)")],
    tip_lost="**Lost in the chart ?** Press {{Fit}} to see everything again.",

    s6="Tags",
    tags_lead="**Tags** are labels you put on CSDs (\"Housing\", \"Car\", \"Leisure\"...) to "
              "group them. A CSD can have several tags.",
    tags_steps=["In the Edit scenario window, press {{Manage tags...}}.",
                "Create your tags and choose the CSDs linked to each one. You can also link "
                "tags from an item window, in {{Linked tags}}.",
                "Use them to filter the list ({{By tags}}) and in the {{Tags}} tab of the "
                "Analysis."],
    s7="Analysis",
    ana_lead="[[Tools → Analysis...]] turns the financial events of a period into charts and "
             "tables, to understand where the money comes from and where it goes.",
    ana_steps=["Open [[Tools → Analysis...]] (a scenario must be loaded).",
               "Choose the period with {{From}} and {{To}} (by default : tomorrow to 5 years "
               "later), then press {{Apply dates}}. Every tab uses this period : change it "
               "to compare, for example, this year with your first year of retirement.",
               "Pick a tab. {{Export chart...}} saves the chart as a PNG image, {{Export "
               "legend...}} saves the table or legend as a CSV file. The {{Help}} tab "
               "describes every tab."],
    ana_head=("Tab", "What you see", "Try this"),
    ana=[("Relative weight",
          "A pie chart of the incomes (or expenses) that weigh the most over the period. The "
          "smaller ones are grouped in one \"Others\" slice.",
          "Choose {{Income}} or {{Expenses}}. Set {{No of most significant items}}. Click a "
          "slice or a legend line to see its {{Rank}}, {{Percentage}} and {{Amount}}."),
         ("Period - chart",
          "Bars of income and expenses, one per month or per year.",
          "{{Monthly}} or {{Annual}}. {{Duration}} : how many bars at a time, {{◀}} {{▶}} to "
          "move. Show {{Income}}, {{Expenses}}, both, or {{Deltas}} (income minus "
          "expenses)."),
         ("Period - list",
          "The same figures as a table : income, expenses and delta, their growth from one "
          "period to the next, and the cash balance.",
          "{{Monthly}} or {{Annual}}. {{Export legend...}} saves the table for a "
          "spreadsheet."),
         ("Period - heatmap",
          "One colored cell per month (a year × month grid) or per year : the stronger the "
          "color, the bigger the amount. Good and bad periods stand out at a glance.",
          "{{Incomes}}, {{Expenses}} or {{Delta}} (surpluses in the Max color, shortfalls in "
          "the Min color). {{Exclude current month}} hides an incomplete month. "
          "{{Cell size}}."),
         ("Tags",
          "For the tags you choose : total amount, weight (%) and number of events. Select a "
          "tag to see which CSDs make it up (table and pie chart).",
          "{{Income}} or {{Expenses}}, then {{Select tags...}}."),
         ("Compare CSD",
          "The financial events of up to 10 CSDs on one chart, one color each, with the "
          "total of each over the period.",
          "Check the CSDs in the list. Click a point to see its date and amount. {{Fit}} "
          "rescales the chart.")],
    ana_tip="**Compare periods :** change {{From}} and {{To}}, for example this year, then "
            "your first year of retirement, and look at the same tab again.",

    s8="Good habits",
    habits=["Save after each work session : [[Scenario → Save]].",
            "Update the start amount each time you open the scenario, using your accounts.",
            "Keep one scenario per \"what if\" : copy it with [[Scenario → Save as...]] and "
            "change only what differs.",
            "Disable a CSD (instead of deleting it) to see the curve without it.",
            "Not sure what a field does ? Hover over it : most fields have a tooltip."],
    more_title="To learn more",
    more="The **User Manual** ([[Help → User manual]]) explains every window and every "
         "option in detail.",
)

FR = dict(
    lang="fr", country="CA",
    file_title="Tutoriel rapide",
    product="Graphical Budget Planner 1.9.0",
    tagline="Voyez où va votre argent, en 15 minutes environ.",
    version="Version : 2026-10-28",
    footer="Graphical Budget Planner 1.9.0 · Tutoriel rapide",
    intro="Ce tutoriel vous mène d'une fenêtre vide à votre première prévision de solde. "
          "Lisez-le dans l'ordre la première fois; ensuite, il sert d'aide-mémoire.",

    s1="Comment fonctionne GBP",
    s1_lead="GBP répond à une question : **si mes revenus et mes dépenses se passent comme "
            "prévu, combien d'argent aurai-je à n'importe quel jour dans le futur ?**",
    c_titles=["1. Vous décrivez", "2. GBP génère", "3. Vous voyez"],
    c_captions=["Revenus et dépenses (DFM)", "Évènements financiers", "Courbe de solde"],
    c_chips=[(("Salaire", "chaque mois"), True), (("Loyer", "chaque mois"), False),
             (("Réparation auto", "une fois, en mars"), False)],
    c_events=[("01 janv", "Salaire", "+3 200", True), ("01 janv", "Loyer", "−1 450", False),
              ("01 févr", "Salaire", "+3 200", True), ("01 févr", "Loyer", "−1 450", False),
              ("14 mars", "Réparation", "−900", False)],
    c_start="montant de départ",
    c_time="temps →",
    s1_csd="Chaque revenu ou dépense que vous décrivez s'appelle une **Définition de flux "
           "monétaire**, ou **DFM**. À partir des DFM, GBP calcule chaque **évènement "
           "financier** futur (une date et un montant), puis les additionne jour après jour "
           "pour tracer la **courbe de solde de trésorerie**.",
    know_title="Bon à savoir",
    know=[("Le futur seulement",
           "GBP fait des projections à partir de demain. Il n'enregistre pas le passé : "
           "c'est un outil de planification, pas de comptabilité."),
          ("Un scénario à la fois",
           "Un fichier **scénario** contient toutes vos DFM. Gardez autant de fichiers que "
           "vous voulez, mais un seul est ouvert à la fois."),
          ("Montant de départ",
           "La courbe part de l'argent que vous avez **aujourd'hui**, saisi au bas de la "
           "fenêtre principale. Il n'est **pas** sauvegardé.")],
    tip_example="**Envie de jeter un coup d'œil d'abord ?** [[Scénario → Ouvrir exemple]] "
                "charge un budget complet. Rien de ce que vous y faites ne touche vos "
                "propres fichiers.",

    s2="Votre premier scénario en 6 étapes",
    steps=[
        ("Créer le scénario",
         "[[Scénario → Nouveau...]], choisissez la monnaie, puis appuyez sur {{Choisir}}. "
         "La monnaie est permanente : elle ne peut plus être changée."),
        ("Saisir l'argent disponible aujourd'hui",
         "Tout de suite, au bas de la fenêtre principale, saisissez le total de l'argent que "
         "vous avez aujourd'hui (tous comptes confondus, déterminé par vos propres moyens). "
         "La courbe part de ce montant. Faites-le chaque fois que vous créez ou ouvrez un "
         "scénario."),
        ("Le nommer",
         "[[Scénario → Éditer...]] ouvre la fenêtre d'édition du scénario. Saisissez un "
         "{{Nom}} (ce n'est pas le nom du fichier). Facultatif : {{Inflation annuelle}} et "
         "{{Durée de calcul}} (combien d'années à l'avance, 25 par défaut)."),
        ("Ajouter vos revenus et dépenses",
         "Dans la liste, choisissez d'abord {{Revenu}} ou {{Dépense}}, puis appuyez sur "
         "{{+ périodique}} pour ce qui se répète, ou sur {{+ irrégulier}} pour les montants "
         "ponctuels ou variables. Remplissez la fenêtre qui s'ouvre et appuyez sur "
         "{{Appliquer}}. Recommencez pour chaque élément."),
        ("Appliquer",
         "De retour dans la fenêtre d'édition du scénario, appuyez sur {{Appliquer}}. "
         "La courbe de la fenêtre principale est mise à jour immédiatement."),
        ("Sauvegarder",
         "[[Scénario → Sauvegarder]]. {{Appliquer}} ne met à jour que ce qui est en "
         "mémoire : sans sauvegarde, tous les changements sont perdus en quittant GBP."),
    ],
    warn_save="**Appliquer n'est pas sauvegarder.** {{Appliquer}} montre le résultat sur la "
              "courbe; seul [[Scénario → Sauvegarder]] l'écrit dans le fichier.",

    s3="Périodique ou irrégulier ?",
    k_periodic="Périodique", k_periodic_sub="même montant, rythme régulier",
    k_irregular="Irrégulier", k_irregular_sub="dates et montants que vous listez",
    k_starts="Débute le", k_ends="Termine au plus tard le", k_every="chaque Mois × 1",
    k_list="Liste des évènements financiers",
    kinds_head=("Type", "À utiliser pour", "Exemples"),
    kinds_rows=[("Périodique", "Les montants qui se répètent à un rythme régulier",
                 "Salaire, loyer, hypothèque, assurance, abonnements"),
                ("Irrégulier", "Les montants ponctuels ou variables, aux dates de votre choix",
                 "Prime, cadeau, réparation auto, rénovation, remboursement d'impôt")],
    fields_title="Les champs les plus importants d'un élément périodique",
    fields=[("Montant", "Le montant de chaque occurrence."),
            ("Période + Multiplicateur", "Le rythme : Jour, Semaine, Mois, Fin-de-mois ou "
             "Année, multiplié. Mois × 3 = chaque trimestre, Semaine × 2 = aux deux "
             "semaines."),
            ("Débute le", "La date de la première occurrence."),
            ("Termine au plus tard le", "Dernière date, facultative. Laissez vide pour "
             "continuer jusqu'à la fin du calcul."),
            ("Onglet Avancé", "Croissance facultative du montant : {{Aucune croissance}}, "
             "{{Appliquer l'inflation du scénario}}, ou votre propre taux, constant ou "
             "variable.")],
    tip_check="**Vérifiez ce que vous avez saisi :** {{Voir les occurrences...}}, dans la "
              "fenêtre de l'élément, liste chaque date et chaque montant que GBP générera "
              "pour cet élément.",

    s4="La fenêtre principale en un coup d'œil",
    mw_title="Mon budget.json — GBP",
    mw_menus=["Scénario", "Outils", "Aide"],
    mw_chart_title="Mon budget",
    mw_p_point="Point sélectionné", mw_p_stats="Statistiques", mw_p_range="Intervalle",
    mw_events=[("3 200,00 : Salaire", "#23864F"), ("−1 450,00 : Loyer", "#C0392B")],
    mw_point_lines=["Date : 2027-03-01", "Montant : 12 994,03", "Changement : 1 750,00"],
    mw_stats_lines=[("Jours réels :", "612"), ("Total des évènements :", "1 436"),
                    ("Points dans la vue :", "20"), ("Points affichés :", "20")],
    mw_range_lines=[("De :", "2026-oct.-29"), ("À :", "2027-oct.-29"),
                    ("Écart :", "1a 0m 0j")],
    mw_start="Montant de départ au 2026-10-28 :",
    mw_amount="2 000,00",
    points="Points", guides="Grille", export="Exporter...",
    fit="Ajuster", eoy="FA", custom="Spécifique",
    legend=[("Menus", "[[Scénario]] pour les fichiers et l'édition, [[Outils]] pour "
             "l'Analyse et les Options, [[Aide]] pour ce tutoriel et le Manuel de "
             "l'utilisateur."),
            ("Boutons d'intervalle", "Affichent le prochain mois ... les 25 prochaines "
             "années. {{Max}} : tout le calcul. {{Ajuster}} : l'intervalle de tous les "
             "évènements. {{FA}} : jusqu'à la fin de l'année. {{Spécifique}} : les dates de "
             "votre choix. {{◀}} {{▶}} : déplacer d'une fenêtre vers la gauche ou la "
             "droite."),
            ("La courbe de solde", "Chaque point est le solde à la fin d'un jour ayant au "
             "moins un évènement. La ligne plate est le zéro : sous elle, vous manquez "
             "d'argent."),
            ("Cliquer un point", "Il est mis en évidence et devient le point sélectionné."),
            ("Point sélectionné", "Date, solde, changement de ce jour, et la liste de tous "
             "les évènements de ce jour."),
            ("Statistiques", "{{Jours réels}} : combien de jours ont au moins un "
             "évènement. {{Total des évènements}} : combien d'évènements au total. {{Points "
             "dans la vue}} : les points de la partie visible du graphique, en vert s'ils "
             "sont affichés, en rouge s'il y en a trop pour les afficher (zoomez; la limite "
             "se règle dans les Options). {{Points affichés}} : combien sont réellement "
             "affichés."),
            ("Intervalle", "Premier ({{De}}) et dernier ({{À}}) jour visibles, et l'écart "
             "entre les deux ({{Écart}}, en années, mois et jours)."),
            ("Montant de départ", "Votre argent disponible aujourd'hui. Changez-le à tout "
             "moment : la courbe monte ou descend."),
            ("Options d'affichage", "{{Points}} montre ou cache les points (un ⓘ apparaît "
             "à côté quand il y en a trop pour les afficher), {{Grille}} le quadrillage. {{Exporter...}} enregistre tous les points de la courbe dans un "
             "fichier CSV.")],

    s5="Se déplacer sur la courbe",
    mouse_head=("Faites ceci", "Pour"),
    mouse=[("Molette de la souris", "Zoomer, autour du pointeur de la souris"),
           ("MAJ + molette", "Zoomer sur le temps seulement (horizontal)"),
           ("CTRL + molette", "Zoomer sur les montants seulement (vertical)"),
           ("Clic gauche et glisser", "Déplacer la courbe dans toutes les directions"),
           ("Cliquer un point", "Voir les détails et les évènements de ce jour"),
           ("{{◀}} ou {{▶}}", "Reculer ou avancer d'une fenêtre complète dans le temps"),
           ("Un bouton d'intervalle", "Réinitialiser la vue (1M ... 25Y, Max, Ajuster, FA, "
            "Spécifique)")],
    tip_lost="**Perdu dans le graphique ?** Appuyez sur {{Ajuster}} pour tout revoir.",

    s6="Étiquettes",
    tags_lead="Les **étiquettes** sont des libellés que vous posez sur les DFM (« Logement », "
              "« Auto », « Loisirs »...) pour les regrouper. Une DFM peut avoir plusieurs "
              "étiquettes.",
    tags_steps=["Dans la fenêtre d'édition du scénario, appuyez sur {{Gérer les "
                "étiquettes...}}.",
                "Créez vos étiquettes et choisissez les DFM liées à chacune. Vous pouvez "
                "aussi lier des étiquettes depuis la fenêtre d'un élément, dans "
                "{{Étiquettes liées}}.",
                "Utilisez-les pour filtrer la liste (filtre {{Étiquettes}}) et dans l'onglet "
                "{{Étiquettes}} de l'Analyse."],
    s7="Analyse",
    ana_lead="[[Outils → Analyse...]] transforme les évènements financiers d'une période en "
             "graphiques et en tableaux, pour comprendre d'où vient l'argent et où il va.",
    ana_steps=["Ouvrez [[Outils → Analyse...]] (un scénario doit être chargé).",
               "Choisissez la période avec {{De}} et {{À}} (par défaut : de demain à 5 ans "
               "plus tard), puis appuyez sur {{Appliquer}}. Tous les onglets utilisent cette "
               "période : changez-la pour comparer, par exemple, cette année avec votre "
               "première année de retraite.",
               "Choisissez un onglet. {{Exporter le graphique...}} enregistre le graphique en "
               "image PNG, {{Exporter la légende...}} enregistre le tableau ou la légende en "
               "fichier CSV. L'onglet {{Aide}} décrit chaque onglet."],
    ana_head=("Onglet", "Ce que vous voyez", "Essayez ceci"),
    ana=[("Poid relatif",
          "Un diagramme circulaire des revenus (ou dépenses) qui pèsent le plus sur la "
          "période. Les plus petits sont regroupés dans une part « Autres ».",
          "Choisissez {{Revenus}} ou {{Dépenses}}. Réglez {{Nombre d'items les plus "
          "significatifs}}. Cliquez une part ou une ligne de la légende pour voir son "
          "{{Rang}}, son {{Pourcentage}} et son {{Montant}}."),
         ("Période - graphique",
          "Des barres de revenus et de dépenses, une par mois ou par année.",
          "{{Mensuel}} ou {{Annuel}}. {{Durée}} : combien de barres à la fois, {{◀}} {{▶}} "
          "pour se déplacer. Montrez {{Revenus}}, {{Dépenses}}, les deux, ou {{Écarts}} "
          "(revenus moins dépenses)."),
         ("Période - liste",
          "Les mêmes chiffres en tableau : revenus, dépenses et écart, leur croissance d'une "
          "période à l'autre, et le solde.",
          "{{Mensuel}} ou {{Annuel}}. {{Exporter la légende...}} enregistre le tableau pour "
          "un tableur."),
         ("Période - carte thermique",
          "Une cellule colorée par mois (une grille année × mois) ou par année : plus la "
          "couleur est forte, plus le montant est grand. Les bonnes et les mauvaises périodes "
          "ressortent d'un coup d'œil.",
          "{{Revenus}}, {{Dépenses}} ou {{Écart}} (surplus dans la couleur max, manques dans "
          "la couleur min). {{Exclure le mois en cours}} cache un mois incomplet. "
          "{{Taille des cellules}}."),
         ("Étiquettes",
          "Pour les étiquettes choisies : montant total, poids (%) et nombre d'évènements. "
          "Sélectionnez une étiquette pour voir les DFM qui la composent (tableau et "
          "diagramme circulaire).",
          "{{Revenus}} ou {{Dépenses}}, puis {{Choisir les étiquettes...}}."),
         ("Comparer DFM",
          "Les évènements financiers de 10 DFM au plus sur un même graphique, une couleur "
          "chacune, avec le total de chacune sur la période.",
          "Cochez les DFM dans la liste. Cliquez un point pour voir sa date et son montant. "
          "{{Ajuster}} remet le graphique à l'échelle.")],
    ana_tip="**Comparez des périodes :** changez {{De}} et {{À}}, par exemple cette année, "
            "puis votre première année de retraite, et regardez de nouveau le même onglet.",

    s8="Bonnes habitudes",
    habits=["Sauvegardez après chaque séance de travail : [[Scénario → Sauvegarder]].",
            "Mettez à jour le montant de départ chaque fois que vous ouvrez le scénario, à "
            "l'aide de vos comptes.",
            "Un scénario par « et si » : copiez-le avec [[Scénario → Sauvegarder sous...]] "
            "et ne changez que ce qui diffère.",
            "Désactivez une DFM (au lieu de la supprimer) pour voir la courbe sans elle.",
            "Un champ n'est pas clair ? Survolez-le : la plupart ont une infobulle."],
    more_title="Pour en savoir plus",
    more="Le **Manuel de l'utilisateur** ([[Aide → Manuel de l'utilisateur]]) explique en "
         "détail chaque fenêtre et chaque option.",
)
