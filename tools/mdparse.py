"""Minimal parser for the pipe tables in docs/source/*.md.

parse_tables(path) -> list of dicts: {"h2", "h3", "header", "rows"} where rows is a
list of dicts keyed by header text. Markdown escapes (\\_ , **bold**) are removed.
"""
import re


def clean(cell):
    cell = cell.strip().replace("\\_", "_").replace("\\|", "|")
    return re.sub(r"\*\*(.+?)\*\*", r"\1", cell)


def split_row(line):
    parts = re.split(r"(?<!\\)\|", line.strip())
    return [clean(p) for p in parts[1:-1]]


def parse_tables(path):
    lines = open(path, encoding="utf-8").read().split("\n")
    h2 = h3 = ""
    out = []
    i = 0
    while i < len(lines):
        ln = lines[i]
        if ln.startswith("## "):
            h2, h3 = ln[3:].strip(), ""
        elif ln.startswith("### "):
            h3 = ln[4:].strip()
        elif ln.startswith("|") and i + 1 < len(lines) and re.match(r"^\|\s*---", lines[i + 1]):
            header = split_row(ln)
            rows = []
            i += 2
            while i < len(lines) and lines[i].startswith("|"):
                cells = split_row(lines[i])
                if len(cells) == len(header):
                    rows.append(dict(zip(header, cells)))
                i += 1
            out.append({"h2": h2, "h3": h3, "header": header, "rows": rows})
            continue
        i += 1
    return out


def parse_range(s):
    """'40–60' -> (40.0, 60.0); '1,190' -> (1190, 1190); '0' -> (0,0); '' -> None"""
    s = s.replace(",", "").replace(" ", "")
    if not s or s in ("—", "-"):
        return None
    m = re.fullmatch(r"([\d.]+)(?:[–-]([\d.]+))?", s)
    if not m:
        return None
    lo = float(m.group(1))
    hi = float(m.group(2)) if m.group(2) else lo
    return lo, hi
