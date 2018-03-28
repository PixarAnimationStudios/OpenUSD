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

from pxr import Usd, Vt, Sdf, Tf
import unittest

stage = Usd.Stage.Open("./Test.usda")
testPrim = stage.GetPrimAtPath("/CollectionTest")

geom = stage.GetPrimAtPath("/CollectionTest/Geom")
box = stage.GetPrimAtPath("/CollectionTest/Geom/Box")

shapes = stage.GetPrimAtPath("/CollectionTest/Geom/Shapes")
sphere = stage.GetPrimAtPath("/CollectionTest/Geom/Shapes/Sphere")
hemiSphere1 = stage.GetPrimAtPath("/CollectionTest/Geom/Shapes/Sphere/Hemisphere1")
hemiSphere2 = stage.GetPrimAtPath("/CollectionTest/Geom/Shapes/Sphere/Hemisphere2")
cube = stage.GetPrimAtPath("/CollectionTest/Geom/Shapes/Cube")
cylinder = stage.GetPrimAtPath("/CollectionTest/Geom/Shapes/Cylinder")
cone = stage.GetPrimAtPath("/CollectionTest/Geom/Shapes/Cone")

def _DebugCollection(collection):
    print "Debugging Collection: ", collection.GetName()
    mquery = collection.ComputeMembershipQuery()
    print "-- Included Objects -- "
    incObjects = Usd.CollectionAPI.ComputeIncludedObjects(mquery, stage)
    for obj in incObjects: print ".. ", obj.GetPath() 

