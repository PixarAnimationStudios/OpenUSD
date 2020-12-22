import os
import platform
import sys

if platform.system() == 'Windows':
    """
    Since python 3.8 we need to explicitely set
    the dll search path on windows
    """
    major = sys.version_info[0]
    minor = sys.version_info[1]
    if major > 3 or (major == 3 and minor >= 8):
        base_dir = os.path.join(os.path.dirname(__file__), '..', '..', '..')
        bin_dir = os.path.join(base_dir, 'bin')
        lib_dir = os.path.join(base_dir, 'lib')

        os.add_dll_directory(bin_dir)
        os.add_dll_directory(lib_dir)

