import sys, os
from pxr import Sdf, Usd, Vt, Tf, Ar
from Mentor.Runtime import AssertTrue,AssertFalse,AssertEqual,AssertNotEqual,RequiredException

# --------------------------------------------------------------------------- #
# Init & Cleanup 
# --------------------------------------------------------------------------- #

class Init:
    def init():
        pass


class Cleanup:
    def terminate(Stage):
        Stage.Close()


# --------------------------------------------------------------------------- #
# Simple Value Inputs 
# --------------------------------------------------------------------------- #

class RelTarget:
    def foobar():
        return Sdf.Path("/Foo/Bar") 


class RelMetadataSpec:
    def comment():
        return ("comment", "Rel metadata value")


class AttrMetadataSpec:
    def activeTrue():
        return ("active", True)


class PrimMetadataSpec:
    def comment():
        return ("comment", "Yes Indeed")

    # XXX:bug 84215
    # def variantSelectionEmpty():
    #     return ("variantSelection", {})

    # def primOrderEmpty():
    #     return ("primOrder", [])

    # def propertyOrderEmpty():
    #     return ("propertyOrder", [])

class AttrSpec:
    def string():
        return dict(name="Something",
                    typeName=Sdf.ValueTypeNames.String,
                    default="foobar",
                    varying="foovar",
                    custom=True)

    def customVerts():
        return dict(name="verts",
                    typeName=Sdf.ValueTypeNames.IntArray,
                    default=Vt.IntArray(range(3)),
                    varying=Vt.IntArray(range(3,6)),
                    custom=True)

    def builtinVerts():
        return dict(name="verts",
                    typeName=Sdf.ValueTypeNames.IntArray,
                    default=Vt.IntArray(range(3)),
                    varying=Vt.IntArray(range(3,6)),
                    custom=False)


class VaryingTime:
    def time99():
        return 99.0


class AttrSpecRefUnvarying:
    def nameAndType():
        return ("fromRef", Sdf.ValueTypeNames.String)


class AttrSpecRefVarying:
    def nameAndType():
        return ("varyingAttr", Sdf.ValueTypeNames.String)


class AttrVaryingRefValue:
    def valueAtTime():
        return "Value at Time"


class RelSpec:
    def something():
        return "relToOther"


class SceneFile:
    def fooUsd():
        import os, time
        sceneFile = "foo.usd"
        Sdf.Layer.CreateNew(sceneFile)
        return sceneFile 

class PrimSpec:
    def foo():
        return [("/Foo", "Xform")]

    def fooBar():
        return [("/Foo", "Xform"),
                ("/Foo/Bar", "Xform"),]


#-----------------------------------------------------------------------------#
# Stage
#-----------------------------------------------------------------------------#

