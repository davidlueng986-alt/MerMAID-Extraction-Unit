import csv, os, subprocess, sys, tempfile
here = os.path.dirname(os.path.abspath(__file__))
log = """H,WM,fw=3f2a91c,t_run_s=120,pwm=128
2026-10-20T10:00:00.1 E,1532,IDLE,START,
D,2532,RUN,128,1
D,3532,RUN,128,2
E,121540,RUN,DONE,
S,1,120.0,128,1532,121540,OK
ID,WM,3f2a91c
"""
with tempfile.TemporaryDirectory() as d:
    p = os.path.join(d, "run.log")
    open(p, "w").write(log)
    r = subprocess.run([sys.executable, os.path.join(here, "log2csv.py"), p, d], capture_output=True, text=True, check=True)
    assert "D=2 E=2 S=1 H=1 other=1" in r.stdout, r.stdout
    s = list(csv.DictReader(open(os.path.join(d, "run_S.csv"))))
    assert s[0]["result"] == "OK" and s[0]["end_ms"] == "121540", s
    e = list(csv.DictReader(open(os.path.join(d, "run_E.csv"))))
    assert e[0]["pc_time"] == "2026-10-20T10:00:00.1" and e[0]["event"] == "START", e
print("log2csv test passed")
