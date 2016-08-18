#!/pxrpythonsubst

import sys, os
from pxr import Sdf, Usd, Tf, Plug

import Mentor.Runtime
from Mentor.Runtime import (AssertEqual, AssertFalse, AssertTrue,
                            FindDataFile, ExpectedErrors, RequiredException)

allFormats = ['usd' + x for x in 'abc']

def TestHidden():
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestHidden.'+fmt)

        stageRoot = stage.GetPseudoRoot()
        foo = stage.OverridePrim("/Foo")
        attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
        rel = foo.CreateRelationship("rel")

        #
        # Layer test - the stage's root prim gives "direct access" to layer
        # metadata
        #
        AssertEqual(stageRoot.IsHidden(), False)
        AssertEqual(stageRoot.HasAuthoredHidden(), False)
        AssertEqual(stageRoot.GetMetadata("hidden"), None)

        AssertEqual(stageRoot.SetHidden(False), True)
        AssertEqual(stageRoot.HasAuthoredHidden(), True)
        AssertEqual(stageRoot.IsHidden(), False)
        AssertEqual(stageRoot.GetMetadata("hidden"), False)

        AssertEqual(stageRoot.SetHidden(True), True)
        AssertEqual(stageRoot.IsHidden(), True)
        AssertEqual(stageRoot.HasAuthoredHidden(), True)
        AssertEqual(stageRoot.GetMetadata("hidden"), True)

        AssertEqual(stageRoot.ClearHidden(), True)
        AssertEqual(stageRoot.IsHidden(), False)
        AssertEqual(stageRoot.HasAuthoredHidden(), False)
        AssertEqual(stageRoot.GetMetadata("hidden"), None)

        #
        # Prim test.
        #
        AssertEqual(foo.IsHidden(), False)
        AssertEqual(foo.GetMetadata("hidden"), None)

        AssertEqual(foo.SetHidden(False), True)
        AssertEqual(foo.IsHidden(), False)
        AssertEqual(foo.GetMetadata("hidden"), False)

        AssertEqual(foo.SetHidden(True), True)
        AssertEqual(foo.IsHidden(), True)
        AssertEqual(foo.GetMetadata("hidden"), True)

        #
        # Attribute test.
        #
        AssertEqual(attr.IsHidden(), False)
        AssertEqual(attr.GetMetadata("hidden"), None)

        AssertEqual(attr.SetHidden(False), True)
        AssertEqual(attr.IsHidden(), False)
        AssertEqual(attr.GetMetadata("hidden"), False)

        AssertEqual(attr.SetHidden(True), True)
        AssertEqual(attr.IsHidden(), True)
        AssertEqual(attr.GetMetadata("hidden"), True)

        #
        # Relationship test.
        #
        AssertEqual(rel.IsHidden(), False)
        AssertEqual(rel.GetMetadata("hidden"), None)

        AssertEqual(rel.SetHidden(False), True)
        AssertEqual(rel.IsHidden(), False)
        AssertEqual(rel.GetMetadata("hidden"), False)

        AssertEqual(rel.SetHidden(True), True)
        AssertEqual(rel.IsHidden(), True)
        AssertEqual(rel.GetMetadata("hidden"), True)


def TestListAndHas():
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestListAndHas.'+fmt)

        stageRoot = stage.GetPseudoRoot()
        foo = stage.OverridePrim("/Foo")
        attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)

        rel = foo.CreateRelationship("rel")

        # Ensure we can read the reported values
        print "Verify GetMetadata(key) and HasMetadata(key)..."
        def _TestGetHas(obj):
            for key in obj.GetAllMetadata():
                assert obj.GetMetadata(key) is not None, \
                     ("Expected to GET metadata key '%s' in %s" % (key, obj))
                assert obj.HasMetadata(key), \
                     ("Expected to HAVE metadata key '%s' in %s" % (key, obj))
        _TestGetHas(foo)
        _TestGetHas(stageRoot)
        _TestGetHas(attr)
        _TestGetHas(rel)

        # Ensure we can write the reported values
        print "Verify SetMetadata(key, GetMetadata(key))..."
        def _TestSet(obj):
            for key in obj.GetAllMetadata():
                value = obj.GetMetadata(key)
                assert obj.SetMetadata(key, value), \
                     ("Expected to SET metadata key '%s' in %s" % (key, obj))
                AssertEqual(value, obj.GetMetadata(key))
        _TestSet(foo)
        _TestSet(stageRoot)
        _TestSet(attr)
        _TestSet(rel)


