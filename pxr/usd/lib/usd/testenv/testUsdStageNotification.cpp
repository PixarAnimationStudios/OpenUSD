//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include <Python.h>
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <cstdio>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using boost::bind;
using boost::function;

using std::vector;

typedef boost::function<bool (const UsdNotice::ObjectsChanged &)> TestFn;

struct _NoticeTester : public TfWeakBase
{
    _NoticeTester(const UsdStageWeakPtr &stage) : _stage(stage) {
        TfNotice::Register(
            TfCreateWeakPtr(this), &_NoticeTester::_Handle, stage);
    }

    template <class Fn>
    void AddTest(const Fn &testFn) { _testFns.push_back(testFn); }

private:
    void _Handle(const UsdNotice::ObjectsChanged &n,
                 const UsdStageWeakPtr &sender) {
        printf(
            "Received notice. ResyncedPaths: %s, ChangedInfoOnlyPaths: %s\n",
            TfStringify(n.GetResyncedPaths()).c_str(),
            TfStringify(n.GetChangedInfoOnlyPaths()).c_str());
        TF_AXIOM(sender == _stage);
        for (const auto& fn : _testFns) {
            TF_AXIOM(fn(n));
        }
    }

    UsdStageWeakPtr _stage;
    std::vector<TestFn> _testFns;
};