class Stage:
    def usdMode(SceneFile):
        st = Usd.Stage.Open(SceneFile)
        # Testing HasLocalLayer() for layers in and outside
        # the stage's LayerStack
        subLayer = Sdf.Layer.CreateNew("sub_" + SceneFile)
        nonSubLayer = Sdf.Layer.CreateNew("nonSub_" + SceneFile)
        st.GetRootLayer().subLayerPaths.append(subLayer.identifier)
        AssertTrue(st.HasLocalLayer(st.GetRootLayer()))
        AssertTrue(st.HasLocalLayer(subLayer))
        AssertFalse(st.HasLocalLayer(nonSubLayer))
        p = st.GetPseudoRoot()
        # this will return none, but used to crash
        p.GetMetadata("documentation")
        #TODO: assert p.GetMetadata("documentation") == None
        #      once this is works consistently
        AssertEqual(st.GetStartTimeCode(), 0.0)
        AssertEqual(st.GetEndTimeCode(), 0.0)
        AssertEqual(st.GetTimeCodesPerSecond(), 24.0)
        AssertEqual(st.GetFramesPerSecond(), 24.0)
        assert not st.HasAuthoredTimeCodeRange()
        st.SetStartTimeCode(1.0)
        st.SetEndTimeCode(24.0)
        st.SetTimeCodesPerSecond(30.0)
        st.SetFramesPerSecond(48.0)
        AssertEqual(st.GetStartTimeCode(), 1.0)
        AssertEqual(st.GetEndTimeCode(), 24.0)
        AssertEqual(st.GetTimeCodesPerSecond(), 30.0)
        AssertEqual(st.GetFramesPerSecond(), 48.0)
        assert st.HasAuthoredTimeCodeRange()
        AssertEqual(st.GetPathResolverContext(), 
                    Ar.GetResolver().CreateDefaultContextForAsset(
                st.GetRootLayer().realPath))
        return st

    def usdModeInMemory():
        layer = Sdf.Layer.CreateAnonymous()
        st = Usd.Stage.Open(layer.identifier)
        AssertTrue(st.HasLocalLayer(layer))
        layer2 = Sdf.Layer.CreateAnonymous()
        AssertFalse(st.HasLocalLayer(layer2))
        p = st.GetPseudoRoot()
        # this will return none, but used to crash
        p.GetMetadata("documentation")
        #TODO: assert p.GetMetadata("documentation") == None
        #      once this is works consistently
        AssertEqual(st.GetStartTimeCode(), 0.0)
        AssertEqual(st.GetEndTimeCode(), 0.0)
        AssertEqual(st.GetTimeCodesPerSecond(), 24.0)
        AssertEqual(st.GetFramesPerSecond(), 24.0)
        assert not st.HasAuthoredTimeCodeRange()
        st.SetStartTimeCode(1.0)
        st.SetEndTimeCode(24.0)
        st.SetTimeCodesPerSecond(30.0)
        st.SetFramesPerSecond(48.0)
        AssertEqual(st.GetStartTimeCode(), 1.0)
        AssertEqual(st.GetEndTimeCode(), 24.0)
        AssertEqual(st.GetTimeCodesPerSecond(), 30.0)
        AssertEqual(st.GetFramesPerSecond(), 48.0)
        assert st.HasAuthoredTimeCodeRange()
        return st


#-----------------------------------------------------------------------------#
# Variants
#-----------------------------------------------------------------------------#

class VariantSet:
    def lod(Stage, Prim):
        assert Prim.GetVariantSets().GetNames() == []
        Prim.GetVariantSets().FindOrCreate("LOD")
        assert "LOD" in Prim.GetVariantSets().GetNames()
        return "LOD"