def TestHasAuthored():
    print "Verify HasAuthoredMetadata() behavior..."
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestHasAuthored.'+fmt)

        stageRoot = stage.GetPseudoRoot()
        foo = stage.OverridePrim("/Foo")
        attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)

        rel = foo.CreateRelationship("rel")

        # Verify HasAuthored behavior, both for explicitly authored values and 
        # "required" Sdf fields.

        # Prim
        assert foo.HasAuthoredMetadata("specifier")

        # Pseudo-root (has different required metadata from Prim)
        assert not stageRoot.HasAuthoredMetadata("specifier")
        assert not stageRoot.HasAuthoredMetadata("comment")

        # Property
        assert attr.HasAuthoredMetadata("custom")
        assert attr.HasAuthoredMetadata("variability")

        # Attribute
        assert attr.HasAuthoredMetadata("typeName")

        # Relationship (has no explicitly required metadata beyond Property).
        assert rel.HasAuthoredMetadata("variability")
        assert not rel.HasAuthoredMetadata("typeName")

        #
        # Test explicitly authored metadata.
        #
        def _TestExplicit(obj, key, value):
            assert not obj.HasAuthoredMetadata(key), key
            assert not obj.HasMetadata(key), key

            assert obj.SetMetadata(key, value)

            assert obj.HasAuthoredMetadata(key), key
            assert obj.HasMetadata(key), key

        _TestExplicit(foo, "comment", "this is a comment")
        _TestExplicit(attr, "comment", "this is a comment")
        _TestExplicit(rel, "comment", "this is a comment")
        _TestExplicit(rel, "noLoadHint", True)


def TestUnregistered():
    print "Verify that unregistered metadata fields cannot be authored..."
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestUnregistered.'+fmt)

        foo = stage.OverridePrim("/Foo")
        with RequiredException(Tf.ErrorException):
            foo.SetMetadata("unregistered", "x")
            
        attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
        with RequiredException(Tf.ErrorException):
            attr.SetMetadata("unregistered", "x")
            
        rel = foo.CreateRelationship("rel")
        with RequiredException(Tf.ErrorException):
            rel.SetMetadata("unregistered", "x")

def TestCompositionData():
    print "Verify that composition-related metadata does not show up"
    print "in 'All' metadata queries, and only gets basic composition"
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestCompositionData'+fmt)

        base = stage.OverridePrim("/base")
        baseChild = stage.OverridePrim("/base/baseChild")
        apex = stage.OverridePrim("/apex")
        apexChild = stage.OverridePrim("/apex/apexChild")

        apex.GetReferences().Add(stage.GetRootLayer().identifier, "/base")

        AssertEqual(len(apex.GetAllChildren()), 2)
        AssertEqual(len(apex.GetMetadata('primChildren')), 1)
        AssertFalse('primChildren' in apex.GetAllMetadata())
        AssertFalse('references' in apex.GetAllMetadata())

def TestDocumentation():
    print "Test documentation metadata and explicit API..."
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestDocumentation.'+fmt)
        stageRoot = stage.GetPseudoRoot()
        foo = stage.OverridePrim("/Foo")
        attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
        rel = foo.CreateRelationship("rel")

        for obj in [foo, attr, rel]:
            AssertEqual(obj.GetDocumentation(), "")
            AssertFalse(obj.HasAuthoredDocumentation());
            AssertEqual(obj.GetMetadata("documentation"), None)

            AssertEqual(obj.SetDocumentation("foo"), True)
            AssertEqual(obj.GetDocumentation(), "foo")
            AssertTrue(obj.HasAuthoredDocumentation());
            AssertEqual(obj.GetMetadata("documentation"), "foo")

            AssertEqual(obj.SetDocumentation(""), True)
            AssertEqual(obj.GetDocumentation(), "")
            AssertTrue(obj.HasAuthoredDocumentation());
            AssertEqual(obj.GetMetadata("documentation"), "")

            AssertEqual(obj.ClearDocumentation(), True)
            AssertEqual(obj.GetDocumentation(), "")
            AssertFalse(obj.HasAuthoredDocumentation());
            AssertEqual(obj.GetMetadata("documentation"), None)

