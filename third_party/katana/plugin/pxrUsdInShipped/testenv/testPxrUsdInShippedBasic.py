#!/local_inst_subst/bin/pixkatana --script

from Mentor.Runtime import Assert, AssertEqual, AssertIn, AssertClose

from KatanaTests import KatanaFixture, KatanaRunner

'''
here's a pretty easy way to get testing.  In the python interpreter in katana:

opNode = NodegraphAPI.GetNode('PxrUsdIn')
opClient = Nodes3DAPI.CreateClient(opNode)
s = "/root/world/geo"
lc = opClient.cookLocation(s + "/World/Mesh")
attrs = lc.getAttrs()
print attrs.getChildByName('xform')

'''

class testPxrOpUsdInShippedBasic(KatanaFixture):
    def ClassSetup(self):
        self.LoadFile('basic.katana')

    def _GetLocationForOp(self, usdPath):
        p = '/root/world/geo' + usdPath
        return self.GetLocationAtNode(p, 'PxrUsdIn')

    def TestMesh(self):
        loc = self._GetLocationForOp('/World/Mesh')
        attrs = loc.getAttrs()

        # should default to subdmesh
        AssertEqual(attrs.getChildByName("type").getValue(), "subdmesh")

        Assert(attrs.getChildByName('bound'))
        Assert(attrs.getChildByName('xform'))
        AssertEqual(attrs.getChildByName("materialAssign").getValue(), 
                "/root/world/geo/World/Looks/PxrDisney5SG")

        AssertEqual(attrs.getChildByName("viewer.default.drawOptions.windingOrder").getValue(), 
                "clockwise")

        AssertEqual(attrs.getChildByName("prmanStatements.orientation").getValue(), 
                "outside")
        AssertEqual(attrs.getChildByName("prmanStatements.subdivisionMesh.scheme").getValue(), 
                "catmull-clark")

    def TestXform(self):
        loc = self._GetLocationForOp('/World/Xform')
        attrs = loc.getAttrs()
        Assert(attrs.getChildByName('xform.matrix'))

    def TestModel(self):
        loc = self._GetLocationForOp('/World/anim/Model')
        attrs = loc.getAttrs()
        AssertEqual(attrs.getChildByName('type').getValue(), 'component')
        AssertEqual(attrs.getChildByName('modelName').getValue(), 'Model')

        AssertEqual(attrs.getChildByName('prmanStatements.scopedCoordinateSystem').getValue(),
                "ModelSpace")

        # make sure we're bringing in the proxy for the model.
        Assert(attrs.getChildByName("proxies.viewer.load.opArgs.a.type").getValue(), "usd")

    def TestModelConstraints(self):
        loc = self._GetLocationForOp('/World/anim/Model')
        AssertIn("ConstraintTargets", loc.getPotentialChildren())
        loc = self._GetLocationForOp('/World/anim/Model/ConstraintTargets')
        AssertIn("RootXf", loc.getPotentialChildren())

    def TestCamera(self):
        loc = self._GetLocationForOp('/World/main_cam')
        attrs = loc.getAttrs()

        # make sure this got type camera
        AssertEqual(attrs.getChildByName('type').getValue(), 'camera')

        grp = attrs.getChildByName('geometry')

        AssertClose(grp.getChildByName('left').getValue(),       -0.89843750745)
        AssertClose(grp.getChildByName('right').getValue(),      +1.10156249255)
        AssertClose(grp.getChildByName('bottom').getValue(),     -0.52539059784)
        AssertClose(grp.getChildByName('top').getValue(),        +0.52539059784)
        AssertClose(grp.getChildByName('fov').getValue(),        60.3832435608)
        AssertClose(grp.getChildByName('near').getValue(),       20.0)
        AssertClose(grp.getChildByName('far').getValue(),      5000.0)

    def TestLook(self):
        # make sure there is nothing under
        # assert no
        loc = self._GetLocationForOp('/World/Looks/PxrDisney5SG')
        attrs = loc.getAttrs()

        AssertEqual(len(loc.getPotentialChildren()), 0)
        AssertEqual(attrs.getChildByName('material.nodes').getNumberOfChildren(), 3)

    def TestBlindData(self):
        loc = self._GetLocationForOp('/World/Looks/PxrDisney5SG')
        attrs = loc.getAttrs()
        Assert(attrs.getChildByName("material.pbsNetworkMaterialVersion"))

    def TestFaceSet(self):
        loc = self._GetLocationForOp('/World/FaceSet/Plane')
        attrs = loc.getAttrs()

        AssertIn("faceset_0", loc.getPotentialChildren())
        AssertIn("faceset_1", loc.getPotentialChildren())

        loc = self._GetLocationForOp('/World/FaceSet/Plane/faceset_0')
        attrs = loc.getAttrs()
        AssertEqual(attrs.getChildByName('materialAssign').getValue(), 
                "/root/world/geo/World/FaceSet/Looks/initialShadingGroup")

    def TestSkippedBindingToLookNotUnderLooks(self):
        loc = self._GetLocationForOp('/World/Xform')
        attrs = loc.getAttrs()
        Assert(attrs.getChildByName("materialAssign") is None)

if __name__ == '__main__':
    KatanaRunner(locals()).Main()

