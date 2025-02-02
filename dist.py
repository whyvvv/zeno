#!/usr/bin/env python3

import sys
import subprocess
import shutil

if sys.platform == 'win32':
    os_name = 'win'
elif sys.platform == 'linux':
    os_name = 'linux'
elif sys.platform == 'darwin':
    os_name = 'macos'
else:
    raise AssertionError(sys.platform)

subprocess.check_call([sys.executable, '-m', 'PyInstaller', 'scripts/launcher_{}.spec'.format(os_name)] + sys.argv[1:])
shutil.make_archive('dist/launcher', 'zip', 'dist/launcher')