def TestDisplayName():
    print "Test display name metadata and explicit API..."
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestDisplayName.'+fmt)
        stageRoot = stage.GetPseudoRoot()
        foo = stage.OverridePrim("/Foo")
        attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
        rel = foo.CreateRelationship("rel")

        for prop in [attr, rel]:
            AssertEqual(prop.GetDisplayName(), "")
            AssertFalse(prop.HasAuthoredDisplayName());
            AssertEqual(prop.GetMetadata("displayName"), None)

            AssertEqual(prop.SetDisplayName("foo"), True)
            AssertEqual(prop.GetDisplayName(), "foo")
            AssertTrue(prop.HasAuthoredDisplayName());
            AssertEqual(prop.GetMetadata("displayName"), "foo")

            AssertEqual(prop.SetDisplayName(""), True)
            AssertEqual(prop.GetDisplayName(), "")
            AssertTrue(prop.HasAuthoredDisplayName());
            AssertEqual(prop.GetMetadata("displayName"), "")

            AssertEqual(prop.ClearDisplayName(), True)
            AssertEqual(prop.GetDisplayName(), "")
            AssertFalse(prop.HasAuthoredDisplayName());
            AssertEqual(prop.GetMetadata("displayName"), None)

def TestDisplayGroup():
    print "Test display group metadata and explicit API..."
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestDisplayGroup.'+fmt)
        stageRoot = stage.GetPseudoRoot()
        foo = stage.OverridePrim("/Foo")
        attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.String)
        rel = foo.CreateRelationship("rel")

        for prop in [attr, rel]:
            AssertEqual(prop.GetDisplayGroup(), "")
            AssertFalse(prop.HasAuthoredDisplayGroup());
            AssertEqual(prop.GetMetadata("displayGroup"), None)

            AssertEqual(prop.SetDisplayGroup("foo"), True)
            AssertEqual(prop.GetDisplayGroup(), "foo")
            AssertTrue(prop.HasAuthoredDisplayGroup());
            AssertEqual(prop.GetMetadata("displayGroup"), "foo")

            AssertEqual(prop.SetDisplayGroup(""), True)
            AssertEqual(prop.GetDisplayGroup(), "")
            AssertTrue(prop.HasAuthoredDisplayGroup());
            AssertEqual(prop.GetMetadata("displayGroup"), "")

            AssertEqual(prop.SetNestedDisplayGroups(
                ("foo", "bar", "baz")), True)
            AssertEqual(prop.GetDisplayGroup(), "foo:bar:baz")
            AssertEqual(prop.GetNestedDisplayGroups(), ["foo", "bar", "baz"])
            AssertTrue(prop.HasAuthoredDisplayGroup())
            AssertEqual(prop.GetMetadata("displayGroup"), "foo:bar:baz")

            AssertEqual(prop.ClearDisplayGroup(), True)
            AssertEqual(prop.GetDisplayGroup(), "")
            AssertFalse(prop.HasAuthoredDisplayGroup());
            AssertEqual(prop.GetMetadata("displayGroup"), None)

def TestBasicCustomData():
    '''Test basic CustomData API, including by-key-path API'''

    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestBasicCustomData.'+fmt)
        p = s.OverridePrim('/foo')

        assert not p.HasCustomData()
        assert not p.HasAuthoredCustomData()

        p.SetCustomData({'foo':'bar'})
        assert p.HasCustomData()
        assert p.HasCustomDataKey('foo')
        assert not p.HasCustomDataKey('bar')
        assert p.GetCustomData() == {'foo':'bar'}

        p.SetCustomDataByKey('foo', 'byKey')
        assert p.GetCustomData() == {'foo':'byKey'}

        p.SetCustomDataByKey('newKey', 'value')
        assert p.GetCustomData() == {'foo':'byKey', 'newKey':'value'}

        p.SetCustomDataByKey('a:deep:key:path', 1.2345)
        assert p.GetCustomData() == {
            'foo':'byKey',
            'newKey':'value',
            'a': {
                'deep': {
                    'key': {
                        'path': 1.2345
                        }
                    }
                }
            }

        assert p.GetCustomDataByKey('a:deep') == {
            'key': {
                'path': 1.2345
                }
            }

        p.ClearCustomDataByKey('foo')
        assert not p.HasCustomDataKey('foo')
        assert not p.HasAuthoredCustomDataKey('foo')
        assert p.HasAuthoredCustomDataKey('a:deep:key')
        assert p.HasCustomDataKey('a:deep:key')
        assert p.HasAuthoredCustomDataKey('a')
        assert p.HasCustomDataKey('a')
        assert p.GetCustomData() == {
            'newKey':'value',
            'a': {
                'deep': {
                    'key': {
                        'path': 1.2345
                        }
                    }
                }
            }

        p.ClearCustomDataByKey('a')
        assert p.GetCustomData() == { 'newKey':'value' }

