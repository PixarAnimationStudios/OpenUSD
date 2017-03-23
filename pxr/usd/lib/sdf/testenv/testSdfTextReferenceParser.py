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

import time, unittest, sys
from pxr import Sdf

class TestSdfTextReferenceParser(unittest.TestCase):
    def presult(self, fileName, refType, refs):
        if refs:
            for i,r in enumerate(refs):
                sys.stdout.write('%s %s[%03d]: %s\n' % (fileName, refType, i+1,r))
        else:
            sys.stdout.write('%s no %s\n' % (fileName, refType))

    def ParseFileWithFunc(self, ctx, parseFunc):
        t0 = time.time()
        sublayers, references, payloads = parseFunc()
        sys.stderr.write('== END: {0} {1}\n'.format(ctx, time.time() - t0))

        self.presult(ctx, 'sublayers', sublayers)
        self.presult(ctx, 'references', references)
        self.presult(ctx, 'payloads', payloads)
        sys.stdout.write('-'*80+'\n')
        return sublayers, references, payloads

    def ParseFile(self, fileName):
        sys.stderr.write('== BEGIN: {0} (as file)\n'.format(fileName))
        return self.ParseFileWithFunc(fileName,
            lambda: Sdf.ExtractExternalReferences(fileName))

    def ParseLayer(self, fileName):
        sys.stderr.write('== BEGIN: {0} (as string)\n'.format(fileName))
        layerData = Sdf.Layer.FindOrOpen(fileName).ExportToString()
        return self.ParseFileWithFunc(fileName,
            lambda: Sdf.ExtractExternalReferencesFromString(layerData))

    def test_Parsing(self):
        for layerName in ('test',):
            layerFile = layerName + '.sdf'
            self.assertEqual(self.ParseFile(layerFile), self.ParseLayer(layerFile))

if __name__ == '__main__':
    unittest.main()
