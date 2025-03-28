#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function

import os
import time
import unittest

from pxr import Tf

class TestFileUtils(unittest.TestCase):
    """
    Test Tf File Utils (The python wrapped porting of the utility functions).
    """
    def test_Touch(self):
        """Testing Touch() function"""
        try:
            print("Touch non-existent file")
            self.assertFalse(Tf.TouchFile("touchFile", False))
            self.assertFalse(os.path.isfile("touchFile"))

            print("Touch non-existent file with create flag")
            self.assertTrue(Tf.TouchFile("touchFile", True))
            self.assertTrue(os.path.isfile("touchFile"))

            print("Test if touch modifies existing file")
            quote  = "Fish are friends, not food.\n"
            with open("touchFile","wt") as file:
                file.write(quote)
            Tf.TouchFile("touchFile", True)
            self.assertTrue(open("touchFile").read() == quote)

            print("Test if touch updates the mod time")
            
            st = os.stat("touchFile")
            oldTime = st.st_mtime
    
            time.sleep(1)
    
            self.assertTrue(Tf.TouchFile("touchFile", False))
    
            st = os.stat("touchFile")
            newTime = st.st_mtime
    
            # Mod time should have been updated by the Has() call.
            self.assertTrue(newTime > oldTime)

        finally:
            if os.path.isfile("touchFile"):
                os.remove("touchFile")

if __name__ == '__main__':
    unittest.main()

