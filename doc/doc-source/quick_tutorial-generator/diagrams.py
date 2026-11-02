"""SVG diagrams for the GBP Quick Tutorial (one set per language)."""

ACCENT = "#1F5E99"
ACCENT_LIGHT = "#E8F0F9"
INCOME = "#23864F"
EXPENSE = "#C0392B"
INK = "#2B2F36"
MUTED = "#6B7280"
LINE = "#C9D1DC"
FONT = "Noto Sans"


def _svg(w, h, body):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" '
            f'viewBox="0 0 {w} {h}" font-family="{FONT}">{body}</svg>')


def _t(x, y, s, size=13, fill=INK, weight="normal", anchor="start", style="normal",
       family=None):
    s = s.replace("&", "&amp;").replace("<", "&lt;")
    return (f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" font-weight="{weight}" '
            f'font-style="{style}" text-anchor="{anchor}"'
            + (f' font-family="{family}"' if family else "") + f'>{s}</text>')


def _arrow(x1, y, x2, color=ACCENT):
    return (f'<line x1="{x1}" y1="{y}" x2="{x2 - 10}" y2="{y}" stroke="{color}" '
            f'stroke-width="3"/>'
            f'<polygon points="{x2},{y} {x2 - 13},{y - 8} {x2 - 13},{y + 8}" fill="{color}"/>')


def _badge(cx, cy, n, r=13, fill=ACCENT):
    return (f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}"/>'
            + _t(cx, cy + 5, str(n), 14, "#FFFFFF", "bold", "middle"))


def concept(L):
    """Three boxes: you describe -> GBP generates -> you see the curve."""
    w, h = 900, 300
    b = []
    bw, bh, top = 250, 230, 20
    xs = [10, 325, 640]
    for i, x in enumerate(xs):
        b.append(f'<rect x="{x}" y="{top}" width="{bw}" height="{bh}" rx="14" '
                 f'fill="#FFFFFF" stroke="{LINE}" stroke-width="1.5"/>')
        b.append(f'<rect x="{x}" y="{top}" width="{bw}" height="44" rx="14" fill="{ACCENT}"/>')
        b.append(f'<rect x="{x}" y="{top + 30}" width="{bw}" height="14" fill="{ACCENT}"/>')
        b.append(_t(x + bw / 2, top + 29, L["c_titles"][i], 16, "#FFFFFF", "bold", "middle"))
        b.append(_t(x + bw / 2, top + bh + 30, L["c_captions"][i], 14, ACCENT, "bold",
                    "middle"))
    b.append(_arrow(xs[0] + bw + 8, top + bh / 2, xs[1] - 8))
    b.append(_arrow(xs[1] + bw + 8, top + bh / 2, xs[2] - 8))

    # box 1 : chips
    for j, (label, inc) in enumerate(L["c_chips"]):
        y = top + 64 + j * 52
        col = INCOME if inc else EXPENSE
        b.append(f'<rect x="{xs[0] + 18}" y="{y}" width="{bw - 36}" height="40" rx="8" '
                 f'fill="{"#E9F6EF" if inc else "#FBECEA"}" stroke="{col}" stroke-width="1"/>')
        b.append(_t(xs[0] + 34, y + 18, label[0], 13.5, INK, "bold"))
        b.append(_t(xs[0] + 34, y + 33, label[1], 11.5, MUTED))
        b.append(_t(xs[0] + bw - 32, y + 27, "+" if inc else "−", 20, col, "bold", "middle"))

    # box 2 : events
    for j, (d, label, amt, inc) in enumerate(L["c_events"]):
        y = top + 78 + j * 30
        b.append(_t(xs[1] + 18, y, d, 12, MUTED))
        b.append(_t(xs[1] + 82, y, label, 12.5, INK))
        b.append(_t(xs[1] + bw - 16, y, amt, 12.5, INCOME if inc else EXPENSE, "bold", "end"))
        b.append(f'<line x1="{xs[1] + 16}" y1="{y + 10}" x2="{xs[1] + bw - 16}" '
                 f'y2="{y + 10}" stroke="#EEF1F5" stroke-width="1"/>')

    # box 3 : step curve
    x0, y0 = xs[2] + 26, top + 70
    cw, ch = bw - 46, 140
    b.append(f'<line x1="{x0}" y1="{y0 + ch}" x2="{x0 + cw}" y2="{y0 + ch}" stroke="{LINE}"/>')
    b.append(f'<line x1="{x0}" y1="{y0}" x2="{x0}" y2="{y0 + ch}" stroke="{LINE}"/>')
    vals = [70, 70, 110, 95, 95, 80, 120, 108, 108, 60, 60, 100, 90, 130]
    step = cw / (len(vals) - 1)
    pts = []
    for k, v in enumerate(vals):
        x = x0 + k * step
        y = y0 + ch - v
        if k:
            pts.append(f"{x:.1f},{pts_y:.1f}")
        pts.append(f"{x:.1f},{y:.1f}")
        pts_y = y
    b.append(f'<polyline points="{" ".join(pts)}" fill="none" stroke="{ACCENT}" '
             f'stroke-width="2.5"/>')
    for k, v in enumerate(vals):
        b.append(f'<circle cx="{x0 + k * step:.1f}" cy="{y0 + ch - v:.1f}" r="3.2" '
                 f'fill="{ACCENT}"/>')
    b.append(f'<circle cx="{x0:.1f}" cy="{y0 + ch - vals[0]:.1f}" r="6" fill="none" '
             f'stroke="{INCOME}" stroke-width="2.5"/>')
    b.append(_t(x0 + 8, y0 + ch - vals[0] + 26, L["c_start"], 11.5, INCOME, "bold"))
    b.append(_t(x0 + cw, y0 + ch + 16, L["c_time"], 11, MUTED, anchor="end"))
    return _svg(w, h, "".join(b))