class Variant:
    def low(SceneFile, Stage, Prim, VariantSet, Schema, Reference, AttrSpecRefUnvarying):
        from pxr import Tf
        
        attrName,attrType = AttrSpecRefUnvarying
        assert Prim.GetAttribute(attrName).Get()

        lod = Prim.GetVariantSet("LOD")
        assert lod.FindOrCreateVariant("Low")
        primVarSelPath = Prim.GetPath().AppendVariantSelection('LOD', 'Low')
        assert not Stage.GetPrimAtPath(primVarSelPath)
        lod.SetVariantSelection('Low')
        assert not Stage.GetPrimAtPath(primVarSelPath)
        with Usd.EditContext(Stage, lod.GetVariantEditTarget()):
            Prim.CreateAttribute(attrName, attrType)
            Prim.GetAttribute(attrName).Set("VariantValue")
            childPath = Prim.GetPath().AppendChild('Foobar')
            Stage.DefinePrim(childPath, 'Scope')

        child = Stage.GetPrimAtPath(childPath)
        assert child
        childVarSelPath = primVarSelPath.AppendChild('Foobar')
        assert not Stage.GetPrimAtPath(childVarSelPath)
        assert child in Prim.GetAllChildren()

        # With no selection, Foobar should disappear -- it's only defined in the
        # Low variant.
        lod.ClearVariantSelection()
        assert not Stage.GetPrimAtPath(childPath)

        # Switch back to 'Low' selection.
        lod.SetVariantSelection('Low')
        # Must re-get child -- it's expired...
        child = Stage.GetPrimAtPath(childPath)
        assert child

        assert Stage.GetPrimAtPath(Prim.GetPath().AppendChild("Foobar"))
        AssertEqual(Prim.GetAttribute(attrName).Get(), "VariantValue")

        # Author direct opinions.
        child.CreateAttribute("test", Sdf.ValueTypeNames.String)
        child.GetAttribute("test").Set("Hello World")
        child.GetAttribute("test").Set("Hello 2", 2.0)

        # Switch back to no selection.
        lod.ClearVariantSelection()

        # Prim exists with no selection due to the direct opinions just authored
        # above.
        assert Stage.GetPrimAtPath(Prim.GetPath().AppendChild("Foobar"))

        # Direct opinions should win over the variant opinions.
        AssertNotEqual(Prim.GetAttribute(attrName).Get(),
                       "VariantValue")
        AssertEqual(child.GetAttribute("test").Get(), "Hello World")
        AssertEqual(child.GetAttribute("test").Get(2.0), "Hello 2")

        standin = child.GetVariantSets().FindOrCreate("Standin")
        standin.FindOrCreateVariant("Anim")
        standin.SetVariantSelection('Anim')
        with Usd.EditContext(Stage, standin.GetVariantEditTarget(
                Stage.GetRootLayer())):
            animChild = Stage.OverridePrim(
                Prim.GetPath().AppendPath("Anim/Standin/Child"))

            tempAttr = animChild.CreateAttribute("nested", Sdf.ValueTypeNames.String)
            tempAttr.Set("nestedAnimValue")
            AssertEqual(tempAttr.Get(), "nestedAnimValue")
            animChild.SetMetadata('active', False)
            # XXX: animChild is dead now, due to active=False ?  Is this a bug?
            # animChild.SetMetadata('active', True)

        del lod, child, animChild


#-----------------------------------------------------------------------------#
# Prim and Schema
#-----------------------------------------------------------------------------#

class Prim:
    @classmethod
    def _ValidatePrim(cls, prim, typeName):
        assert prim
        AssertEqual(prim.GetTypeName(), typeName)

    def rootPrimWithNoType(Stage, PrimSpec):
        for primPath, primType in PrimSpec:
            assert Stage.OverridePrim(primPath)
            Prim._ValidatePrim(Stage.GetPrimAtPath(primPath), "")
        return Stage.GetPrimAtPath(primPath)

    def rootPrim(Stage, PrimSpec):
        for primPath, primType in PrimSpec:
            assert Stage.DefinePrim(primPath, primType)
            Prim._ValidatePrim(Stage.GetPrimAtPath(primPath), primType)
        return Stage.GetPrimAtPath(primPath)

    def nestedParent(Stage, PrimSpec):
        for primPath, primType in PrimSpec:
            assert Stage.DefinePrim(primPath, primType)
            Prim._ValidatePrim(Stage.GetPrimAtPath(primPath), primType)
        return Stage.GetPrimAtPath(PrimSpec[0][0])

    def parentOver(Stage, PrimSpec):
        for primPath, primType in PrimSpec:
            prim = Stage.OverridePrim(primPath)
            assert prim, primPath
            Prim._ValidatePrim(prim, "")
        return Stage.GetPrimAtPath(PrimSpec[0][0])

    def childOver(Stage, PrimSpec):
        for primPath, primType in PrimSpec:
            prim = Stage.OverridePrim(primPath)
            assert prim, primPath
            Prim._ValidatePrim(prim, "")
        return Stage.GetPrimAtPath(PrimSpec[-1][0])


class Schema:
    def byPrim(Prim):
        return Usd.SchemaBase(Prim)


class PrimMetadataIO:
    def basic(Prim, PrimMetadataSpec):
        key, value = PrimMetadataSpec
        Prim.SetMetadata(key, value)
        AssertEqual(Prim.GetMetadata(key), value)
        AssertEqual(Prim.GetMetadata("bad_key"), None)

#-----------------------------------------------------------------------------#
# References 
#-----------------------------------------------------------------------------#