def TestComposedNestedDictionaries():
    '''Test to ensure dictionaries are properly merged.
       They will be recursively merged, with the typical strength
       preferences in place.'''

    print "Verify composed nested dictionary behavior..."
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestComposedNestedDictionaries.'+fmt)
        a = s.DefinePrim('/A')
        a.SetCustomData({ 'sub': { 'otherStr' : 'definedInA' ,
                                   'willCollide' : 'definedInA' } })

        b = s.DefinePrim('/B')
        b.SetCustomData({ 'sub': { 'newStr' : 'definedInB' ,
                                   'willCollide' : 'definedInB' } })
        b.GetReferences().AddInternal(a.GetPath())

        expectedCustomData = {'sub': {'newStr': 'definedInB', 
                                      'willCollide': 'definedInB',
                                      'otherStr': 'definedInA'}}
        assert b.GetCustomData() == expectedCustomData

def TestComposedCustomData():
    '''Test customData composition (dictionary-wise)'''
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestComposedCustomData.'+fmt)

        # Create two prims, 'weaker' and 'stronger', 'stronger' references
        # 'weaker'.
        weaker = s.OverridePrim('/weaker')
        stronger = s.OverridePrim('/stronger')
        stronger.GetReferences().Add(
            s.GetRootLayer().identifier, weaker.GetPath())

        # Values set in weaker should shine through to stronger.
        weaker.SetCustomDataByKey('foo', 'weaker')
        assert stronger.GetCustomData() == {'foo':'weaker'}

        # An empty dict in stronger should not affect composition.
        stronger.SetCustomData({})
        assert stronger.GetCustomData() == {'foo':'weaker'}

        # Set a different key in stronger, dicts should merge.
        stronger.SetCustomDataByKey('bar', 'stronger')
        assert stronger.GetCustomDataByKey('foo') == 'weaker'
        assert stronger.GetCustomDataByKey('bar') == 'stronger'
        assert stronger.GetCustomData() == {'foo':'weaker', 'bar':'stronger'}

        # Override a key from weaker.
        stronger.SetCustomDataByKey('foo', 'stronger')
        assert stronger.GetCustomDataByKey('foo') == 'stronger'
        assert stronger.GetCustomDataByKey('bar') == 'stronger'
        assert stronger.GetCustomData() == {'foo':'stronger', 'bar':'stronger'}

        # Author a weaker key, then clear the stronger to let it shine through.
        weaker.SetCustomDataByKey('bar', 'weaker')
        assert stronger.GetCustomDataByKey('foo') == 'stronger'
        assert stronger.GetCustomDataByKey('bar') == 'stronger'
        assert stronger.GetCustomData() == {'foo':'stronger', 'bar':'stronger'}
        stronger.ClearCustomDataByKey('bar')
        assert stronger.GetCustomDataByKey('foo') == 'stronger'
        assert stronger.GetCustomDataByKey('bar') == 'weaker'
        assert stronger.GetCustomData() == {'foo':'stronger', 'bar':'weaker'}

