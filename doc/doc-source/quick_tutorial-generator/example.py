"""Worked example : events and daily balance, computed like GBP (end-of-day balance)."""
import calendar
from datetime import date

TODAY = date(2026, 10, 28)
END = date(2027, 10, 31)
START_AMOUNT = 800

# key, income ?, amount, kind, rhythm, first date
CSDS = [
    ("salary", True, 3200, "monthly", date(2026, 11, 15)),
    ("rent", False, 1450, "monthly", date(2026, 11, 1)),
    ("groceries", False, 650, "eom", date(2026, 11, 30)),
    ("insurance", False, 1100, "yearly", date(2027, 1, 10)),
    ("repair", False, 900, "once", date(2027, 3, 14)),
    ("vacation", False, 3000, "once", date(2027, 7, 5)),
]


def _add_months(d, n):
    y, m = divmod(d.month - 1 + n, 12)
    y += d.year
    m += 1
    return date(y, m, min(d.day, calendar.monthrange(y, m)[1]))


def events():
    ev = []
    for key, inc, amt, kind, first in CSDS:
        sign = 1 if inc else -1
        if kind == "once":
            dates = [first]
        elif kind == "yearly":
            dates = [first.replace(year=first.year + k) for k in range(3)]
        elif kind == "eom":
            dates = []
            d = first
            for k in range(14):
                x = _add_months(date(first.year, first.month, 1), k)
                dates.append(date(x.year, x.month, calendar.monthrange(x.year, x.month)[1]))
        else:
            dates = [_add_months(first, k) for k in range(14)]
        ev += [(d, key, sign * amt) for d in dates if TODAY < d <= END]
    return sorted(ev)


def balances():
    """[(date, end-of-day balance, [(key, amount)...])] for each eventful day."""
    out, bal, by_day = [], START_AMOUNT, {}
    for d, key, a in events():
        by_day.setdefault(d, []).append((key, a))
    for d in sorted(by_day):
        bal += sum(a for _, a in by_day[d])
        out.append((d, bal, by_day[d]))
    return out


if __name__ == "__main__":
    for d, b, e in balances():
        print(d, b, e)