class RefSpec:
    def simple(AttrSpecRefUnvarying, AttrSpecRefVarying, VaryingTime, AttrVaryingRefValue):
        # TODO: make in-memory when that option is available
        try:
            import os
            os.unlink("other.usd")
        except:
            pass

        st = Usd.Stage.Open(Sdf.Layer.CreateNew("other.usd"))
        st.DefinePrim("/Ref/ReferencedPrim", "Scope")
        refPrim = st.GetPrimAtPath(Sdf.Path("/Ref"))
        
        tempAttr = refPrim.CreateAttribute(*AttrSpecRefUnvarying)
        tempAttr.Set("From Reference")

        tempAttr = refPrim.CreateAttribute(*AttrSpecRefVarying)
        tempAttr.Set(AttrVaryingRefValue, VaryingTime)

        st.GetRootLayer().Save()
        st.Close()
        del st
        return ("other.usd", Sdf.Path("/Ref"), "ReferencedPrim")


class Reference:
    def simple(Stage, Prim, RefSpec):
        refFile, refPath, includedPrim = RefSpec
        schema = Usd.SchemaBase(Prim)
        ref = Sdf.Reference(refFile, refPath)
        newPath = schema.GetPath().AppendPath(includedPrim)

        assert Prim.GetReferences().GetPrim()

        # This is too simplistic, but it tests the most basic use of these
        # methods. It also generates a lot of composition work.

        # Add / Remove
        assert Prim.GetReferences().Add(ref)
        assert Stage.GetPrimAtPath(newPath), newPath
        assert Prim.GetReferences().Remove(ref)
        assert not Stage.GetPrimAtPath(newPath)

        # Add / Clear
        assert Prim.GetReferences().Add(ref)
        assert Stage.GetPrimAtPath(newPath), newPath
        assert Prim.GetReferences().Clear()
        assert not Stage.GetPrimAtPath(newPath)

        # Finally, Add the reference and leave it in place
        assert Prim.GetReferences().Add(ref)
        assert Stage.GetPrimAtPath(newPath), newPath

        return Stage.GetPrimAtPath(newPath)


#-----------------------------------------------------------------------------#
# Attributes
#-----------------------------------------------------------------------------#

class AttrAlts:
    # A bunch of attributes for cases when more than one is needed
    def create(Prim):
        names = ["attr" + str(i) for i in range(9, -1, -1)]
        for name in names:
            assert Prim.CreateAttribute(name, Sdf.ValueTypeNames.Int)
        return names


class Attr:
    def create(Prim, AttrSpec):
        assert Prim.CreateAttribute(AttrSpec['name'],
                                    AttrSpec['typeName'],
                                    AttrSpec['custom'])
        assert AttrSpec['name'] in Prim.GetPropertyNames()
        assert Prim.GetAttribute(AttrSpec['name']).IsDefined()
        # This is testing the default value of the custom parameter of
        # Attributes::Create, it is assumed to be false (when len(attrSpec) < 4)
        # except in the case where we explicitly set it in last value of
        # AttrSpec
        AssertEqual(Prim.GetAttribute(AttrSpec['name']).IsCustom(),
                    AttrSpec['custom'])
        return AttrSpec['name']


class ConstAttrAuthoredValue:
    def unvarying(Prim, Attr, AttrSpec):
        value = AttrSpec['default']
        Prim.GetAttribute(Attr).Set(value)
        newValue = Prim.GetAttribute(Attr).Get()
        AssertEqual(value, newValue)
        assert not Prim.GetAttribute("bad_attr")
        return newValue

