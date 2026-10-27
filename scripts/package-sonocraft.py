"""Package a locally built SonoCraft application with its license and limitations."""
import argparse
import pathlib
import shutil
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--platform', choices=['mac-universal', 'windows-x64'], required=True)
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
dist = root / 'dist'
stage = dist / ('SonoCraft-' + args.platform)
if stage.exists():
    shutil.rmtree(stage)
stage.mkdir(parents=True)
artefacts = root / 'build' / 'SonoBus_artefacts' / 'Release' / 'Standalone'
if args.platform == 'mac-universal':
    source = artefacts / 'SonoCraft.app'
    destination = stage / source.name
    shutil.copytree(source, destination, copy_function=shutil.copy)
    subprocess.run(['lipo', '-verify_arch', 'arm64', 'x86_64', str(destination / 'Contents/MacOS/SonoCraft')], check=True)
    subprocess.run(['codesign', '--force', '--deep', '--sign', '-', str(destination)], check=True)
    subprocess.run(['codesign', '--verify', '--deep', '--strict', str(destination)], check=True)
else:
    shutil.copy(artefacts / 'SonoCraft.exe', stage)
for filename in ['LICENSE', 'LICENSE_EXCEPTION', 'SONOCRAFT.md']:
    shutil.copy(root / filename, stage)
if args.platform == 'mac-universal':
    subprocess.run(['ditto', '-c', '-k', '--keepParent', str(stage), str(dist / (stage.name + '.zip'))], check=True)
else:
    shutil.make_archive(str(dist / stage.name), 'zip', root_dir=dist, base_dir=stage.name)
print('Packaged:', dist / (stage.name + '.zip'))
