"""Retain only this regression's logs and macOS crash reports on CI failure."""
from pathlib import Path
import shutil

root = Path(__file__).resolve().parents[1]
out = root / 'diagnostics'
out.mkdir(exist_ok=True)
log = root / 'build/Testing/Temporary/LastTest.log'
if log.is_file():
    shutil.copy(log, out)
crashes = Path.home() / 'Library/Logs/DiagnosticReports'
if crashes.is_dir():
    for report in crashes.glob('SonoCraftAudioRegression*.ips'):
        shutil.copy(report, out)
