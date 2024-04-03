#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
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

