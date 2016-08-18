#!/pxrpythonsubst

import time
from pxr import Sdf
from Mentor.Runtime import Runner, Fixture, AssertEqual

class TestSdfTextReferenceParser(Fixture):

    def ClassSetup(self):
        self._rlog = open('result.txt', 'w')

    def rlog(self, m):
        print >>self._rlog, m

    def presult(self, fileName, refType, refs):
        if refs:
            for i,r in enumerate(refs):
                self.rlog('%s %s[%03d]: %s' % (fileName, refType, i+1, r))
        else:
            self.rlog('%s no %s' % (fileName, refType))

    def ParseFileWithFunc(self, ctx, parseFunc):
        t0 = time.time()
        sublayers, references, payloads = parseFunc()
        self.log.info('== END: {0} {1}'.format(ctx, time.time() - t0))

        self.presult(ctx, 'sublayers', sublayers)
        self.presult(ctx, 'references', references)
        self.presult(ctx, 'payloads', payloads)
        self.rlog('-'*80)
        return sublayers, references, payloads

    def ParseFile(self, fileName):
        self.log.info('== BEGIN: {0} (as file)'.format(fileName))
        return self.ParseFileWithFunc(fileName,
            lambda: Sdf.ExtractExternalReferences(fileName))

    def ParseLayer(self, fileName):
        self.log.info('== BEGIN: {0} (as string)'.format(fileName))
        layerData = Sdf.Layer.FindOrOpen(fileName).ExportToString()
        return self.ParseFileWithFunc(fileName,
            lambda: Sdf.ExtractExternalReferencesFromString(layerData))

    def TestParsing(self):
        for layerName in ('test',):
            layerFile = layerName + '.sdf'
            AssertEqual(self.ParseFile(layerFile), self.ParseLayer(layerFile))

if __name__ == '__main__':
    Runner().Main()