class VaryingAttrAuthoredValue:
    def varying(Prim, Attr, AttrSpec,
                AttrSpecRefVarying, VaryingTime, AttrVaryingRefValue):
        Prim.GetAttribute(Attr).Set(AttrSpec['varying'], 10)
        Prim.GetAttribute(Attr).Set(AttrSpec['varying'], 12)
        newValue = Prim.GetAttribute(Attr).Get(11)
        AssertEqual(AttrSpec['varying'], newValue)

        # XXX: This shouldn't be required, the spec was already created
        #      in the reference.
        tempAttr = Prim.CreateAttribute(*AttrSpecRefVarying)
        AssertEqual(AttrVaryingRefValue, tempAttr.Get(VaryingTime))
        
        defClobberValue = "local default should clobber varying from ref"
        tempAttr.Set(defClobberValue)
        AssertEqual(defClobberValue, tempAttr.Get(VaryingTime))

        offClobberValue = "offset time local should clobber varying from ref"
        tempAttr.Set(offClobberValue, VaryingTime+1)
        AssertEqual(offClobberValue, tempAttr.Get(VaryingTime))

        varClobberValue = "local time should clobber local offset time"
        tempAttr.Set(varClobberValue, VaryingTime)
        AssertEqual(varClobberValue, tempAttr.Get(VaryingTime))

        # First time sample should extend to negative infinity and beyond!
        AssertEqual(varClobberValue, tempAttr.Get(-999999.0))

        AssertEqual(Prim.GetAttribute("bad_attr").Get(10), None)

        return (newValue, 10)

class AttrValueMightBeTimeVarying:
    def basic(Prim, Attr, AttrSpec):
        timeSamples = Prim.GetAttribute(Attr).GetTimeSamples()
        if len(timeSamples) > 1:
            AssertTrue(Prim.GetAttribute(Attr).ValueMightBeTimeVarying())
        if not Prim.GetAttribute(Attr).ValueMightBeTimeVarying():
            AssertTrue(len(timeSamples) == 0 or len(timeSamples) == 1)

class AttrMetadataIO:
    def basic(Prim, Attr, AttrMetadataSpec):
        key, value = AttrMetadataSpec
        tempAttr = Prim.GetAttribute(Attr)
        assert tempAttr.SetMetadata(key, value)
        AssertEqual(tempAttr.GetMetadata(key), value)
        AssertEqual(tempAttr.GetAllMetadata()[key], value)
        AssertEqual(tempAttr.GetMetadata("bad_key"), None)
 

#-----------------------------------------------------------------------------#
# Relationships 
#-----------------------------------------------------------------------------#

class ForwardedRel:
    #
    # These cases are combined into a single test because the result of
    # ForwardedRel is not used downstream, thus creating lots of test runs
    # with no benefit.  It would be nice if the Combine test generator would do
    # this automatically.
    #
    @classmethod
    def _simple(cls, Schema, AttrAlts):
        return ([
                ("a", [Schema.GetPath().AppendProperty("b")] ),
                ("b", [Schema.GetPath().AppendProperty(AttrAlts[0])] ), 
               ], 
               [Schema.GetPath().AppendProperty(AttrAlts[0])])

    @classmethod
    def _fanOutIn(cls, Schema, AttrAlts):
        return ([
                ("c", [
                        Schema.GetPath().AppendProperty("b"),
                        Schema.GetPath().AppendProperty("e"),
                        Schema.GetPath().AppendProperty(AttrAlts[0]),
                      ] ),
                ("d", [Schema.GetPath().AppendProperty("e")] ), 
                ("e", [Schema.GetPath().AppendProperty(AttrAlts[0])] ), 
               ], 
               [Schema.GetPath().AppendProperty(AttrAlts[0])])

    @classmethod
    def _cycle(cls, Schema, AttrAlts):
        # Create a cycle, it should silently be ignored
        return ([
                ("f", [
                        Schema.GetPath().AppendProperty("g"),
                      ] ),
                ("g", [
                        Schema.GetPath().AppendProperty("f"),
                        Schema.GetPath().AppendProperty(AttrAlts[0]),
                      ] ), 
               ], 
               [Schema.GetPath().AppendProperty(AttrAlts[0])])

    @classmethod
    def _fanOut(cls, Schema, AttrAlts):
        # Create a cycle, it should silently be ignored
        return ([
                ("h", [
                        Schema.GetPath().AppendProperty("i"),
                        Schema.GetPath().AppendProperty("j"),
                        Schema.GetPath().AppendProperty("k"),
                      ] ),
                ("i", [
                        Schema.GetPath().AppendProperty(AttrAlts[0]),
                      ] ), 

                ("j", [
                        Schema.GetPath().AppendProperty(AttrAlts[1]),
                      ] ), 
                ("k", [
                        Schema.GetPath().AppendProperty(AttrAlts[2]),
                      ] ), 

               ], 
               [
                    Schema.GetPath().AppendProperty(AttrAlts[0]),
                    Schema.GetPath().AppendProperty(AttrAlts[1]),
                    Schema.GetPath().AppendProperty(AttrAlts[2]),
               ])

    def basic(Stage, AttrAlts, Schema, Prim):
        funcs = [ForwardedRel._simple,
                ForwardedRel._fanOutIn,
                ForwardedRel._cycle,
                ForwardedRel._fanOut]

        for fwdRel in [func(Schema,AttrAlts) for func in funcs]:
            for spec, targets in fwdRel[0]:
                if not Prim.GetRelationship(spec):
                    Prim.CreateRelationship(spec)
                for target in targets:
                    Prim.CreateRelationship(spec).AddTarget(target)
                # Make sure non-existent-targets do not crash GetForwardedTargets
                Prim.CreateRelationship(spec).AddTarget('/i/do/not/exist')
            mainSpec = fwdRel[0][0][0]
            expectedTargets = list(fwdRel[1]) + [Sdf.Path('/i/do/not/exist')]
            expectedTargets = sorted(expectedTargets)
            AssertEqual(sorted(Prim.GetRelationship(mainSpec).GetForwardedTargets()), 
                        expectedTargets)
        
