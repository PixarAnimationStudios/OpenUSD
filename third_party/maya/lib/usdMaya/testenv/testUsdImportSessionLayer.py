#!/pxrpythonsubst

# Initialize maya.
import pixrun
pixrun.setupEnv()
from pixar import pxMayaVersion
pxMayaVersion.setup(version='mayaversion_subst')
import maya.standalone
maya.standalone.initialize('pypix')

from maya import cmds

from pxr import Usd

from Mentor.Runtime import Assert
from Mentor.Runtime import AssertEqual
from Mentor.Runtime import FindDataFile
from Mentor.Runtime import Fixture
from Mentor.Runtime import Runner


class testUsdImportSessionLayer(Fixture):

    def TestUsdImport(self):
        """
        This tests that executing a usdImport with variants specified causes
        those variant selections to be made in a session layer and not affect
        other open stages.
        """
        usdFile = FindDataFile('Cubes.usda')

        # Open the asset USD file and make sure the default variant is what we
        # expect it to be.
        stage = Usd.Stage.Open(usdFile)
        Assert(stage)

        modelPrimPath = '/Cubes'
        modelPrim = stage.GetPrimAtPath(modelPrimPath)
        Assert(modelPrim)

        variantSet = modelPrim.GetVariantSet('modelingVariant')
        variantSelection = variantSet.GetVariantSelection()

        # This is the default variant.
        AssertEqual(variantSelection, 'OneCube')

        # Now do a usdImport of a different variant in a clean Maya scene.
        cmds.file(new=True, force=True)
        cmds.loadPlugin('pxrUsd')

        variants = [('modelingVariant', 'ThreeCubes')]
        cmds.usdImport(file=usdFile, primPath=modelPrimPath, variant=variants)

        expectedMayaCubeNodesSet = set([
            '|Cubes|Geom|CubeOne',
            '|Cubes|Geom|CubeTwo',
            '|Cubes|Geom|CubeThree'])
        mayaCubeNodesSet = set(cmds.ls('|Cubes|Geom|Cube*', long=True))
        AssertEqual(expectedMayaCubeNodesSet, mayaCubeNodesSet)

        # The import should have made the variant selections in a session layer,
        # so make sure the selection in our open USD stage was not changed.
        variantSet = modelPrim.GetVariantSet('modelingVariant')
        variantSelection = variantSet.GetVariantSelection()
        AssertEqual(variantSelection, 'OneCube')


if __name__ == '__main__':
    Runner().Main()