class TestUsdCollectionAPI(unittest.TestCase):
    def test_AuthorCollections(self):
        # ----------------------------------------------------------
        # Test an explicitOnly collection.
        explicitColl = Usd.CollectionAPI.ApplyCollection(testPrim, 
                "test:Explicit:Collection", Usd.Tokens.explicitOnly)
        self.assertTrue(explicitColl.HasNoIncludedPaths())
        self.assertEqual(['CollectionAPI:test:Explicit:Collection'],
                         testPrim.GetAppliedSchemas())
        self.assertTrue(testPrim.HasAPI(Usd.CollectionAPI))
        self.assertTrue(testPrim.HasAPI(Usd.CollectionAPI, 
            instanceName="test:Explicit:Collection"))
        self.assertTrue(not testPrim.HasAPI(Usd.CollectionAPI, 
            instanceName="unknown"))
        self.assertTrue(not testPrim.HasAPI(Usd.CollectionAPI, 
            instanceName="test"))

        explicitColl.CreateIncludesRel().AddTarget(sphere.GetPath())
        self.assertFalse(explicitColl.HasNoIncludedPaths())

        explicitColl.GetIncludesRel().AddTarget(cube.GetPath())
        explicitColl.GetIncludesRel().AddTarget(cylinder.GetPath())
        explicitColl.GetIncludesRel().AddTarget(cone.GetPath())

        explicitCollMquery = explicitColl.ComputeMembershipQuery()

        explicitCollIncObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                explicitCollMquery, stage)
        self.assertEqual(len(explicitCollIncObjects), 4)

        for obj in explicitCollIncObjects:
            self.assertTrue(explicitCollMquery.IsPathIncluded(obj.GetPath()))

        # Ensure that descendants of explicitly included objects aren't 
        # included in the collection.
        self.assertFalse(explicitCollMquery.IsPathIncluded(hemiSphere1.GetPath()))
        self.assertFalse(explicitCollMquery.IsPathIncluded(hemiSphere2.GetPath()))

        # An explicitly included object can be explicitly excluded from the 
        # collection. i.e. excludes is stronger than includes.
        explicitColl.CreateExcludesRel().AddTarget(cone.GetPath())

        # We have to recompute the membership map if we add or remove 
        # includes/excludes targets.
        explicitCollMquery = explicitColl.ComputeMembershipQuery()

        # Ensure that the cone is excluded.
        self.assertFalse(explicitCollMquery.IsPathIncluded(cone.GetPath()))

        # ----------------------------------------------------------
        # Test an expandPrims collection.
        expandPrimsColl = Usd.CollectionAPI.ApplyCollection(testPrim, 
                "testExpandPrimsColl", Usd.Tokens.expandPrims)
        expandPrimsColl.CreateIncludesRel().AddTarget(geom.GetPath())
        expandPrimsCollMquery = expandPrimsColl.ComputeMembershipQuery()
        
        expandPrimCollIncObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                expandPrimsCollMquery, stage)
        self.assertEqual(len(expandPrimCollIncObjects), 9)

        
        for obj in expandPrimCollIncObjects:
            self.assertTrue(expandPrimsCollMquery.IsPathIncluded(obj.GetPath()))

        # Exclude all shapes from the collection. This leaves just the instanced 
        # box behind.
        expandPrimsColl.CreateExcludesRel().AddTarget(shapes.GetPath())

        # Verify that there's no harm in excluding a path that isn't 
        # included.
        expandPrimsColl.GetExcludesRel().AddTarget(
            Sdf.Path("/Collection/Materials/Plastic"))

        expandPrimsCollMquery = expandPrimsColl.ComputeMembershipQuery()
        expandPrimCollIncObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                expandPrimsCollMquery, stage, Usd.TraverseInstanceProxies())
        self.assertEqual(len(expandPrimCollIncObjects), 4)

        # ----------------------------------------------------------
        # Test an expandPrimsAndProperties collection.
        expandPrimsAndPropertiesColl = Usd.CollectionAPI.ApplyCollection(
                testPrim, 
                "testExpandPrimsAndPropertiesColl",
                Usd.Tokens.expandPrimsAndProperties)
        expandPrimsAndPropertiesColl.CreateIncludesRel().AddTarget(
                shapes.GetPath())
        expandPnPCollMquery = expandPrimsAndPropertiesColl.ComputeMembershipQuery()
        expandPnPCollObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                expandPnPCollMquery, stage)

        self.assertEqual(len(expandPnPCollObjects), 18)
        for obj in expandPnPCollObjects:
            self.assertTrue(expandPnPCollMquery.IsPathIncluded(obj.GetPath()))

        # ----------------------------------------------------------
        # Test a collection that includes other collections. 
        # 
        # Create a collection that combines the explicit collection and 
        # the expandPrimsAndProperties collection.
        combinedColl = Usd.CollectionAPI.ApplyCollection(testPrim, "combined", 
                Usd.Tokens.explicitOnly)
        combinedColl.CreateIncludesRel().AddTarget(
            expandPrimsAndPropertiesColl.GetCollectionPath())
        combinedColl.CreateIncludesRel().AddTarget(
            explicitColl.GetCollectionPath())

        combinedMquery = combinedColl.ComputeMembershipQuery()

        combinedCollIncObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                combinedMquery, stage)

        for obj in combinedCollIncObjects:
            self.assertTrue(combinedMquery.IsPathIncluded(obj.GetPath()))

        self.assertEqual(len(combinedCollIncObjects), 15)

        # now add the collection "expandPrimsColl", which includes "Geom" and 
        # exludes "Shapes", but is weaker than the "expandPrimsAndProperties" 
        # collection.
        combinedColl.CreateIncludesRel().AddTarget(
            expandPrimsColl.GetCollectionPath(), position=Usd.ListPositionBackOfAppendList)
        combinedMquery = combinedColl.ComputeMembershipQuery()
        
        combinedCollIncObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                combinedMquery, stage)

        for obj in combinedCollIncObjects:
            self.assertTrue(combinedMquery.IsPathIncluded(obj.GetPath()))
        self.assertEqual(len(combinedCollIncObjects), 5)

    def test_testIncludeAndExcludePath(self):
        geomCollection = Usd.CollectionAPI.ApplyCollection(geom, 
            "geom")
        self.assertTrue(geomCollection.IncludePath(shapes.GetPath()))
        self.assertTrue(geomCollection.ExcludePath(sphere.GetPath()))

        query = geomCollection.ComputeMembershipQuery()
        self.assertTrue(query.IsPathIncluded(cylinder.GetPath()))
        self.assertTrue(query.IsPathIncluded(cube.GetPath()))
        self.assertFalse(query.IsPathIncluded(sphere.GetPath()))
        self.assertFalse(query.IsPathIncluded(hemiSphere1.GetPath()))
        self.assertFalse(query.IsPathIncluded(hemiSphere2.GetPath()))

        # Add just hemiSphere2
        self.assertTrue(geomCollection.IncludePath(hemiSphere2.GetPath()))

        # Remove hemiSphere1. Note that this does nothing however since it's 
        # not included in the collection.
        self.assertTrue(geomCollection.ExcludePath(hemiSphere1.GetPath()))

        # Every time we call IncludePath() or ExcludePath(), we must recompute 
        # the MembershipQuery object.
        query = geomCollection.ComputeMembershipQuery()
        self.assertFalse(query.IsPathIncluded(sphere.GetPath()))
        self.assertFalse(query.IsPathIncluded(hemiSphere1.GetPath()))
        self.assertTrue(query.IsPathIncluded(hemiSphere2.GetPath()))

        # Add back sphere and verify that everything is included now.
        self.assertTrue(geomCollection.IncludePath(sphere.GetPath()))

        query = geomCollection.ComputeMembershipQuery()
        self.assertTrue(query.IsPathIncluded(sphere.GetPath()))
        self.assertTrue(query.IsPathIncluded(hemiSphere1.GetPath()))
        self.assertTrue(query.IsPathIncluded(hemiSphere2.GetPath()))
        self.assertTrue(query.IsPathIncluded(cylinder.GetPath()))
        self.assertTrue(query.IsPathIncluded(cube.GetPath()))

    def test_testReadCollection(self):
        leafGeom = Usd.CollectionAPI(testPrim, "leafGeom")
        (valid, reason) = leafGeom.Validate()
        self.assertTrue(valid)

        # Test the other overload of GetCollection.
        leafGeomPath = leafGeom.GetCollectionPath()
        leafGeom = Usd.CollectionAPI.GetCollection(stage, leafGeomPath)
        self.assertEqual(leafGeom.GetCollectionPath(), leafGeomPath)

        (valid, reason) = leafGeom.Validate()
        self.assertTrue(valid)
        
        # Test GetName() API.
        self.assertEqual(leafGeom.GetName(), 'leafGeom')

        # Test Get/IsCollectionPath API.
        self.assertTrue(Usd.CollectionAPI.IsCollectionPath(
            leafGeom.GetCollectionPath()))

        # Ensure that paths of collection schema properties aren't valid
        # collection paths.
        self.assertFalse(Usd.CollectionAPI.IsCollectionPath(
            leafGeom.GetExpansionRuleAttr().GetPath()))
        self.assertFalse(Usd.CollectionAPI.IsCollectionPath(
            leafGeom.GetIncludesRel().GetPath()))

        leafGeomMquery = leafGeom.ComputeMembershipQuery()
        self.assertEqual(
            len(Usd.CollectionAPI.ComputeIncludedObjects(leafGeomMquery,
                                                         stage)),
            2)

        # Calling ApplyCollection on an already existing collection will update
        # the expansionRule.
        self.assertEqual(leafGeom.GetExpansionRuleAttr().Get(), 
                         Usd.Tokens.explicitOnly)
        leafGeom = Usd.CollectionAPI.ApplyCollection(testPrim, "leafGrom", 
            Usd.Tokens.expandPrims)
        self.assertEqual(leafGeom.GetExpansionRuleAttr().Get(), 
                         Usd.Tokens.expandPrims)

        allGeom = Usd.CollectionAPI(testPrim, "allGeom")
        (valid, reason) = allGeom.Validate()
        allGeomMquery = allGeom.ComputeMembershipQuery()
        self.assertEqual(len(Usd.CollectionAPI.ComputeIncludedObjects(
                allGeomMquery,stage)), 9)

        # included object count increases when we count instance proxies.
        self.assertEqual(len(Usd.CollectionAPI.ComputeIncludedObjects(
                allGeomMquery,stage,
                predicate=Usd.TraverseInstanceProxies())), 11)
    
        allGeomProperties = Usd.CollectionAPI(testPrim, "allGeomProperties")
        (valid, reason) = allGeomProperties.Validate()
        allGeomPropertiesMquery = allGeomProperties.ComputeMembershipQuery()
        self.assertEqual(len(Usd.CollectionAPI.ComputeIncludedObjects(
                allGeomPropertiesMquery, stage)), 27)

        hasRels = Usd.CollectionAPI(testPrim, "hasRelationships")
        (valid, reason) = hasRels.Validate()
        self.assertTrue(valid)
        hasRelsMquery = hasRels.ComputeMembershipQuery()
        incObjects = Usd.CollectionAPI.ComputeIncludedObjects(hasRelsMquery, stage)
        for obj in incObjects: 
            self.assertTrue(isinstance(obj, Usd.Property))

        hasInstanceProxy = Usd.CollectionAPI(testPrim, "hasInstanceProxy")
        (valid, reason) = hasInstanceProxy.Validate()
        self.assertTrue(valid)
        hasInstanceProxyMquery = hasInstanceProxy.ComputeMembershipQuery()
        incObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                hasInstanceProxyMquery, stage)
        self.assertEqual(len(incObjects), 2)
        for obj in incObjects:
            self.assertTrue(obj.IsInstanceProxy())
            self.assertFalse(obj.IsInMaster())
        
        coneProperties = Usd.CollectionAPI(testPrim, "coneProperties")
        (valid, reason) = coneProperties.Validate()
        self.assertTrue(valid)
        conePropertiesMquery = coneProperties.ComputeMembershipQuery()
        incObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                conePropertiesMquery, stage)
        self.assertEqual(len(incObjects), 2)
        for obj in incObjects:
            self.assertTrue(isinstance(obj, Usd.Property))


        includesCollection = Usd.CollectionAPI(testPrim, "includesCollection")
        (valid, reason) = includesCollection.Validate()
        self.assertTrue(valid)
        includesCollectionMquery = includesCollection.ComputeMembershipQuery()
        incObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                includesCollectionMquery, stage)
        self.assertTrue(hemiSphere2 in incObjects)
        self.assertTrue(hemiSphere1 not in incObjects)

        excludeInstanceGeom = Usd.CollectionAPI(testPrim, "excludeInstanceGeom")
        (valid, reason) = excludeInstanceGeom.Validate()
        self.assertTrue(valid)
        excludeInstanceGeomMquery = excludeInstanceGeom.ComputeMembershipQuery()
        incObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                excludeInstanceGeomMquery, stage)
        self.assertEqual(len(incObjects), 1)

        allIncObjects = Usd.CollectionAPI.ComputeIncludedObjects(
                excludeInstanceGeomMquery, stage, 
                predicate=Usd.TraverseInstanceProxies())
        self.assertEqual(len(allIncObjects), 2)

    def test_invalidCollections(self):
        invalidCollectionNames = ["invalidExpansionRule", 
            "invalidExcludesExplicitOnly",
            "invalidExcludesExpandPrims"]

        for collName in invalidCollectionNames:
            coll = Usd.CollectionAPI(testPrim, collName)
            (valid, reason) = coll.Validate()
            self.assertFalse(valid)
            self.assertTrue(len(reason) > 0)

    def test_CircularDependency(self):
        collectionA = Usd.CollectionAPI.ApplyCollection(testPrim, 
                "A", Usd.Tokens.explicitOnly)
        collectionB = Usd.CollectionAPI.ApplyCollection(testPrim, 
                "B")
        collectionC = Usd.CollectionAPI.ApplyCollection(testPrim, 
                "C")

        collectionD = Usd.CollectionAPI.ApplyCollection(testPrim, 
                "D")
                
        collectionA.CreateIncludesRel().AddTarget(
                collectionB.GetCollectionPath())
        collectionB.CreateIncludesRel().AddTarget(
                collectionC.GetCollectionPath())
        collectionC.CreateIncludesRel().AddTarget(
                collectionA.GetCollectionPath())
        
        collectionD.CreateIncludesRel().AddTarget(geom.GetPath())

        ComputeIncObjs = Usd.CollectionAPI.ComputeIncludedObjects

        # XXX: It would be good to verify that this produces a warning.
        (valid, reason) = collectionA.Validate()
        self.assertFalse(valid)
        self.assertTrue('circular' in reason)
        mqueryA = collectionA.ComputeMembershipQuery()
        self.assertEqual(len(ComputeIncObjs(mqueryA, stage)), 0)

        (valid, reason) = collectionB.Validate()
        self.assertFalse(valid)
        self.assertTrue('circular' in reason)
        mqueryB = collectionB.ComputeMembershipQuery()
        self.assertEqual(len(ComputeIncObjs(mqueryB, stage)), 0)

        (valid, reason) = collectionC.Validate()
        self.assertFalse(valid)
        self.assertTrue('circular' in reason)
        mqueryC = collectionC.ComputeMembershipQuery()
        self.assertEqual(len(ComputeIncObjs(mqueryC, stage)), 0)

        # Now, if A includes D, the warning about circular dependency should 
        # not prevent inclusion of D in A, B or C.
        collectionA.CreateIncludesRel().AddTarget(
            collectionD.GetCollectionPath())
        mqueryA = collectionA.ComputeMembershipQuery()
        self.assertEqual(len(ComputeIncObjs(mqueryA, stage)), 9)

        mqueryB = collectionB.ComputeMembershipQuery()
        self.assertEqual(len(ComputeIncObjs(mqueryB, stage)), 9)

        mqueryC = collectionC.ComputeMembershipQuery()
        self.assertEqual(len(ComputeIncObjs(mqueryC, stage)), 9)

    def test_InvalidApplyCollection(self):
        # ----------------------------------------------------------
        # Test ApplyCollection when passed a string that doesn't tokenize to
        # make sure we don't crash in that case, but issue a coding error.
        with self.assertRaises(Tf.ErrorException):
            Usd.CollectionAPI.ApplyCollection(testPrim, "", 
                    Usd.Tokens.explicitOnly)


if __name__ == "__main__":
    unittest.main()
