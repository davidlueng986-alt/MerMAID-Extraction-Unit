#!/usr/bin/env python3
"""Split a serial log into CSV files by line type.

    python3 tools/log2csv.py run.log [outdir]

Lines may carry a PC-side timestamp prefix ("2026-10-20T10:00:00.123 D,...") which is kept as column 'pc_time'.
Writes <outdir>/<stem>_D.csv, _E.csv, _S.csv and _H.txt.  Column names follow spec 11 / 6.4 and 7.x;
fields not covered by a known header get generic names (f1, f2 ...).  Lines of other types (command replies)
and lines that do not parse are counted and reported, never dropped silently.
"""
import csv, os, re, sys

HEAD = {
    ("D", "WM"): ["ms", "state", "pwm", "run_s"],
    ("D", "SM"): ["ms", "state", "mass_g", "rpm", "motor_pwm", "temp_C"],
    ("D", "UV"): ["ms", "state", "mass_g", "pd_raw", "dose_rel", "t_ring_C", "t_tube_C", "relay", "il_ok"],
    ("S", "WM"): ["run", "t_run_s", "pwm", "start_ms", "end_ms", "result"],
    ("S", "SM"): ["run", "m_soil_g", "v_ml", "p1_g", "p2_g", "ls_err_pct", "rpm_mean", "t_ext_s", "contact_s", "pulled_g", "errors"],
    ("S", "UV"): ["run", "tube_id", "filtrate_g", "h2o2_g", "enzyme_g", "final_g", "dilution", "dose_rel", "t_uv_s", "pause_s", "t_max_C", "errors"],
    ("E", "ALL"): ["ms", "state", "event", "detail"],
}
TS = re.compile(r"^(\S+)\s+(?=[HDESA-Z],)")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    path = sys.argv[1]
    outdir = sys.argv[2] if len(sys.argv) > 2 else os.path.dirname(os.path.abspath(path))
    stem = os.path.splitext(os.path.basename(path))[0]
    os.makedirs(outdir, exist_ok=True)
    module, rows, headers, other = "ALL", {"D": [], "E": [], "S": []}, [], 0
    for raw in open(path, encoding="utf-8", errors="replace"):
        line = raw.strip()
        if not line:
            continue
        pc = ""
        m = TS.match(line)
        if m:
            pc, line = m.group(1), line[m.end():]
        f = line.split(",")
        t = f[0]
        if t == "H":
            headers.append(line)
            if len(f) > 1:
                module = f[1]
        elif t in rows:
            rows[t].append([pc] + f[1:])
        else:
            other += 1
    for t, rs in rows.items():
        if not rs:
            continue
        cols = HEAD.get((t, module)) or HEAD.get((t, "ALL")) or []
        width = max(len(r) - 1 for r in rs)
        names = ["pc_time"] + [cols[i] if i < len(cols) else f"f{i + 1}" for i in range(max(width, len(cols)))]
        with open(os.path.join(outdir, f"{stem}_{t}.csv"), "w", newline="", encoding="utf-8") as fh:
            w = csv.writer(fh)
            w.writerow(names)
            for r in rs:
                w.writerow(r + [""] * (len(names) - len(r)))
    if headers:
        open(os.path.join(outdir, f"{stem}_H.txt"), "w", encoding="utf-8").write("\n".join(headers) + "\n")
    print(f"{path}: module={module} D={len(rows['D'])} E={len(rows['E'])} S={len(rows['S'])} H={len(headers)} other={other}")


main()