class Rel:
    def create(Prim, RelSpec):
        assert not RelSpec in Prim.GetPropertyNames()
        assert not Prim.GetRelationship(RelSpec)
        assert not Prim.GetRelationship(RelSpec).IsDefined()
        
        Prim.CreateRelationship(RelSpec)
        
        assert RelSpec in Prim.GetPropertyNames()
        assert Prim.GetRelationship(RelSpec)
        assert Prim.GetRelationship(RelSpec).IsDefined()
        assert Prim.GetRelationship(RelSpec).IsCustom()

        AssertEqual(Prim.GetRelationship(RelSpec).GetTargets(), [])

        return Prim.GetRelationship(RelSpec)


class RelIO:
    def setTargets(Stage, Schema, Rel, RelTarget):
        assert RelTarget not in Rel.GetTargets()
        Rel.SetTargets([RelTarget])
        assert RelTarget in Rel.GetTargets()

        return (Rel, RelTarget)
       
    def addTarget(Stage, Schema, Rel, RelTarget):
        assert RelTarget not in Rel.GetTargets()
        Rel.AddTarget(RelTarget)
        assert RelTarget in Rel.GetTargets()

        return (Rel, RelTarget)


class RelMetadataIO:
    def basic(Schema, Rel, RelMetadataSpec):
        key, value = RelMetadataSpec
        Rel.SetMetadata(key, value)
        retVal = Rel.GetMetadata(key)
        AssertEqual(retVal, value)
        AssertEqual(Rel.GetAllMetadata()[key], value)
        AssertEqual(Rel.GetMetadata("bad_key"), None)


class RemovedRelTarget:
    def blockRelTrg(Stage, Schema, Rel, RelIO, RelTarget, RelMetadataIO):
        assert RelTarget in Rel.GetTargets()
        Rel.BlockTargets()
        assert RelTarget not in Rel.GetTargets()
        return RelTarget 

    def removeRelTrg(Stage, Schema, Rel, RelIO, RelTarget, RelMetadataIO):
        assert RelTarget in Rel.GetTargets()
        Rel.RemoveTarget(RelTarget)
        assert RelTarget not in Rel.GetTargets()
        return RelTarget 


#-----------------------------------------------------------------------------#
# TreeIterator 
#-----------------------------------------------------------------------------#