void
TestObjectsChanged()
{
    typedef UsdNotice::ObjectsChanged Notice;

    // Test that:
    // - Simple resyncs.
    // - Ancestral resyncs subsume descendant ones.
    // - Info changes.
    // - Resyncs subsume info changes.

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    SdfLayerHandle rootLayer = stage->GetRootLayer();

    UsdPrim foo = stage->OverridePrim(SdfPath("/foo"));

    // Change foo's typename, assert that it gets resynced.
    {
        printf("Changing /foo should resync it\n");
        _NoticeTester tester(stage);
        tester.AddTest(bind(&Notice::ResyncedObject, _1, foo));
        rootLayer->GetPrimAtPath(SdfPath("/foo"))->SetTypeName("Scope");
    }

    // Add a child prim, assert that both are considered resynced when changing
    // the parent.
    UsdPrim bar = stage->OverridePrim(SdfPath("/foo/bar"));
    {
        printf("Changing /foo should consider /foo and /foo/bar resync'd\n");
        _NoticeTester tester(stage);
        tester.AddTest(bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(bind(&Notice::ResyncedObject, _1, bar));
        rootLayer->GetPrimAtPath(SdfPath("/foo"))->SetTypeName("");
    }

    // Assert that changing bar doesn't resync foo.
    {
        printf("Changing /foo/bar shouldn't resync /foo\n");
        _NoticeTester tester(stage);
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(bind(&Notice::ResyncedObject, _1, bar));
        rootLayer->GetPrimAtPath(SdfPath("/foo/bar"))->SetTypeName("Scope");
    }

    // Assert that changing both foo and bar works as expected.
    {
        printf("Changing both /foo and /foo/bar should resync just /foo\n");
        _NoticeTester tester(stage);
        tester.AddTest(bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(bind(&Notice::ResyncedObject, _1, bar));
        tester.AddTest(
            bind(&SdfPathVector::size,
                 bind(&Notice::GetResyncedPaths, _1)) == 1);
        { SdfChangeBlock block;
            rootLayer->GetPrimAtPath(SdfPath("/foo"))->SetTypeName("Scope");
            rootLayer->GetPrimAtPath(SdfPath("/foo/bar"))->SetTypeName("");
        }
    }

    // Assert that adding a reference resyncs the prims in a reference chain.
    UsdPrim
        target1 = stage->OverridePrim(SdfPath("/target1")),
        target2 = stage->OverridePrim(SdfPath("/target2"));
    foo.GetReferences().AddReference(rootLayer->GetIdentifier(),
                                        target1.GetPath());
    {
        printf("adding reference target1 -> target2 should resync target1 and "
               "foo, but not target2\n");
        _NoticeTester tester(stage);
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, target2));
        tester.AddTest(bind(&Notice::ResyncedObject, _1, target1));
        tester.AddTest(bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(bind(&Notice::ResyncedObject, _1, bar));
        // Now add the reference.
        target1.GetReferences().AddReference(rootLayer->GetIdentifier(),
                                                target2.GetPath());
    }

    // Assert that changing an inherited value causes changes to instances.
    UsdPrim cls = stage->CreateClassPrim(SdfPath("/cls"));
    foo.GetInherits().AddInherit(cls.GetPath());
    bar.GetInherits().AddInherit(cls.GetPath());
    {
        printf("changing info in cls should cause info change in foo & bar\n");
        _NoticeTester tester(stage);
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, bar));
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, cls));
        tester.AddTest(bind(&Notice::AffectedObject, _1, cls));
        tester.AddTest(bind(&Notice::AffectedObject, _1, foo));
        tester.AddTest(bind(&Notice::AffectedObject, _1, bar));
        tester.AddTest(bind(&Notice::ChangedInfoOnly, _1, cls));
        tester.AddTest(bind(&Notice::ChangedInfoOnly, _1, foo));
        tester.AddTest(bind(&Notice::ChangedInfoOnly, _1, bar));
        cls.SetMetadata(SdfFieldKeys->Documentation, "cls doc");
    }

    // Assert that changing specializes causes changes to instances.
    UsdPrim specialize = stage->DefinePrim(SdfPath("/spec"));
    foo.GetSpecializes().AddSpecialize(specialize.GetPath());
    bar.GetSpecializes().AddSpecialize(specialize.GetPath());
    {
        printf("changing info in spec should cause info change in foo & bar\n");
        _NoticeTester tester(stage);
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, bar));
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, cls));
        tester.AddTest(bind(&Notice::AffectedObject, _1, cls));
        tester.AddTest(bind(&Notice::AffectedObject, _1, foo));
        tester.AddTest(bind(&Notice::AffectedObject, _1, bar));
        tester.AddTest(bind(&Notice::ChangedInfoOnly, _1, cls));
        tester.AddTest(bind(&Notice::ChangedInfoOnly, _1, foo));
        tester.AddTest(bind(&Notice::ChangedInfoOnly, _1, bar));
        cls.SetMetadata(SdfFieldKeys->Documentation, "cls spec doc");
    }

    // Assert that changes to non-composition related metadata fields come
    // through as info changes rather than resyncs.
    {
        printf("Setting prim doc should cause info change, but no resync\n");
        _NoticeTester tester(stage);
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(bind(&Notice::ChangedInfoOnly, _1, foo));
        tester.AddTest(bind(&Notice::AffectedObject, _1, foo));
        foo.SetMetadata(SdfFieldKeys->Documentation, "hello doc");
    }

    // Assert that resyncs subsume info changes.
    {
        printf("Setting prim doc and typename in one go should cause a "
               "resync\n");
        _NoticeTester tester(stage);
        tester.AddTest(bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(!bind(&Notice::ChangedInfoOnly, _1, foo));
        tester.AddTest(bind(&Notice::AffectedObject, _1, foo));
        { SdfChangeBlock block;
            rootLayer->GetPrimAtPath(SdfPath("/foo"))->SetTypeName("Cube");
            rootLayer->GetPrimAtPath(
                SdfPath("/foo"))->SetDocumentation("Cubicle");
        }
    }

    // Assert that creating an attribute causes resyncs
    UsdAttribute attr;
    {
        printf("Creating an attribute should cause a resync\n");
        _NoticeTester tester(stage);
        tester.AddTest([](const UsdNotice::ObjectsChanged& n) {
            return (n.GetResyncedPaths() == SdfPathVector{SdfPath("/foo.attr")} 
                    && n.GetChangedInfoOnlyPaths() == SdfPathVector{});
        });
        attr = foo.CreateAttribute(TfToken("attr"), SdfValueTypeNames->Int);
    }

    // Assert that changing an attribute value causes info changes.
    {
        printf("Setting an attribute value should cause info change\n");
        _NoticeTester tester(stage);
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, attr));
        tester.AddTest(bind(&Notice::ChangedInfoOnly, _1, attr));
        tester.AddTest(bind(&Notice::AffectedObject, _1, attr));
        tester.AddTest([](const UsdNotice::ObjectsChanged& n) {
            return (n.GetResyncedPaths() == SdfPathVector{} 
                    && n.GetChangedInfoOnlyPaths() == 
                        SdfPathVector{SdfPath("/foo.attr")});
        });

        attr.Set(42);
    }

    // Assert that creating a relationship causes resyncs
    UsdRelationship rel;
    {
        printf("Creating a relationship should cause a resync\n");
        _NoticeTester tester(stage);
        tester.AddTest([](const UsdNotice::ObjectsChanged& n) {
            return (n.GetResyncedPaths() == SdfPathVector{SdfPath("/foo.rel")} 
                    && n.GetChangedInfoOnlyPaths() == SdfPathVector{});
        });
        rel = foo.CreateRelationship(TfToken("rel"));
    }

    // Assert that setting a relationship target causes info changes.
    {
        printf("Changing relationship targets should cause info change\n");
        _NoticeTester tester(stage);
        tester.AddTest(!bind(&Notice::ResyncedObject, _1, rel));
        tester.AddTest(bind(&Notice::ChangedInfoOnly, _1, rel));
        tester.AddTest(bind(&Notice::AffectedObject, _1, rel));

        tester.AddTest([](const UsdNotice::ObjectsChanged& n) {
            // XXX: This is a bug -- resynced paths should not include
            // the relationship target path, since no such object exists
            // in the USD scenegraph.
            return (n.GetResyncedPaths() == 
                        SdfPathVector{SdfPath("/foo.rel[/bar]")} 
                    && n.GetChangedInfoOnlyPaths() == 
                        SdfPathVector{SdfPath("/foo.rel")});
        });
        rel.AddTarget(SdfPath("/bar"));
    }
}

int main()
{
    TfErrorMark m;

    TestObjectsChanged();

    TF_AXIOM(m.IsClean());

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TF_AXIOM(!Py_IsInitialized());
#endif // PXR_PYTHON_SUPPORT_ENABLED
}
