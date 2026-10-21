"""PlatformIO pre-script: put the git commit (and '-dirty') into FW_COMMIT so the H line records it."""
import os, subprocess
Import("env")  # noqa: F821
try:
    c = subprocess.check_output(["git", "rev-parse", "--short=7", "HEAD"], text=True).strip()
    if subprocess.check_output(["git", "status", "--porcelain", "firmware"], text=True).strip():
        c += "-dirty"
except Exception:
    c = "unknown"
os.environ.setdefault("FW_COMMIT", c)
env.Append(CPPDEFINES=[("FW_COMMIT", env.StringifyMacro(os.environ["FW_COMMIT"]))])  # noqa: F821