class Traversal:
    #
    # Helper method implementing tree traversal.
    # Note that because this method is private, it will not be selected for 
    # execution by the Combine fixture.
    #
    @classmethod
    def _Traverse(cls, prim, outList, pre=True, post=False):
        if pre:
            outList.append(prim.GetPath())
        for primChild in prim.GetAllChildren():
            Traversal._Traverse(primChild, outList, pre, post)
        if post:
            outList.append(prim.GetPath())

    #
    # Traversals are aggregated here because individual runs add to many test
    # variations and those variations don't add value
    #
    def allTraversals(Stage, Prim):
        def preOrder(Stage, Prim):
            prims = []
            expected = []
            Traversal._Traverse(Prim, expected, True, False)
            it = Usd.TreeIterator.AllPrims(Prim)
            for x in it:
                assert x
                assert not it.IsPostVisit()
                prims.append(x.GetPath())
            AssertEqual(expected, prims)

        def preAndPostOrder(Stage, Prim):
            # Make sure both the core visitation and the IsPostVisit
            # functionality works. This is done by splitting the results out
            # into three lists of prims.
            prims = []
            primsPre = []
            primsPost = []

            expected = []
            expPre = []
            expPost = []

            Traversal._Traverse(Prim, expected, True, True)
            Traversal._Traverse(Prim, expPre, True, False)
            Traversal._Traverse(Prim, expPost, False, True)

            itr = Usd.TreeIterator.AllPrimsPreAndPostVisit(Prim)
            for x in itr:
                assert x
                prims.append(x.GetPath())
                if itr.IsPostVisit():
                    primsPost.append(x.GetPath())
                else:
                    primsPre.append(x.GetPath())

            AssertEqual(expected, prims)
            AssertEqual(expPre, primsPre)
            AssertEqual(expPost, primsPost)

        preOrder(Stage, Prim)
        preAndPostOrder(Stage, Prim)


#-----------------------------------------------------------------------------#
# Removal
#-----------------------------------------------------------------------------#

class RemovedPrim:
    def removePrim(Traversal, RemovedRelTarget, PrimMetadataIO, AttrMetadataIO, 
                   VariantSet, Variant,
                   ConstAttrAuthoredValue,
                   VaryingAttrAuthoredValue,
                   Schema, Prim, Stage):
        assert Prim
        path = Prim.GetPath()
        assert Schema.GetPrim()
        assert Stage.RemovePrim(path)
        assert not Stage.GetPrimAtPath(path)
        assert not Schema.GetPrim()
        assert not Prim
        return Prim

class RemoveProp:
    def removeProp(Schema, Stage, Prim, Attr, VaryingAttrAuthoredValue,
                   PrimMetadataIO, AttrMetadataIO):
        assert Prim
        attrName = Prim.GetAttribute(Attr).GetName() 
        assert Prim.RemoveProperty(attrName)
        return Prim 

#-----------------------------------------------------------------------------#
# AttributeQuery
#-----------------------------------------------------------------------------#

class AttributeQuery:
    def basic(Prim, Attr, AttrSpec):
        attr = Prim.GetAttribute(Attr)
        attr.Set(AttrSpec['default'])
        attr.Set(AttrSpec['varying'], 40)
        attr.Set(AttrSpec['varying'], 42)
        
        query = Usd.AttributeQuery(Prim, AttrSpec['name'])

        AssertTrue(query)
        AssertTrue(query.IsValid())
        AssertEqual(query.GetAttribute(), attr)

        AssertEqual(query.Get(40), attr.Get(40))
        AssertEqual(query.Get(42), attr.Get(42))

        AssertEqual(query.Get(), attr.Get())

        AssertEqual(query.GetTimeSamples(), attr.GetTimeSamples())

        for t in xrange(39, 43):
            AssertEqual(query.GetBracketingTimeSamples(t),
                        attr.GetBracketingTimeSamples(t))

        AssertEqual(query.HasValue(), attr.HasValue())
        AssertEqual(query.HasAuthoredValueOpinion(), 
                    attr.HasAuthoredValueOpinion())
        AssertEqual(query.ValueMightBeTimeVarying(), 
                    attr.ValueMightBeTimeVarying())

