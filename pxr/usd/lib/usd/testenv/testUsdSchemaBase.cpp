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
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/schemaBase.h"

PXR_NAMESPACE_USING_DIRECTIVE

static const int SCHEMA_BASE_INIT = 1971;
static const int TEST_BASE_INIT = 44;
static const int TEST_DERIVED_INIT = 42;
static const int MUTATED_VAL = 22;

class Usd_TestBase : public UsdSchemaBase
{
public:

    /// Construct a Usd_TestBase on UsdPrim \p prim .
    /// Equivalent to Usd_TestBase::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit Usd_TestBase(const UsdPrim& prim=UsdPrim())
        : UsdSchemaBase(prim)
        , foo(TEST_BASE_INIT)
    {
        printf("called Usd_TestBase(const UsdPrim& prim=UsdPrim())\n");
    }

    /// Construct a Usd_TestBase on the prim wrapped by \p schemaObj .
    /// Should be preferred over Usd_TestBase(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit Usd_TestBase(const UsdSchemaBase& schemaObj)
        : UsdSchemaBase(schemaObj)
        , foo(SCHEMA_BASE_INIT)
    {
        printf("called Usd_TestBase(const UsdSchemaBase& schemaObj)\n");
    }

    virtual ~Usd_TestBase() {};

    // XXX This is what we need to test slicing
    int  foo;
};

class Usd_TestDerived : public Usd_TestBase
{
public:

    /// Construct a Usd_TestDerived on UsdPrim \p prim .
    /// Equivalent to Usd_TestDerived::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit Usd_TestDerived(const UsdPrim& prim=UsdPrim())
        : Usd_TestBase(prim)
    {
        foo = TEST_DERIVED_INIT;
        printf("called Usd_Derived(const UsdPrim& prim=UsdPrim())\n");
    }

    /// Construct a Usd_TestDerived on the prim wrapped by \p schemaObj .
    /// Should be preferred over Usd_TestDerived(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit Usd_TestDerived(const UsdSchemaBase& schemaObj)
        : Usd_TestBase(schemaObj)
    {
        foo = SCHEMA_BASE_INIT;
        printf("called Usd_TestDerived(const UsdSchemaBase& schemaObj)\n");
    }

    virtual ~Usd_TestDerived() {};

    int  bar;
};


static void
TestEnsureParentCtorForCopying()
{
    printf("TestEnsureParentCtorForCopying...\n");

    Usd_TestDerived  derived;
    Usd_TestBase     base;

    derived.foo = MUTATED_VAL;

    printf("--------Now assigning derived to base -------\n");

    base = derived;

    // This will fail if compiler picks the explicit UsdSchemaBase copy ctor
    // over the implicit ctor provided for Usd_TestBase
    TF_VERIFY( base.foo == MUTATED_VAL );
}

static void
TestPrimQueries()
{
    printf("TestPrimQueries...\n");

    auto stage = UsdStage::CreateInMemory("TestPrimQueries.usd");
    auto path = SdfPath("/p");
    auto prim = stage->DefinePrim(path);
    
    printf("--------Ensuring no schemas are applied -------\n");
    assert(!prim.HasAPI<UsdClipsAPI>());
    assert(!prim.HasAPI<UsdModelAPI>());
    
    printf("--------Applying UsdModelAPI -------\n");
    UsdModelAPI::Apply(prim);
    assert(!prim.HasAPI<UsdClipsAPI>());
    assert(prim.HasAPI<UsdModelAPI>());

    printf("--------Applying UsdClipsAPI -------\n");
    UsdClipsAPI::Apply(prim);
    assert(prim.HasAPI<UsdClipsAPI>());
    assert(prim.HasAPI<UsdModelAPI>());

    UsdCollectionAPI coll = UsdCollectionAPI::ApplyCollection(prim, 
            TfToken("testColl"));
    assert(prim.HasAPI<UsdCollectionAPI>());

    assert(prim.HasAPI<UsdCollectionAPI>(
            /*instanceName*/ TfToken("testColl")));

    assert(!prim.HasAPI<UsdCollectionAPI>(
            /*instanceName*/ TfToken("nonExistentColl")));

    std::cerr << "--- BEGIN EXPECTED ERROR --" << std::endl;
    TfErrorMark mark;
    // Passing in a non-empty instance name with a single-apply API schema like
    // ModelAPI results in a coding error
    assert(!prim.HasAPI<UsdModelAPI>(/*instanceName*/ TfToken("instance")));
    TF_VERIFY(!mark.IsClean());
    std::cerr << "--- END EXPECTED ERROR --" << std::endl;
}

int main(int argc, char** argv)
{
    TestEnsureParentCtorForCopying();
    TestPrimQueries();
    
    printf("Passed!\n");
    
#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TF_AXIOM(!Py_IsInitialized());
#endif // PXR_PYTHON_SUPPORT_ENABLED

    return EXIT_SUCCESS;
}
