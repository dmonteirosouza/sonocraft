"""Run CTest and retain a stack sample if the macOS regression stalls."""
import pathlib
import subprocess
import sys

root = pathlib.Path(__file__).resolve().parents[1]
process = subprocess.Popen(['ctest', '--test-dir', 'build', '-C', 'Release', '--output-on-failure'], cwd=root)
try:
    result = process.wait(timeout=90)
except subprocess.TimeoutExpired:
    if sys.platform == 'darwin':
        out = root / 'diagnostics'
        out.mkdir(exist_ok=True)
        children = subprocess.run(['pgrep', '-P', str(process.pid)], capture_output=True, text=True)
        for child in children.stdout.split():
            name = subprocess.run(['ps', '-p', child, '-o', 'comm='], capture_output=True, text=True).stdout.strip()
            if pathlib.Path(name).name == 'SonoCraftAudioRegression':
                subprocess.run(['sample', child, '3', '-file', str(out / 'audio-regression-stack.txt')], timeout=15, check=False)
    try:
        # CTest has separate bounds for dialog (20 s), audio (240 s), and
        # resampling (180 s). Allow their sequential run and diagnostic sampling.
        result = process.wait(timeout=390)
    except subprocess.TimeoutExpired:
        process.terminate()
        process.wait(timeout=10)
        result = 1
sys.exit(result)
