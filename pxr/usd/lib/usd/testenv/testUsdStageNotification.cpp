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
#include "pxr/usd/usd/editContext.h"
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

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include <boost/python/detail/wrap_python.hpp>
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <cstdio>
#include <functional>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using std::vector;
using namespace std::placeholders;

typedef std::function<bool (const UsdNotice::ObjectsChanged &)> TestFn;

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
        printf("Received notice.\n");

        printf("Resynced paths:\n");
        auto resyncedPaths = n.GetResyncedPaths();
        for (auto it = resyncedPaths.begin(), end = resyncedPaths.end();
             it != end; ++it) {
            printf("  - %s\n", it->GetText());
            printf("    changed fields: %s\n", 
                   TfStringify(it.GetChangedFields()).c_str());
        }        

        printf("ChangedInfoOnly paths:\n");
        auto changedInfoPaths = n.GetChangedInfoOnlyPaths();
        for (auto it = changedInfoPaths.begin(), end = changedInfoPaths.end();
             it != end; ++it) {
            printf("  - %s\n", it->GetText());
            printf("    changed fields: %s\n", 
                   TfStringify(it.GetChangedFields()).c_str());
        }        

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

    SdfLayerRefPtr subLayer = SdfLayer::CreateAnonymous(".usda");
    rootLayer->InsertSubLayerPath(subLayer->GetIdentifier());

    // Add a new override prim, assert that it's considered a resync.
    {
        printf("Adding a new override prim /over should be a resync\n");
        _NoticeTester tester(stage);
        tester.AddTest([stage](Notice const &n) {
                UsdPrim newPrim = stage->GetPrimAtPath(SdfPath("/over"));
                return TF_AXIOM(n.ResyncedObject(newPrim));
            });
        tester.AddTest([stage](Notice const &n) {
                return 
                    TF_AXIOM(n.GetChangedFields(SdfPath("/over")).empty()) &&
                    TF_AXIOM(!n.HasChangedFields(SdfPath("/over")));
            });
        UsdEditContext context(
            stage, stage->GetEditTargetForLocalLayer(subLayer));
        UsdPrim over = stage->OverridePrim(SdfPath("/over"));
    }

    // Add an inert spec for /over, assert that it's *not* considered a resync.
    UsdPrim over = stage->GetPrimAtPath(SdfPath("/over"));
    {
        printf("Adding an inert spec for /over should not be a resync\n");
        _NoticeTester tester(stage);
        tester.AddTest([over](Notice const &n) {
                return 
                    TF_AXIOM(!n.ResyncedObject(over)) &&
                    TF_AXIOM(n.ChangedInfoOnly(over));
            });
        tester.AddTest([stage](Notice const &n) {
                return 
                    TF_AXIOM(n.GetChangedFields(SdfPath("/over")).empty()) &&
                    TF_AXIOM(!n.HasChangedFields(SdfPath("/over")));
            });
        SdfCreatePrimInLayer(rootLayer, SdfPath("/over"));
    }

    // Change foo's typename, assert that it gets resynced.
    UsdPrim foo = stage->OverridePrim(SdfPath("/foo"));
    {
        printf("Changing /foo should resync it\n");
        _NoticeTester tester(stage);
        tester.AddTest([foo](Notice const &n) {
                return TF_AXIOM(n.ResyncedObject(foo));                
            });
        tester.AddTest([foo](Notice const &n) {
                return TF_AXIOM(n.HasChangedFields(foo)) &&
                       TF_AXIOM(n.GetChangedFields(foo) == 
                                TfTokenVector{SdfFieldKeys->TypeName});
            });
        rootLayer->GetPrimAtPath(SdfPath("/foo"))->SetTypeName("Scope");
    }

    // Add a child prim, assert that both are considered resynced when changing
    // the parent.
    UsdPrim bar = stage->OverridePrim(SdfPath("/foo/bar"));
    {
        printf("Changing /foo should consider /foo and /foo/bar resync'd\n");
        _NoticeTester tester(stage);
        tester.AddTest([foo, bar](Notice const &n) {
                return
                    TF_AXIOM(n.ResyncedObject(foo)) &&
                    TF_AXIOM(n.ResyncedObject(bar));
            });
        tester.AddTest([foo, bar](Notice const &n) {
                return
                    TF_AXIOM(n.HasChangedFields(foo)) &&
                    TF_AXIOM(n.GetChangedFields(foo) == 
                             TfTokenVector{SdfFieldKeys->TypeName}) &&
                    TF_AXIOM(!n.HasChangedFields(bar)) &&
                    TF_AXIOM(n.GetChangedFields(bar).empty());
            });
        rootLayer->GetPrimAtPath(SdfPath("/foo"))->SetTypeName("");
    }

    // Assert that changing foo's typeName and other metadata together causes
    // a resync with both changed fields reported.
    {
        printf("Changing typeName and metadata on /foo should resync\n");
        _NoticeTester tester(stage);
        tester.AddTest([foo](Notice const &n) {
                return TF_AXIOM(n.ResyncedObject(foo));
            });
        tester.AddTest([foo](Notice const &n) {
                return 
                    TF_AXIOM(n.HasChangedFields(foo)) &&
                    TF_AXIOM(n.GetChangedFields(foo) == 
                             (TfTokenVector{SdfFieldKeys->Documentation,
                                            SdfFieldKeys->TypeName}));
            });
        {
            SdfChangeBlock block;
            rootLayer->GetPrimAtPath(SdfPath("/foo"))->SetTypeName("Sphere");
            rootLayer->GetPrimAtPath(SdfPath("/foo"))->SetDocumentation(
                "Test docs");
        }
    }

    // Assert that changing bar doesn't resync foo.
    {
        printf("Changing /foo/bar shouldn't resync /foo\n");
        _NoticeTester tester(stage);
        tester.AddTest([foo, bar](Notice const &n) {
                return TF_AXIOM(!n.ResyncedObject(foo)) &&
                    TF_AXIOM(n.ResyncedObject(bar));
            });
        tester.AddTest([foo, bar](Notice const &n) {
                return 
                    TF_AXIOM(!n.HasChangedFields(foo)) &&
                    TF_AXIOM(n.GetChangedFields(foo).empty() &&
                    TF_AXIOM(n.HasChangedFields(bar)) &&
                    TF_AXIOM(n.GetChangedFields(bar) == 
                             TfTokenVector{SdfFieldKeys->TypeName}));

            });
        rootLayer->GetPrimAtPath(SdfPath("/foo/bar"))->SetTypeName("Scope");
    }

    // Assert that changing both foo and bar works as expected.
    {
        printf("Changing both /foo and /foo/bar should resync just /foo\n");
        _NoticeTester tester(stage);
        tester.AddTest([foo, bar](Notice const &n) {
                return
                    TF_AXIOM(n.ResyncedObject(foo)) &&
                    TF_AXIOM(n.ResyncedObject(bar)) &&
                    TF_AXIOM(n.GetResyncedPaths().size() == 1);
            });
        tester.AddTest([foo, bar](Notice const &n) {
                return
                    TF_AXIOM(n.HasChangedFields(foo)) &&
                    TF_AXIOM(n.GetChangedFields(foo) == 
                             TfTokenVector{SdfFieldKeys->TypeName}) &&
                    TF_AXIOM(!n.HasChangedFields(bar)) &&
                    TF_AXIOM(n.GetChangedFields(bar).empty());
            });
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
        tester.AddTest([target1, target2, foo, bar](Notice const &n) {
                return
                    TF_AXIOM(!n.ResyncedObject(target2)) &&
                    TF_AXIOM(n.ResyncedObject(target1)) &&
                    TF_AXIOM(n.ResyncedObject(foo)) &&
                    TF_AXIOM(n.ResyncedObject(bar));
            });
        tester.AddTest([target1, target2, foo, bar](Notice const &n) {
                return
                    TF_AXIOM(!n.HasChangedFields(target2)) &&
                    TF_AXIOM(n.GetChangedFields(target2).empty()) &&
                    TF_AXIOM(!n.HasChangedFields(target1)) &&
                    TF_AXIOM(n.GetChangedFields(target1).empty()) &&
                    TF_AXIOM(!n.HasChangedFields(foo)) &&
                    TF_AXIOM(n.GetChangedFields(foo).empty()) &&

                    // XXX: Sdf currently does not report affected fields for
                    // certain types of changes. Once that's fixed, these test
                    // cases can replace the ones above.
                    //
                    // TF_AXIOM(n.HasChangedFields(target1)) &&
                    // TF_AXIOM(n.GetChangedFields(target1) == 
                    //          TfTokenVector{SdfFieldKeys->References}) &&
                    // TF_AXIOM(n.HasChangedFields(foo)) &&
                    // TF_AXIOM(n.GetChangedFields(foo) == 
                    //          TfTokenVector{SdfFieldKeys->References}) &&

                    TF_AXIOM(!n.HasChangedFields(bar));
                    TF_AXIOM(n.GetChangedFields(bar).empty());
            });
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

        tester.AddTest([foo, bar, cls](Notice const &n) {
                return
                    TF_AXIOM(!n.ResyncedObject(foo)) &&
                    TF_AXIOM(!n.ResyncedObject(bar)) &&
                    TF_AXIOM(!n.ResyncedObject(cls)) &&
                    TF_AXIOM(n.AffectedObject(foo)) &&
                    TF_AXIOM(n.AffectedObject(bar)) &&
                    TF_AXIOM(n.AffectedObject(cls)) &&
                    TF_AXIOM(n.ChangedInfoOnly(foo)) &&
                    TF_AXIOM(n.ChangedInfoOnly(bar)) &&
                    TF_AXIOM(n.ChangedInfoOnly(cls));
            });

        tester.AddTest([foo, bar, cls](Notice const &n) {
                return
                    TF_AXIOM(n.HasChangedFields(foo)) &&
                    TF_AXIOM(n.GetChangedFields(foo) == 
                             TfTokenVector{SdfFieldKeys->Documentation}) &&
                    TF_AXIOM(n.HasChangedFields(bar)) &&
                    TF_AXIOM(n.GetChangedFields(bar) == 
                             TfTokenVector{SdfFieldKeys->Documentation}) &&
                    TF_AXIOM(n.HasChangedFields(cls)) &&
                    TF_AXIOM(n.GetChangedFields(cls) == 
                             TfTokenVector{SdfFieldKeys->Documentation});
            });

        cls.SetMetadata(SdfFieldKeys->Documentation, "cls doc");
    }

    // Assert that changing specializes causes changes to instances.
    UsdPrim specialize = stage->DefinePrim(SdfPath("/spec"));
    foo.GetSpecializes().AddSpecialize(specialize.GetPath());
    bar.GetSpecializes().AddSpecialize(specialize.GetPath());
    {
        printf("changing info in spec should cause info change in foo & bar\n");
        _NoticeTester tester(stage);
        tester.AddTest([foo, bar, specialize](Notice const &n) {
                return
                    TF_AXIOM(!n.ResyncedObject(foo)) &&
                    TF_AXIOM(!n.ResyncedObject(bar)) &&
                    TF_AXIOM(!n.ResyncedObject(specialize)) &&
                    TF_AXIOM(n.AffectedObject(foo)) &&
                    TF_AXIOM(n.AffectedObject(bar)) &&
                    TF_AXIOM(n.AffectedObject(specialize)) &&
                    TF_AXIOM(n.ChangedInfoOnly(foo)) &&
                    TF_AXIOM(n.ChangedInfoOnly(bar)) &&
                    TF_AXIOM(n.ChangedInfoOnly(specialize));
            });
        tester.AddTest([foo, bar, specialize](Notice const &n) {
                return
                    TF_AXIOM(n.HasChangedFields(foo)) &&
                    TF_AXIOM(n.GetChangedFields(foo) == 
                             TfTokenVector{SdfFieldKeys->Documentation}) &&
                    TF_AXIOM(n.HasChangedFields(bar)) &&
                    TF_AXIOM(n.GetChangedFields(bar) == 
                             TfTokenVector{SdfFieldKeys->Documentation}) &&
                    TF_AXIOM(n.HasChangedFields(specialize)) &&
                    TF_AXIOM(n.GetChangedFields(specialize) == 
                             TfTokenVector{SdfFieldKeys->Documentation});
            });
        specialize.SetMetadata(SdfFieldKeys->Documentation, "spec doc");
    }

    // Assert that changes to non-composition related metadata fields come
    // through as info changes rather than resyncs.
    {
        printf("Setting prim doc should cause info change, but no resync\n");
        _NoticeTester tester(stage);
        tester.AddTest([foo](Notice const &n) {
                return
                    TF_AXIOM(!n.ResyncedObject(foo)) &&
                    TF_AXIOM(n.ChangedInfoOnly(foo)) &&
                    TF_AXIOM(n.AffectedObject(foo));
            });
        tester.AddTest([foo](Notice const &n) {
                return
                    TF_AXIOM(n.HasChangedFields(foo)) &&
                    TF_AXIOM(n.GetChangedFields(foo) == 
                             TfTokenVector{SdfFieldKeys->Documentation});
            });
        foo.SetMetadata(SdfFieldKeys->Documentation, "hello doc");
    }

    // Assert that resyncs subsume info changes.
    {
        printf("Setting prim doc and typename in one go should cause a "
               "resync\n");
        _NoticeTester tester(stage);
        tester.AddTest([foo](Notice const &n) {
                return
                    TF_AXIOM(n.ResyncedObject(foo)) &&
                    TF_AXIOM(!n.ChangedInfoOnly(foo)) &&
                    TF_AXIOM(n.AffectedObject(foo));
            });
        tester.AddTest([foo](Notice const &n) {
                return
                    TF_AXIOM(n.HasChangedFields(foo)) &&
                    TF_AXIOM(n.GetChangedFields(foo) == 
                             (TfTokenVector{SdfFieldKeys->Documentation, 
                                            SdfFieldKeys->TypeName}));
            });
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
        tester.AddTest([](Notice const &n) {
                return
                    TF_AXIOM(SdfPathVector(n.GetResyncedPaths()) ==
                             SdfPathVector{SdfPath("/foo.attr")}) &&
                    TF_AXIOM(n.GetChangedInfoOnlyPaths().empty());
            });
        tester.AddTest([](Notice const &n) {
                return
                    TF_AXIOM(n.HasChangedFields(SdfPath("/foo.attr"))) &&
                    TF_AXIOM(n.GetChangedFields(SdfPath("/foo.attr")) == 
                             TfTokenVector{SdfFieldKeys->Custom});
            });
        attr = foo.CreateAttribute(TfToken("attr"), SdfValueTypeNames->Int);
    }

    // Assert that changing an attribute value causes info changes.
    {
        printf("Setting an attribute value should cause info change\n");
        _NoticeTester tester(stage);
        tester.AddTest([attr](Notice const &n) {
                return
                    TF_AXIOM(!n.ResyncedObject(attr)) &&
                    TF_AXIOM(n.ChangedInfoOnly(attr)) &&
                    TF_AXIOM(n.AffectedObject(attr)) &&
                    TF_AXIOM(n.GetResyncedPaths().empty()) &&
                    TF_AXIOM(SdfPathVector(n.GetChangedInfoOnlyPaths()) == 
                             SdfPathVector{SdfPath("/foo.attr")});
            });
        tester.AddTest([attr](Notice const &n) {
                return
                    TF_AXIOM(n.HasChangedFields(attr)) &&
                    TF_AXIOM(n.GetChangedFields(attr) == 
                             TfTokenVector{SdfFieldKeys->Default});
            });
        attr.Set(42);
    }

    // Assert that creating a relationship causes resyncs
    UsdRelationship rel;
    {
        printf("Creating a relationship should cause a resync\n");
        _NoticeTester tester(stage);
        tester.AddTest([](Notice const &n) {
                return
                    TF_AXIOM(SdfPathVector(n.GetResyncedPaths()) ==
                             SdfPathVector{SdfPath("/foo.rel")}) &&
                    TF_AXIOM(n.GetChangedInfoOnlyPaths().empty());
            });
        tester.AddTest([](Notice const &n) {
                return
                    TF_AXIOM(n.HasChangedFields(SdfPath("/foo.rel"))) &&
                    TF_AXIOM(n.GetChangedFields(SdfPath("/foo.rel")) == 
                             (TfTokenVector{SdfFieldKeys->Custom,
                                            SdfFieldKeys->Variability}));
            });
        rel = foo.CreateRelationship(TfToken("rel"));
    }

    // Assert that setting a relationship target causes info changes.
    {
        printf("Changing relationship targets should cause info change\n");
        _NoticeTester tester(stage);
        tester.AddTest([rel](Notice const &n) {
                return
                    TF_AXIOM(!n.ResyncedObject(rel)) &&
                    TF_AXIOM(n.ChangedInfoOnly(rel)) &&
                    TF_AXIOM(n.AffectedObject(rel)) &&
                    TF_AXIOM(n.GetResyncedPaths().empty()) &&
                    TF_AXIOM(SdfPathVector(n.GetChangedInfoOnlyPaths()) == 
                             SdfPathVector{SdfPath("/foo.rel")});
            });
        tester.AddTest([rel](Notice const &n) {
                return
                    TF_AXIOM(!n.HasChangedFields(rel)) &&
                    TF_AXIOM(n.GetChangedFields(rel).empty());
                    // XXX: Sdf currently does not report affected fields for
                    // certain types of changes. Once that's fixed, these test
                    // cases can replace the ones above.
                    //
                    // TF_AXIOM(n.HasChangedFields(rel)) &&
                    // TF_AXIOM(n.GetChangedFields(rel) == 
                    //          TfTokenVector{SdfFieldKeys->TargetPaths});
                    
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