def kinds(L):
    """Two timelines: periodic (regular) vs irregular (uneven)."""
    w, h = 900, 242
    b = []
    for row, (title, sub, col, marks) in enumerate([
            (L["k_periodic"], L["k_periodic_sub"], ACCENT,
             [(i * 64, 46) for i in range(11)]),
            (L["k_irregular"], L["k_irregular_sub"], "#8A5CC2",
             [(25, 30), (160, 78), (205, 22), (420, 60), (610, 90)])]):
        y = 30 + row * 118
        b.append(_t(10, y + 6, title, 16, col, "bold"))
        b.append(_t(10, y + 26, sub, 12, MUTED))
        ax = 230
        base = y + 62
        b.append(f'<line x1="{ax}" y1="{base}" x2="{ax + 660}" y2="{base}" stroke="{LINE}" '
                 f'stroke-width="2"/>')
        for x, hh in marks:
            b.append(f'<rect x="{ax + 10 + x}" y="{base - hh}" width="18" height="{hh}" '
                     f'rx="3" fill="{col}"/>')
        if row == 0:
            b.append(f'<line x1="{ax + 19}" y1="{base + 4}" x2="{ax + 19}" y2="{base + 22}" '
                     f'stroke="{INK}" stroke-width="1.5"/>')
            b.append(_t(ax + 27, base + 22, L["k_starts"], 11.5, INK))
            b.append(f'<line x1="{ax + 659}" y1="{base + 4}" x2="{ax + 659}" '
                     f'y2="{base + 22}" stroke="{INK}" stroke-width="1.5"/>')
            b.append(_t(ax + 651, base + 22, L["k_ends"], 11.5, INK, anchor="end"))
            b.append(_t(ax + 339, base + 22, L["k_every"], 11.5, ACCENT, "bold",
                        "middle"))
        else:
            b.append(_t(ax + 339, base + 22, L["k_list"], 11.5, "#8A5CC2", "bold", "middle"))
    return _svg(w, h, "".join(b))