def TestBasicCustomDataViaMetadataAPI():
    '''Test basic metadata API, including by-key-path API'''

    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestBasicCustomDataViaMetadataAPI.'+fmt)
        p = s.OverridePrim('/foo')

        assert not p.HasMetadata('customData')
        assert not p.HasAuthoredMetadata('customData')

        p.SetMetadata('customData', {'foo':'bar'})
        assert p.HasMetadata('customData')
        assert p.HasMetadataDictKey('customData', 'foo')
        assert not p.HasMetadataDictKey('customData', 'bar')
        assert p.GetMetadata('customData') == {'foo':'bar'}

        p.SetMetadataByDictKey('customData', 'foo', 'byKey')
        assert p.GetMetadata('customData') == {'foo':'byKey'}

        p.SetMetadataByDictKey('customData', 'newKey', 'value')
        assert p.GetMetadata('customData') == {'foo':'byKey', 'newKey':'value'}

        p.SetMetadataByDictKey('customData', 'a:deep:key:path', 1.2345)
        assert p.GetMetadata('customData') == {
            'foo':'byKey',
            'newKey':'value',
            'a': {
                'deep': {
                    'key': {
                        'path': 1.2345
                        }
                    }
                }
            }

        assert p.GetMetadataByDictKey('customData', 'a:deep') == {
            'key': {
                'path': 1.2345
                }
            }

        p.ClearMetadataByDictKey('customData', 'foo')
        assert not p.HasMetadataDictKey('customData', 'foo')
        assert not p.HasAuthoredMetadataDictKey('customData', 'foo')
        assert p.HasAuthoredMetadataDictKey('customData', 'a:deep:key')
        assert p.HasMetadataDictKey('customData', 'a:deep:key')
        assert p.HasAuthoredMetadataDictKey('customData', 'a')
        assert p.HasMetadataDictKey('customData', 'a')
        assert p.GetMetadata('customData') == {
            'newKey':'value',
            'a': {
                'deep': {
                    'key': {
                        'path': 1.2345
                        }
                    }
                }
            }

        p.ClearMetadataByDictKey('customData', 'a')
        assert p.GetMetadata('customData') == { 'newKey':'value' }

def TestComposedCustomDataViaMetadataAPI():
    '''Test customData composition (dictionary-wise)'''
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory(
            'TestComposedCustomDataViaMetadataAPI.'+fmt)

        # Create two prims, 'weaker' and 'stronger', 'stronger' references
        # 'weaker'.
        weaker = s.OverridePrim('/weaker')
        stronger = s.OverridePrim('/stronger')
        stronger.GetReferences().Add(
            s.GetRootLayer().identifier, weaker.GetPath())

        # Values set in weaker should shine through to stronger.
        weaker.SetMetadataByDictKey('customData', 'foo', 'weaker')
        assert stronger.GetMetadata('customData') == {'foo':'weaker'}

        # An empty dict in stronger should not affect composition.
        stronger.SetMetadata('customData', {})
        assert stronger.GetMetadata('customData') == {'foo':'weaker'}

        # Set a different key in stronger, dicts should merge.
        stronger.SetMetadataByDictKey('customData', 'bar', 'stronger')
        assert stronger.GetMetadataByDictKey('customData', 'foo') == 'weaker'
        assert stronger.GetMetadataByDictKey('customData', 'bar') == 'stronger'
        assert stronger.GetMetadata('customData') == {
            'foo':'weaker', 'bar':'stronger'}

        # Override a key from weaker.
        stronger.SetMetadataByDictKey('customData', 'foo', 'stronger')
        assert stronger.GetMetadataByDictKey('customData', 'foo') == 'stronger'
        assert stronger.GetMetadataByDictKey('customData', 'bar') == 'stronger'
        assert stronger.GetMetadata('customData') == {
            'foo':'stronger', 'bar':'stronger'}

        # Author a weaker key, then clear the stronger to let it shine through.
        weaker.SetMetadataByDictKey('customData', 'bar', 'weaker')
        assert stronger.GetMetadataByDictKey('customData', 'foo') == 'stronger'
        assert stronger.GetMetadataByDictKey('customData', 'bar') == 'stronger'
        assert stronger.GetMetadata('customData') == {
            'foo':'stronger', 'bar':'stronger'}
        stronger.ClearMetadataByDictKey('customData', 'bar')
        assert stronger.GetMetadataByDictKey('customData', 'foo') == 'stronger'
        assert stronger.GetMetadataByDictKey('customData', 'bar') == 'weaker'
        assert stronger.GetMetadata('customData') == {
            'foo':'stronger', 'bar':'weaker'}

