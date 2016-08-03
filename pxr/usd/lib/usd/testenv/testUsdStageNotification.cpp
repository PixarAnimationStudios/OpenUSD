#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/foreach.hpp>

#include <Python.h>

#include <cstdio>
#include <vector>

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
        BOOST_FOREACH(const TestFn &fn, _testFns) {
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
        tester.AddTest(not bind(&Notice::ResyncedObject, _1, foo));
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
    foo.GetReferences().Add(rootLayer->GetIdentifier(), target1.GetPath());
    {
        printf("adding reference target1 -> target2 should resync target1 and "
               "foo, but not target2\n");
        _NoticeTester tester(stage);
        tester.AddTest(not bind(&Notice::ResyncedObject, _1, target2));
        tester.AddTest(bind(&Notice::ResyncedObject, _1, target1));
        tester.AddTest(bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(bind(&Notice::ResyncedObject, _1, bar));
        // Now add the reference.
        target1.GetReferences().Add(rootLayer->GetIdentifier(),
                                    target2.GetPath());
    }

    // Assert that changing an inherited value causes changes to instances.
    UsdPrim cls = stage->CreateClassPrim(SdfPath("/cls"));
    foo.GetInherits().Add(cls.GetPath());
    bar.GetInherits().Add(cls.GetPath());
    {
        printf("changing info in cls should cause info change in foo & bar\n");
        _NoticeTester tester(stage);
        tester.AddTest(not bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(not bind(&Notice::ResyncedObject, _1, bar));
        tester.AddTest(not bind(&Notice::ResyncedObject, _1, cls));
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
    foo.GetSpecializes().Add(specialize.GetPath());
    bar.GetSpecializes().Add(specialize.GetPath());
    {
        printf("changing info in spec should cause info change in foo & bar\n");
        _NoticeTester tester(stage);
        tester.AddTest(not bind(&Notice::ResyncedObject, _1, foo));
        tester.AddTest(not bind(&Notice::ResyncedObject, _1, bar));
        tester.AddTest(not bind(&Notice::ResyncedObject, _1, cls));
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
        tester.AddTest(not bind(&Notice::ResyncedObject, _1, foo));
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
        tester.AddTest(not bind(&Notice::ChangedInfoOnly, _1, foo));
        tester.AddTest(bind(&Notice::AffectedObject, _1, foo));
        { SdfChangeBlock block;
            rootLayer->GetPrimAtPath(SdfPath("/foo"))->SetTypeName("Cube");
            rootLayer->GetPrimAtPath(
                SdfPath("/foo"))->SetDocumentation("Cubicle");
        }
    }
}

int main()
{
    TfErrorMark m;

    TestObjectsChanged();

    TF_AXIOM(m.IsClean());

    TF_AXIOM(not Py_IsInitialized());
}