def main_window(L):
    """Schematic of the GBP 1.9.0 main window, with numbered callouts."""
    w, h = 900, 512
    b = []
    # frame
    b.append(f'<rect x="4" y="4" width="{w - 8}" height="{h - 8}" rx="10" fill="#F6F7F9" '
             f'stroke="#9AA4B2" stroke-width="1.5"/>')
    b.append(f'<rect x="4" y="4" width="{w - 8}" height="30" rx="10" fill="#E3E7EC"/>')
    b.append(f'<rect x="4" y="24" width="{w - 8}" height="10" fill="#E3E7EC"/>')
    b.append(_t(w / 2, 24, L["mw_title"], 12.5, INK, anchor="middle"))
    for k in range(3):
        b.append(f'<circle cx="{w - 24 - k * 20}" cy="19" r="5.5" fill="#B9C1CC"/>')
    # menu bar
    x = 18
    for m in L["mw_menus"]:
        b.append(_t(x, 54, m, 13, INK))
        x += len(m) * 8 + 22
    menu_end = x
    # toolbar
    x = 18
    for btn in ["1M", "3M", "6M", "1Y", "2Y", "5Y", "10Y", "25Y", "Max", L["fit"], L["eoy"],
                L["custom"]]:
        bw = 14 + len(btn) * 7.5
        b.append(f'<rect x="{x}" y="66" width="{bw}" height="22" rx="4" fill="#FFFFFF" '
                 f'stroke="#B9C1CC"/>')
        b.append(_t(x + bw / 2, 81, btn, 11, INK, anchor="middle"))
        x += bw + 4
    x += 14
    for btn in ["◀", "▶"]:
        b.append(f'<rect x="{x}" y="66" width="26" height="22" rx="4" fill="#FFFFFF" '
                 f'stroke="#B9C1CC"/>')
        b.append(_t(x + 13, 81, btn, 11, INK, anchor="middle", family="DejaVu Sans"))
        x += 30
    toolbar_end = x
    # chart
    cx, cy, cw, ch = 18, 100, 610, 340
    b.append(f'<rect x="{cx}" y="{cy}" width="{cw}" height="{ch}" rx="6" fill="#FFFFFF" '
             f'stroke="#D5DBE3"/>')
    b.append(_t(cx + cw / 2, cy + 24, L["mw_chart_title"], 13, MUTED, anchor="middle"))
    for g in range(1, 6):
        gy = cy + 40 + g * 48
        b.append(f'<line x1="{cx + 50}" y1="{gy}" x2="{cx + cw - 16}" y2="{gy}" '
                 f'stroke="#EEF1F5"/>')
    vals = [120, 120, 170, 150, 140, 140, 230, 215, 200, 200, 130, 120, 110, 180, 165,
            165, 240, 225, 210, 195]
    x0, base = cx + 56, cy + ch - 30
    step = (cw - 80) / (len(vals) - 1)
    pts, prev = [], None
    for k, v in enumerate(vals):
        px, py = x0 + k * step, base - v
        if prev is not None:
            pts.append(f"{px:.1f},{prev:.1f}")
        pts.append(f"{px:.1f},{py:.1f}")
        prev = py
    b.append(f'<polyline points="{" ".join(pts)}" fill="none" stroke="#3A3F47" '
             f'stroke-width="1.6"/>')
    for k, v in enumerate(vals):
        b.append(f'<circle cx="{x0 + k * step:.1f}" cy="{base - v:.1f}" r="4" '
                 f'fill="#2D5BD8"/>')
    sel = 6
    selx, sely = x0 + sel * step, base - vals[sel]
    b.append(f'<circle cx="{selx:.1f}" cy="{sely:.1f}" r="7" fill="#1FAF4F" '
             f'stroke="#FFFFFF" stroke-width="2"/>')
    b.append(f'<line x1="{cx + 50}" y1="{base + 14}" x2="{cx + cw - 16}" y2="{base + 14}" '
             f'stroke="#26A69A" stroke-width="1.5"/>')
    # right panels
    px, pw = 646, 238
    panels = [(L["mw_p_point"], 100, 150), (L["mw_p_stats"], 262, 104),
              (L["mw_p_range"], 378, 88)]
    for title, py, ph in panels:
        b.append(_t(px, py + 12, title, 13, INK, "bold"))
        b.append(f'<rect x="{px}" y="{py + 20}" width="{pw}" height="{ph - 20}" rx="5" '
                 f'fill="#FFFFFF" stroke="#D5DBE3"/>')
    for k, txt in enumerate(L["mw_point_lines"]):
        b.append(_t(px + 10, 140 + k * 17, txt, 11, INCOME if k else INK))
    b.append(f'<line x1="{px + 10}" y1="188" x2="{px + pw - 10}" y2="188" stroke="#EEF1F5"/>')
    for k, (txt, col) in enumerate(L["mw_events"]):
        b.append(_t(px + 10, 205 + k * 18, txt, 11, col))
    # Statistics : label, value, value colored in green for "Points in view" (points shown)
    for k, (lbl, val) in enumerate(L["mw_stats_lines"]):
        y = 300 + k * 17
        b.append(_t(px + 10, y, lbl, 11, MUTED))
        b.append(_t(px + pw - 12, y, val, 11, INCOME if k == 2 else INK, anchor="end"))
    for k, (lbl, val) in enumerate(L["mw_range_lines"]):
        y = 416 + k * 17
        b.append(_t(px + 10, y, lbl, 11, MUTED))
        b.append(_t(px + pw - 12, y, val, 11, INK, anchor="end"))
    # bottom bar
    by = 480
    b.append(_t(18, by, L["mw_start"], 12.5, INK))
    sx = 18 + len(L["mw_start"]) * 6.6 + 8
    b.append(f'<rect x="{sx}" y="{by - 16}" width="96" height="24" rx="4" fill="#FFFFFF" '
             f'stroke="{INCOME}" stroke-width="2"/>')
    b.append(_t(sx + 8, by + 1, L["mw_amount"], 12, INK))
    chk = 424
    chk_start = chk
    for lbl in [L["points"], L["guides"]]:
        b.append(f'<rect x="{chk}" y="{by - 12}" width="13" height="13" rx="2" fill="#FFFFFF" '
                 f'stroke="#7A8494"/>')
        b.append(f'<polyline points="{chk + 3},{by - 6} {chk + 6},{by - 2} {chk + 11},{by - 10}"'
                 f' fill="none" stroke="{ACCENT}" stroke-width="2"/>')
        b.append(_t(chk + 19, by, lbl, 12, INK))
        chk += 24 + len(lbl) * 6.4
    b.append(f'<rect x="578" y="{by - 16}" width="50" height="24" rx="4" fill="#FFFFFF" '
             f'stroke="#B9C1CC"/>')
    b.append(_t(603, by + 1, L["export"], 10, INK, anchor="middle"))
    # callouts
    b.append(_badge(menu_end + 4, 49, 1, 11))
    b.append(_badge(toolbar_end + 16, 77, 2, 11))
    b.append(_badge(cx + cw - 30, cy + 26, 3, 11))
    b.append(_badge(selx + 20, sely - 18, 4, 11))
    b.append(_badge(px + pw - 12, 112, 5, 11))
    b.append(_badge(px + pw - 12, 274, 6, 11))
    b.append(_badge(px + pw - 12, 390, 7, 11))
    b.append(_badge(sx + 110, by - 4, 8, 11))
    assert (chk_start - 16) - (sx + 110) >= 24, "badges 6 and 7 overlap"
    assert chk < 582, "checkboxes overlap Export"
    b.append(_badge(chk_start - 16, by - 4, 9, 11))
    return _svg(w, h, "".join(b))