def TestBasicRequiredFields():
    ('Ensure only expected required fields are returned by '
     'GetAllAuthoredMetadata, test for bug 98419')
    for fmt in allFormats:
        a = Usd.Stage.CreateInMemory('TestBasicRequiredFields.'+fmt)
        b = a.OverridePrim('/hello')
        attr = b.CreateAttribute('foo', Sdf.ValueTypeNames.Double)
        attr.Set(1.234)
        ins = ['typeName', 'custom', 'variability']
        outs = ['allowedTokens']
        metadata = attr.GetAllAuthoredMetadata()
        for key in ins:
            assert key in metadata, ("expected %s in %s" % (key, metadata))
        for key in outs:
            assert key not in metadata, ("expected %s not in %s" %
                                         (key, metadata))

def TestBasicListOpMetadata():
    '''Tests basic metadata API with supported list op types'''
    def _TestBasic(fieldName, listOp, expectedListOp):
        for fmt in allFormats:
            s = Usd.Stage.CreateNew("TestBasicListOpMetadata."+fmt)

            prim = s.OverridePrim('/Root')

            assert not prim.HasMetadata(fieldName)
            assert not prim.HasAuthoredMetadata(fieldName)
            AssertEqual(prim.GetMetadata(fieldName), None)

            assert prim.SetMetadata(fieldName, listOp)
            assert prim.HasMetadata(fieldName)
            assert prim.HasAuthoredMetadata(fieldName)
            AssertEqual(prim.GetMetadata(fieldName), expectedListOp)
    
            prim.ClearMetadata(fieldName)
            assert not prim.HasMetadata(fieldName)
            assert not prim.HasAuthoredMetadata(fieldName)
            AssertEqual(prim.GetMetadata(fieldName), None)

            s.Close()

    # List ops are applied into a single explicit list op during
    # value resolution, so the expected list op isn't the same
    # as the given list op.

    # Sdf.IntListOp
    listOp = Sdf.IntListOp()
    listOp.addedItems = [-2147483648, 1, 2, 3, 2147483647]
    listOp.deletedItems = [-2147483648, 10, 20, 30, 2147483647]
    listOp.orderedItems = [2147483647, 3, 2, 1, -2147483648]

    expectedListOp = Sdf.IntListOp()
    expectedListOp.explicitItems = [2147483647, 3, 2, 1, -2147483648]
    _TestBasic('intListOpTest', listOp, expectedListOp)

    # Sdf.Int64ListOp
    listOp = Sdf.Int64ListOp()
    listOp.addedItems = [-9223372036854775808, 1, 2, 3, 9223372036854775807]
    listOp.deletedItems = [-9223372036854775808, 10, 20, 30, 9223372036854775807]
    listOp.orderedItems = [9223372036854775807, 3, 2, 1, -9223372036854775808]

    expectedListOp = Sdf.Int64ListOp()
    expectedListOp.explicitItems = [9223372036854775807, 3, 2, 1, -9223372036854775808]
    _TestBasic('int64ListOpTest', listOp, expectedListOp)

    # Sdf.UIntListOp
    listOp = Sdf.UIntListOp()
    listOp.addedItems = [1, 2, 3, 4294967295]
    listOp.deletedItems = [10, 20, 30, 4294967295]
    listOp.orderedItems = [4294967295, 3, 2, 1]

    expectedListOp = Sdf.UIntListOp()
    expectedListOp.explicitItems = [4294967295, 3, 2, 1]
    _TestBasic('uintListOpTest', listOp, expectedListOp)

    # Sdf.UInt64ListOp
    listOp = Sdf.UInt64ListOp()
    listOp.addedItems = [1, 2, 3, 18446744073709551615]
    listOp.deletedItems = [10, 20, 30, 18446744073709551615]
    listOp.orderedItems = [18446744073709551615, 3, 2, 1]

    expectedListOp = Sdf.UInt64ListOp()
    expectedListOp.explicitItems = [18446744073709551615, 3, 2, 1]
    _TestBasic('uint64ListOpTest', listOp, expectedListOp)

    # Sdf.StringListOp
    listOp = Sdf.StringListOp()
    listOp.addedItems = ["foo", "bar"]
    listOp.deletedItems = ["baz"]
    listOp.orderedItems = ["bar", "foo"]

    expectedListOp = Sdf.StringListOp()
    expectedListOp.explicitItems = ["bar", "foo"]
    _TestBasic('stringListOpTest', listOp, expectedListOp)

    # Sdf.TokenListOp
    listOp = Sdf.TokenListOp()
    listOp.addedItems = ["foo", "bar"]
    listOp.deletedItems = ["baz"]
    listOp.orderedItems = ["bar", "foo"]

    expectedListOp = Sdf.TokenListOp()
    expectedListOp.explicitItems = ["bar", "foo"]
    _TestBasic('tokenListOpTest', listOp, expectedListOp)

