#!/pxrpythonsubst

# Copyright, Pixar Animation Studios 2015. All rights reserved.

from pxr import Sdf, UsdUtils
from Mentor.Runtime import * 


SetAssertMode(MTR_EXIT_TEST)

class TestUsdStitchBasic(Fixture):
    def ClassSetup(self):
        self.layers = [Sdf.Layer.CreateAnonymous(), 
                       Sdf.Layer.FindOrOpen(FindDataFile('src/Particles_'\
                                                         'Splash.101.usd')),
                       Sdf.Layer.FindOrOpen(FindDataFile('src/Particles_'\
                                                         'Splash.103.usd')),
                       Sdf.Layer.CreateAnonymous(),
                       Sdf.Layer.FindOrOpen(FindDataFile('src/letters_perFrame'\
                                                         '.101.usda')),
                       Sdf.Layer.FindOrOpen(FindDataFile('src/letters_perFrame'\
                                                         '.102.usda')),
                       Sdf.Layer.FindOrOpen(FindDataFile('src/letters_perFrame'\
                                                         '.103.usda'))]

        self.rootprims_combined = None
        self.rootprims_a = None
        self.rootprims_b = None

    def Setup(self):
        # stitch layers for basic tests
        UsdUtils.StitchLayers(self.layers[0], self.layers[1])
        UsdUtils.StitchLayers(self.layers[0], self.layers[2])

        self.rootprims_combined = self.layers[0].rootPrims
        self.rootprims_a = self.layers[1].rootPrims
        self.rootprims_b = self.layers[2].rootPrims

        # stitch layers for secondary tests
        UsdUtils.StitchLayers(self.layers[3], self.layers[4])
        UsdUtils.StitchLayers(self.layers[3], self.layers[5])
        UsdUtils.StitchLayers(self.layers[3], self.layers[6])

    def TestAllPrimsFullyCopied(self):
        for prims in [self.rootprims_a, self.rootprims_b]:
            for rootprim in prims:
                for child in rootprim.nameChildren:
                    AssertTrue(self.layers[0].GetPrimAtPath(child.path),
                        'Missing object at ' + str(child.path))
                for attr in rootprim.attributes:
                    AssertTrue(self.layers[0].GetAttributeAtPath(attr.path),
                        'Missing attribute at ' + str(attr.path))
                for rel in rootprim.relationships:
                    Assert(self.layers[0].GetRelationshipAtPath(rel.path),
                        'Missing relationship at ' + str(rel.path))
         
    def TestAllTimeSamplesMerged(self):
        timesamples_a = self.layers[1].ListAllTimeSamples()
        timesamples_b = self.layers[2].ListAllTimeSamples()

        # the composed layer should have the union of time samples
        AssertTrue(len(self.layers[0].ListAllTimeSamples()) ==
                   len(set().union(*[timesamples_a, timesamples_b])))

    def TestFrameInfoMerged(self):
        hasFps = self.layers[1].framesPerSecond != None or \
                 self.layers[2].framesPerSecond != None
        hasFramePrec = self.layers[1].framePrecision != None or \
                       self.layers[2].framePrecision != None
        hasEndTimeCode = self.layers[1].endTimeCode != None or \
                         self.layers[2].endTimeCode != None
        hasStartTimeCode = self.layers[1].startTimeCode != None or \
                           self.layers[2].startTimeCode != None
        
        AssertTrue(((hasFps and self.layers[0].framesPerSecond != None) or \
                    (not hasFps)), 'Missing fps')
        AssertTrue((hasFramePrec and self.layers[0].framePrecision != None) or \
                    (not hasFramePrec), 'Missing fps')
        AssertTrue(((hasStartTimeCode and self.layers[0].startTimeCode != None) or \
                    (not hasStartTimeCode)), 'Missing fps')
        AssertTrue(((hasEndTimeCode and self.layers[0].endTimeCode != None) or \
                    (not hasEndTimeCode)), 'Missing fps')

    def TestValidFileCreation(self):
        import tempfile
        with tempfile.NamedTemporaryFile(suffix='usda') as tempUsd:
            AssertTrue(self.layers[0].Export(tempUsd.name))

    # see bug: 119134 for further details
    # previously, usdstitch would attempt to copy 
    # targetPaths on a relationship via SetInfo
    # which causes a coding error because targetPaths
    # are read only. They are now updated through the
    # target path proxy API.
    def TestTargetPathCopy(self):
        primPath = '/World/fx/Letters/points'
        primWithTargets = self.layers[3].GetPrimAtPath(primPath) 
        relationshipInPrim = primWithTargets.relationships[0]
        assert relationshipInPrim, 'Relationship not stitched'
        addedPaths = relationshipInPrim.targetPathList.addedItems
        assert len(addedPaths) == 3, 'Paths missing in relationship'

if __name__ == '__main__':
    Runner().Main()