def _btn(x, y, label, size=11, w=None, bold=False, color=INK, family=None):
    w = w or (14 + len(label) * size * 0.58)
    return (f'<rect x="{x}" y="{y}" width="{w:.1f}" height="22" rx="4" fill="#FFFFFF" '
            f'stroke="#B9C1CC"/>'
            + _t(x + w / 2, y + 15, label, size, color, "bold" if bold else "normal",
                 "middle", family=family)), w


def edit_scenario(L):
    """Schematic of the 'Edit current scenario' window, with numbered callouts."""
    w, h = 900, 560
    b = [f'<rect x="4" y="4" width="{w - 8}" height="{h - 8}" rx="10" fill="#F6F7F9" '
         f'stroke="#9AA4B2" stroke-width="1.5"/>',
         f'<rect x="4" y="4" width="{w - 8}" height="30" rx="10" fill="#E3E7EC"/>',
         f'<rect x="4" y="24" width="{w - 8}" height="10" fill="#E3E7EC"/>',
         _t(w / 2, 24, L["es_title"], 12.5, INK, anchor="middle")]
    lx, fx = 190, 200   # label right edge, field left
    rows = [(60, L["es_name"]), (98, L["es_desc"]), (150, L["es_infl"]), (186, L["es_dur"])]
    for y, lbl in rows:
        b.append(_t(lx, y + 15, lbl, 12, INK, anchor="end"))
    b.append(f'<rect x="{fx}" y="60" width="420" height="24" rx="3" fill="#FFFFFF" '
             f'stroke="#B9C1CC"/>')
    b.append(_t(fx + 8, 77, L["es_name_val"], 12, INK))
    b.append(f'<rect x="{fx}" y="94" width="560" height="44" rx="3" fill="#FFFFFF" '
             f'stroke="#B9C1CC"/>')
    s, bw = _btn(fx + 572, 105, L["es_full"])
    b.append(s)
    # inflation
    x = fx
    for k, lbl in enumerate([L["es_const"], L["es_var"]]):
        b.append(f'<circle cx="{x + 7}" cy="{162}" r="6" fill="#FFFFFF" stroke="#7A8494"/>')
        if k == 0:
            b.append(f'<circle cx="{x + 7}" cy="162" r="3" fill="{ACCENT}"/>')
        b.append(_t(x + 18, 166, lbl, 12, INK))
        x += 26 + len(lbl) * 7
        if k == 0:
            b.append(f'<rect x="{x}" y="151" width="80" height="22" rx="3" fill="#FFFFFF" '
                     f'stroke="#B9C1CC"/>')
            b.append(_t(x + 72, 166, "2.00 %" if L["lang"] == "en" else "2,00 %", 11.5,
                        INK, anchor="end"))
            x += 100
    s, _ = _btn(x, 151, L["es_viewedit"])
    b.append(s)
    b.append(f'<rect x="{fx}" y="186" width="60" height="22" rx="3" fill="#FFFFFF" '
             f'stroke="#B9C1CC"/>')
    b.append(_t(fx + 52, 201, "25", 11.5, INK, anchor="end"))
    b.append(_t(fx + 68, 201, L["es_years"], 12, INK))
    # group box
    gy = 226
    b.append(f'<rect x="18" y="{gy}" width="{w - 36}" height="262" rx="6" fill="none" '
             f'stroke="#C9D1DC"/>')
    b.append(f'<rect x="30" y="{gy - 9}" width="{len(L["es_group"]) * 7.2 + 12}" height="18" '
             f'fill="#F6F7F9"/>')
    b.append(_t(36, gy + 5, L["es_group"], 12, INK, "bold"))
    # filters
    fy = gy + 30
    x = 32
    b.append(_t(x, fy + 4, L["es_filters"], 11.5, INK))
    x += len(L["es_filters"]) * 6.6 + 10
    for k, lbl in enumerate([L["es_income"], L["es_expense"]]):
        b.append(f'<circle cx="{x + 6}" cy="{fy}" r="6" fill="#FFFFFF" stroke="#7A8494"/>')
        if k == 0:
            b.append(f'<circle cx="{x + 6}" cy="{fy}" r="3" fill="{ACCENT}"/>')
        b.append(_t(x + 16, fy + 4, lbl, 11.5, INCOME if k == 0 else EXPENSE, "bold"))
        x += 26 + len(lbl) * 7
    radio_end = x
    x += 14
    filt_start = x
    for k, lbl in enumerate(L["es_checks"]):
        b.append(f'<rect x="{x}" y="{fy - 6}" width="12" height="12" rx="2" fill="#FFFFFF" '
                 f'stroke="#7A8494"/>')
        if k < 4:
            b.append(f'<polyline points="{x + 2.5},{fy} {x + 5},{fy + 3} {x + 10},{fy - 4}" '
                     f'fill="none" stroke="{ACCENT}" stroke-width="1.8"/>')
        b.append(_t(x + 17, fy + 4, lbl, 11, INK))
        x += 26 + len(lbl) * 6.2
    s, bw = _btn(x, fy - 11, L["es_set"], 10.5)
    b.append(s)
    # table
    ty = fy + 20
    cols = [(32, L["es_cols"][0], 90), (122, L["es_cols"][1], 150), (272, L["es_cols"][2], 100),
            (372, L["es_cols"][3], 494)]
    b.append(f'<rect x="32" y="{ty}" width="834" height="150" fill="#FFFFFF" stroke="#C9D1DC"/>')
    b.append(f'<rect x="32" y="{ty}" width="834" height="22" fill="#EEF1F5"/>')
    for cx_, lbl, cw in cols:
        b.append(_t(cx_ + 6, ty + 15, lbl, 11, INK, "bold"))
    for r, (typ, name, amt, info) in enumerate(L["es_rows"]):
        ry = ty + 22 + r * 24
        if r == 0:
            b.append(f'<rect x="33" y="{ry}" width="832" height="24" fill="#DCE8F6"/>')
        vals = [typ, name, amt, info]
        for k, (cx_, _, cw) in enumerate(cols):
            col = INCOME if (k == 1 and L["es_rows_inc"][r]) else INK
            b.append(_t(cx_ + 6 + (cw - 14 if k == 2 else 0), ry + 16, vals[k], 11,
                        col if k == 1 else (MUTED if k == 3 else INK),
                        anchor="end" if k == 2 else "start"))
    # action buttons
    by = ty + 162
    x = 32
    new_end = None
    # font size so that the whole row of buttons fits in the group box
    bsize = 10.5
    while sum(14 + len(t) * bsize * 0.58 + 5 for t in L["es_buttons"]) > 830:
        bsize -= 0.25
    for k, lbl in enumerate(L["es_buttons"]):
        s, bw = _btn(x, by, lbl, bsize, bold=k < 2, color=ACCENT if k < 2 else INK)
        b.append(s)
        x += bw + 5
        if k == 1:
            new_end = x
    # bottom row
    yb = h - 50
    s, mt_w = _btn(18, yb, L["es_tags"], 11)
    b.append(s)
    xr = w - 18
    rights = []
    # state shown : changes not applied yet -> Cancel and Apply (Close is hidden)
    for lbl in reversed([L["es_cancel"], L["es_apply"]]):
        bw = 14 + len(lbl) * 11 * 0.58
        xr -= bw
        rights.append((xr, lbl, bw))
        xr -= 6
    for xx, lbl, bw in rights:
        s, _ = _btn(xx, yb, lbl, 11, w=bw, bold=(lbl == L["es_apply"]),
                    color=ACCENT if lbl == L["es_apply"] else INK)
        b.append(s)
    warn_x = rights[-1][0] - 22
    b.append(_t(warn_x, yb + 17, "⚠", 17, "#FF9800", family="DejaVu Sans"))
    # callouts
    b.append(_badge(fx + 436, 72, 1, 11))
    b.append(_badge(lx - len(L["es_infl"]) * 6.8 - 18, 162, 2, 11))
    b.append(_badge(fx + 120 + len(L["es_years"]) * 6.5, 197, 3, 11))
    b.append(_badge((radio_end + filt_start) / 2, fy - 18, 4, 11))
    b.append(_badge(x + 2 if False else filt_start + 40, fy - 20, 5, 11))
    b.append(_badge(850, ty + 34, 6, 11))
    b.append(_badge(new_end - 2, by - 6, 7, 11))
    b.append(_badge(18 + mt_w + 16, yb + 11, 8, 11))
    b.append(_badge(warn_x - 16, yb + 11, 9, 11))
    return _svg(w, h, "".join(b))


