#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest, threading

from pxr import Usd

class TestUsdSchemaRegistryThreadedInit(unittest.TestCase):
    def test_ThreadedInit(self):
        threads = []
        for i in range(2):
            thread = threading.Thread(target=lambda: Usd.SchemaRegistry())
            thread.start()
            threads.append(thread)

        for thread in threads:
            thread.join()

if __name__ == "__main__":
    unittest.main()
