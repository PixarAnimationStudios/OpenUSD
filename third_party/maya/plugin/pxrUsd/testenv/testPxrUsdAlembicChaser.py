#!/pxrpythonsubst

# Initialize maya.
import pixrun
pixrun.setupEnv()
from pixar import pxMayaVersion
pxMayaVersion.setup(version='mayaversion_subst')
import maya.standalone
maya.standalone.initialize('pypix')

import os

from pixar import Usd, UsdGeom

from Mentor.Runtime import Assert
from Mentor.Runtime import AssertEqual
from Mentor.Runtime import FindDataFile
from Mentor.Runtime import Fixture
from Mentor.Runtime import Runner


class testUserExportedAttributes(Fixture):

    def TestExportAttributes(self):
        from maya import cmds
        cmds.file(FindDataFile('AlembicChaser.ma'), open=True, force=True)

        usdFilePath = os.path.abspath('out.usda')

        # alembic chaser is in the pxrUsd plugin.
        cmds.loadPlugin('pxrUsd')

        # Export to USD.
        cmds.usdExport(
            file=usdFilePath,
            chaser=['alembic'],
            chaserArgs=[
                ('alembic', 'attrprefix', 'ABC_'),
            ])

        stage = Usd.Stage.Open(usdFilePath)
        Assert(stage)

        prim = stage.GetPrimAtPath('/Foo/Plane')
        Assert(prim)

        attr = prim.GetAttribute('ABC_test')
        Assert(attr)
        AssertEqual(attr.Get(), 'success')

        subd = UsdGeom.Mesh.Get(stage, '/Foo/sphere_subd')
        AssertEqual(subd.GetSubdivisionSchemeAttr().Get(), 'catmullClark')
        poly = UsdGeom.Mesh.Get(stage, '/Foo/sphere_poly')
        AssertEqual(poly.GetSubdivisionSchemeAttr().Get(), 'none')

if __name__ == '__main__':
    Runner().Main()