def example_curve(L, bal):
    """Worked example : the cash balance curve of the example, with annotations."""
    from datetime import date
    w, h = 900, 320
    b = []
    x0, x1, y0, y1 = 70, 880, 22, 290          # plot area
    d0, d1 = date(2026, 10, 28), date(2027, 10, 31)
    vmin, vmax = -1000, 10000

    def X(d):
        return x0 + (d - d0).days / (d1 - d0).days * (x1 - x0)

    def Y(v):
        return y1 - (v - vmin) / (vmax - vmin) * (y1 - y0)
    b.append(f'<rect x="{x0}" y="{y0}" width="{x1 - x0}" height="{y1 - y0}" fill="#FFFFFF" '
             f'stroke="#D5DBE3"/>')
    for v in range(0, 10001, 2000):
        b.append(f'<line x1="{x0}" y1="{Y(v):.1f}" x2="{x1}" y2="{Y(v):.1f}" '
                 f'stroke="#EEF1F5"/>')
        b.append(_t(x0 - 8, Y(v) + 4, L["fmt"](v), 10.5, MUTED, anchor="end"))
    b.append(f'<rect x="{x0}" y="{Y(0):.1f}" width="{x1 - x0}" height="{Y(vmin) - Y(0):.1f}" '
             f'fill="#FBECEA" opacity="0.7"/>')
    b.append(f'<line x1="{x0}" y1="{Y(0):.1f}" x2="{x1}" y2="{Y(0):.1f}" stroke="#26A69A" '
             f'stroke-width="1.5"/>')
    for m in range(12):
        y_, mo = divmod(10 + m, 12)
        d = date(2026 + y_, mo + 1, 1)
        b.append(f'<line x1="{X(d):.1f}" y1="{y1}" x2="{X(d):.1f}" y2="{y1 + 5}" '
                 f'stroke="#9AA4B2"/>')
        b.append(_t(X(d) + 2, y1 + 18, L["months"][mo], 10.5, MUTED))
    # start point (today) and step line
    pts = [(X(d0), Y(800))]
    prev = Y(800)
    for d, v, _ in bal:
        pts.append((X(d), prev))
        pts.append((X(d), Y(v)))
        prev = Y(v)
    b.append('<polyline points="' + " ".join(f"{x:.1f},{y:.1f}" for x, y in pts)
             + f'" fill="none" stroke="#3A3F47" stroke-width="1.4"/>')
    for d, v, _ in bal:
        col = EXPENSE if v < 0 else "#2D5BD8"
        b.append(f'<circle cx="{X(d):.1f}" cy="{Y(v):.1f}" r="3.4" fill="{col}"/>')
    b.append(f'<circle cx="{X(d0):.1f}" cy="{Y(800):.1f}" r="5" fill="none" stroke="{INCOME}" '
             f'stroke-width="2.2"/>')
    # annotations : (date, letter, dx, dy)
    marks = {date(2026, 11, 1): ("A", 20, 4), date(2027, 1, 10): ("B", 0, 26),
             date(2027, 3, 14): ("C", 0, 26), date(2027, 7, 5): ("D", 0, 26),
             date(2027, 10, 31): ("E", -14, -22)}
    for d, v, _ in bal:
        if d in marks:
            letter, dx, dy = marks[d]
            b.append(_badge(X(d) + dx, Y(v) + dy, letter, 10, EXPENSE if letter != "E"
                            else INCOME))
    b.append(_t(X(d0) + 8, Y(800) - 9, L["ex_start_lbl"], 10.5, INCOME, "bold"))
    return _svg(w, h, "".join(b))


