"""Verify the actual download, including a macOS signature after ZIP extraction."""
import argparse
from pathlib import Path
import os
import plistlib
import struct
import subprocess
import tempfile
import zipfile


def verify(archive, platform):
    folder = 'SonoCraft-' + platform
    with zipfile.ZipFile(archive) as zipped:
        bad = zipped.testzip()
        if bad:
            raise ValueError('Corrupt ZIP member: ' + bad)
        names = zipped.namelist()
        if len(names) != len(set(names)):
            raise ValueError('Duplicate ZIP members')
        for name in names:
            if Path(name).is_absolute() or '..' in Path(name).parts:
                raise ValueError('Invalid ZIP path: ' + name)
        for name in ['LICENSE', 'LICENSE_EXCEPTION', 'SONOCRAFT.md', 'COMO-USAR-FONTES.md']:
            if not zipped.read(folder + '/' + name).strip():
                raise ValueError('Empty documentation: ' + name)
        if platform == 'windows-x64':
            data = zipped.read(folder + '/SonoCraft.exe')
            if len(data) < 64 or data[:2] != b'MZ':
                raise ValueError('Missing Windows executable header')
            offset = struct.unpack_from('<I', data, 0x3c)[0]
            if len(data) < offset + 6 or data[offset:offset + 4] != b'PE\0\0':
                raise ValueError('Invalid Windows executable')
            if struct.unpack_from('<H', data, offset + 4)[0] != 0x8664:
                raise ValueError('Windows executable is not x64')
            return
    with tempfile.TemporaryDirectory(prefix='sonocraft-package-') as temp:
        subprocess.run(['ditto', '-x', '-k', str(archive), temp], check=True)
        app = Path(temp) / folder / 'SonoCraft.app'
        executable = app / 'Contents/MacOS/SonoCraft'
        if not executable.is_file() or not os.access(executable, os.X_OK):
            raise ValueError('macOS executable missing or not executable after extraction')
        with (app / 'Contents/Info.plist').open('rb') as stream:
            info = plistlib.load(stream)
        if info.get('CFBundleIdentifier') != 'org.sonocraft.audio':
            raise ValueError('Unexpected app identifier')
        if not info.get('CFBundleVersion') or not info.get('NSMicrophoneUsageDescription'):
            raise ValueError('Missing app version or microphone usage description')
        subprocess.run(['lipo', str(executable), '-verify_arch', 'arm64', 'x86_64'], check=True)
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(app)], check=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--platform', choices=['mac-universal', 'windows-x64'], required=True)
    args = parser.parse_args()
    archive = Path(__file__).resolve().parents[1] / 'dist' / ('SonoCraft-' + args.platform + '.zip')
    verify(archive, args.platform)
    print('Verified download:', archive.name)