def TestComposedListOpMetadata():
    '''Tests composition of list op-valued metadata fields'''
    def _TestComposition(fieldName, weakListOp, strongListOp,
                         expectedListOp):
        for fmt in allFormats:
            s = Usd.Stage.CreateNew("TestComposedListOpMetadata."+fmt)

            ref = s.OverridePrim('/Ref')
            ref.SetMetadata(fieldName, weakListOp)

            root = s.OverridePrim('/Root')
            root.SetMetadata(fieldName, strongListOp)
            root.GetReferences().AddInternal('/Ref')

            AssertEqual(root.GetMetadata(fieldName), expectedListOp)
            s.Close()

    # Sdf.IntListOp
    weakListOp = Sdf.IntListOp()
    weakListOp.explicitItems = [10, 20, 30]

    strongListOp = Sdf.IntListOp()
    strongListOp.addedItems = [-2147483648, 1, 2, 3, 2147483647]
    strongListOp.deletedItems = [-2147483648, 10, 20, 30, 2147483647]
    strongListOp.orderedItems = [2147483647, 3, 2, 1, -2147483648]

    expectedListOp = Sdf.IntListOp()
    expectedListOp.explicitItems = [2147483647, 3, 2, 1, -2147483648]
    _TestComposition('intListOpTest', 
                     weakListOp, strongListOp, expectedListOp)

    # Sdf.Int64ListOp
    weakListOp = Sdf.Int64ListOp()
    weakListOp.explicitItems = [10, 20, 30]

    strongListOp = Sdf.Int64ListOp()
    strongListOp.addedItems = [-9223372036854775808, 1, 2, 3, 9223372036854775807]
    strongListOp.deletedItems = [-9223372036854775808, 10, 20, 30, 9223372036854775807]
    strongListOp.orderedItems = [9223372036854775807, 3, 2, 1, -9223372036854775808]

    expectedListOp = Sdf.Int64ListOp()
    expectedListOp.explicitItems = [9223372036854775807, 3, 2, 1, -9223372036854775808]
    _TestComposition('int64ListOpTest', 
                     weakListOp, strongListOp, expectedListOp)

    # Sdf.UIntListOp
    weakListOp = Sdf.UIntListOp()
    weakListOp.explicitItems = [10, 20, 30]

    strongListOp = Sdf.UIntListOp()
    strongListOp.addedItems = [1, 2, 3, 4294967295]
    strongListOp.deletedItems = [10, 20, 30, 4294967295]
    strongListOp.orderedItems = [4294967295, 3, 2, 1]

    expectedListOp = Sdf.UIntListOp()
    expectedListOp.explicitItems = [4294967295, 3, 2, 1]
    _TestComposition('uintListOpTest', 
                     weakListOp, strongListOp, expectedListOp)

    # Sdf.UInt64ListOp
    weakListOp = Sdf.UInt64ListOp()
    weakListOp.explicitItems = [10, 20, 30]

    strongListOp = Sdf.UInt64ListOp()
    strongListOp.addedItems = [1, 2, 3, 18446744073709551615]
    strongListOp.deletedItems = [10, 20, 30, 18446744073709551615]
    strongListOp.orderedItems = [18446744073709551615, 3, 2, 1]

    expectedListOp = Sdf.UInt64ListOp()
    expectedListOp.explicitItems = [18446744073709551615, 3, 2, 1]
    _TestComposition('uint64ListOpTest', 
                     weakListOp, strongListOp, expectedListOp)
    
    # Sdf.StringListOp
    weakListOp = Sdf.StringListOp()
    weakListOp.explicitItems = ["baz"]

    strongListOp = Sdf.StringListOp()
    strongListOp.addedItems = ["foo", "bar"]
    strongListOp.deletedItems = ["baz"]
    strongListOp.orderedItems = ["bar", "foo"]

    expectedListOp = Sdf.StringListOp()
    expectedListOp.explicitItems = ["bar", "foo"]
    _TestComposition('stringListOpTest', 
                     weakListOp, strongListOp, expectedListOp)

    # Sdf.TokenListOp
    weakListOp = Sdf.TokenListOp()
    weakListOp.explicitItems = ["baz"]

    strongListOp = Sdf.TokenListOp()
    strongListOp.addedItems = ["foo", "bar"]
    strongListOp.deletedItems = ["baz"]
    strongListOp.orderedItems = ["bar", "foo"]

    expectedListOp = Sdf.TokenListOp()
    expectedListOp.explicitItems = ["bar", "foo"]
    _TestComposition('tokenListOpTest', 
                     weakListOp, strongListOp, expectedListOp)