def analysis_thumbs(L):
    """Six small cards : one sketch per Analysis tab."""
    w, h = 900, 330
    cw, ch, gap = 288, 150, 18
    b = []
    pal = ["#2D5BD8", "#E69F00", "#23864F", "#C0392B", "#8A5CC2", "#26A69A"]
    for i, name in enumerate(L["ana_tabs"]):
        cx = (i % 3) * (cw + gap)
        cy = (i // 3) * (ch + gap + 12)
        b.append(f'<rect x="{cx + 1}" y="{cy + 1}" width="{cw - 2}" height="{ch}" rx="10" '
                 f'fill="#FFFFFF" stroke="{LINE}" stroke-width="1.3"/>')
        b.append(_t(cx + cw / 2, cy + ch + 18, name, 12.5, ACCENT, "bold", "middle"))
        mx, my = cx + cw / 2, cy + ch / 2
        if i == 0:      # pie
            import math
            fr = [0.34, 0.22, 0.16, 0.12, 0.16]
            a = -math.pi / 2
            for k, f in enumerate(fr):
                a2 = a + f * 2 * math.pi
                r = 55
                x1_, y1_ = mx + r * math.cos(a), my + r * math.sin(a)
                x2_, y2_ = mx + r * math.cos(a2), my + r * math.sin(a2)
                large = 1 if f > 0.5 else 0
                col = "#B9C1CC" if k == 4 else pal[k]
                b.append(f'<path d="M{mx},{my} L{x1_:.1f},{y1_:.1f} A{r},{r} 0 {large} 1 '
                         f'{x2_:.1f},{y2_:.1f} Z" fill="{col}" stroke="#FFFFFF" '
                         f'stroke-width="1.5"/>')
                a = a2
        elif i == 1:    # bars income / expenses
            base = cy + ch - 22
            for k in range(8):
                bx = cx + 30 + k * 30
                hi, he = 60 + (k * 13) % 30, 45 + (k * 17) % 35
                b.append(f'<rect x="{bx}" y="{base - hi}" width="11" height="{hi}" '
                         f'fill="{INCOME}"/>')
                b.append(f'<rect x="{bx + 12}" y="{base - he}" width="11" height="{he}" '
                         f'fill="{EXPENSE}"/>')
            b.append(f'<line x1="{cx + 22}" y1="{base}" x2="{cx + cw - 22}" y2="{base}" '
                     f'stroke="#9AA4B2"/>')
        elif i == 2:    # table
            for r in range(6):
                ry = cy + 22 + r * 19
                fill = ACCENT if r == 0 else ("#F4F7FB" if r % 2 else "#FFFFFF")
                b.append(f'<rect x="{cx + 20}" y="{ry}" width="{cw - 40}" height="19" '
                         f'fill="{fill}" stroke="#DDE3EA"/>')
                for c in range(4):
                    if r:
                        b.append(f'<rect x="{cx + 30 + c * 62}" y="{ry + 7}" width="40" '
                                 f'height="5" rx="2" fill="#B9C1CC"/>')
        elif i == 3:    # heatmap
            vals = [3, 1, 4, 2, 5, 3, 0, 2, 4, 5, 1, 3, 2, 4, 5, 3, 1, 2, 4, 3, 5, 2, 3, 4,
                    1, 3, 5, 4, 2, 3, 4, 2, 3, 5, 1, 4]
            cols = ["#C0392B", "#E8A39B", "#F4F7FB", "#B5DEC5", "#5DB585", "#23864F"]
            for k, v in enumerate(vals):
                r, c = divmod(k, 12)
                b.append(f'<rect x="{cx + 24 + c * 20.5}" y="{cy + 30 + r * 30}" width="18" '
                         f'height="27" rx="2" fill="{cols[v]}"/>')
        elif i == 4:    # tags : horizontal bars
            for k, f in enumerate([0.9, 0.62, 0.45, 0.3]):
                ry = cy + 26 + k * 28
                b.append(f'<rect x="{cx + 24}" y="{ry}" width="54" height="14" rx="3" '
                         f'fill="#E6EAF0"/>')
                b.append(f'<rect x="{cx + 86}" y="{ry}" width="{f * 175:.0f}" height="14" '
                         f'rx="3" fill="{pal[k]}"/>')
        else:           # compare CSD : lines
            for k, (yy, amp) in enumerate([(50, 8), (80, 14), (110, 6)]):
                pts = " ".join(f"{cx + 24 + j * 24},{cy + yy + (amp if j % 2 else -amp)}"
                               for j in range(11))
                b.append(f'<polyline points="{pts}" fill="none" stroke="{pal[k]}" '
                         f'stroke-width="2.2"/>')
    return _svg(w, h, "".join(b))


def analysis_thumb(L, i):
    """One card of analysis_thumbs(), cropped out of the full drawing."""
    import re as _re
    cw, ch, gap = 288, 150, 18
    cx = (i % 3) * (cw + gap)
    cy = (i // 3) * (ch + gap + 12)
    svg = analysis_thumbs(L)
    return _re.sub(r'width="\d+" height="\d+" viewBox="[^"]+"',
                   f'width="{cw}" height="{ch + 2}" viewBox="{cx} {cy} {cw} {ch + 2}"', svg, 1)
