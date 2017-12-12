#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

import sys, unittest
from pxr import Sdf, Tf

class TestSdfLayer(unittest.TestCase):
    def test_IdentifierWithArgs(self):
        paths = [
            ("foo.sdf", 
             "foo.sdf", 
             {}),
            ("foo.sdf1!@#$%^*()-_=+[{]}|;:',<.>", 
             "foo.sdf1!@#$%^*()-_=+[{]}|;:',<.>", 
             {}),
            ("foo.sdf:SDF_FORMAT_ARGS:a=b&c=d", 
             "foo.sdf",
             {"a":"b", "c":"d"}),
            ("foo.sdf?otherargs&evenmoreargs:SDF_FORMAT_ARGS:a=b&c=d", 
             "foo.sdf?otherargs&evenmoreargs",
             {"a":"b", "c":"d"}),
        ]
        
        for (identifier, path, args) in paths:
            splitPath, splitArgs = Sdf.Layer.SplitIdentifier(identifier)
            self.assertEqual(path, splitPath)
            self.assertEqual(args, splitArgs)

            joinedIdentifier = Sdf.Layer.CreateIdentifier(splitPath, splitArgs)
            self.assertEqual(identifier, joinedIdentifier)

if __name__ == "__main__":
    unittest.main()