def TestUnknownFieldsRoundTripThroughUsdc():
    import tempfile, difflib
    s = Usd.Stage.CreateInMemory('testBadFields.usda')
    s.GetRootLayer().ImportFromString('''#usda 1.0
(
    delete badListOpTest = [10, 20, 30]
    add badListOpTest = [1, 2, 3]
    reorder badListOpTest = [3, 2, 1]
    noSuchListOpTest = [10, 20, 30]
)

def "BadDict" (
    badDictTest = {
        bool someBool = 1
        double[] someDoubleArray = [0, 1]
    }
)
{
}

def "BadListOp"
{
    double ExplicitAttr (
        badListOpTest = [-2147483648, 1, 2, 3, 4, 5, 2147483647]
    )
    rel ExplicitRel (
        badListOpTest = [-2147483648, 1, 2, 3, 4, 5, 2147483647]
    )
    double NoneAttr (
        badListOpTest = None
    )
    rel NoneRel (
        badListOpTest = None
    )
    double NonExplicitAttr (
        delete badListOpTest = [-2147483648, 10, 20, 30, 2147483647]
        add badListOpTest = [-2147483648, 1, 2, 3, 2147483647]
        reorder badListOpTest = [2147483647, 3, 2, 1, -2147483648]
    )
    rel NonExplicitRel (
        delete badListOpTest = [-2147483648, 10, 20, 30, 2147483647]
        add badListOpTest = [-2147483648, 1, 2, 3, 2147483647]
        reorder badListOpTest = [2147483647, 3, 2, 1, -2147483648]
    )

    def "Explicit" (
        badListOpTest = [-2147483648, 1, 2, 3, 4, 5, 2147483647]
    )
    {
    }

    def "None" (
        badListOpTest = None
    )
    {
    }

    def "NonExplicit" (
        delete badListOpTest = [-2147483648, 10, 20, 30, 2147483647]
        add badListOpTest = [-2147483648, 1, 2, 3, 2147483647]
        reorder badListOpTest = [2147483647, 3, 2, 1, -2147483648]
    )
    {
    }
}

def "Foo" (
    nosuchfield = 1234
)
{
}
''')
    # Now export to both .usdc and .usda, then export the usdc to usda, and
    # compare -- they should match.
    with tempfile.NamedTemporaryFile(suffix='.usda') as textFile, \
         tempfile.NamedTemporaryFile(suffix='.usda') as roundTripFile, \
         tempfile.NamedTemporaryFile(suffix='.usdc') as binFile:
        s.GetRootLayer().Export(textFile.name)
        s.GetRootLayer().Export(binFile.name)
        binLayer = Sdf.Layer.FindOrOpen(binFile.name)
        assert binLayer
        binLayer.Export(roundTripFile.name)

        # Now textFile and roundTripFile should match.
        a = open(textFile.name).read()
        b = open(roundTripFile.name).read()
        if a != b:
            print '\n'.join(difflib.unified_diff(a.split('\n'), b.split('\n')))
        assert a == b

if __name__ == '__main__':
    Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)

    # Register test plugin defining list op metadata fields.
    testDir = FindDataFile('testUsdMetadata')
    assert len(Plug.Registry().RegisterPlugins(testDir)) == 1

    TestHidden()
    TestListAndHas()
    TestHasAuthored()
    TestUnregistered()
    TestCompositionData()
    TestDocumentation()
    TestDisplayName()
    TestDisplayGroup()
    TestBasicCustomData()
    TestComposedCustomData()
    TestComposedNestedDictionaries()
    TestBasicCustomDataViaMetadataAPI()
    TestComposedCustomDataViaMetadataAPI()
    TestBasicRequiredFields()
    TestBasicListOpMetadata()
    TestComposedListOpMetadata()
    TestUnknownFieldsRoundTripThroughUsdc()
