//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    TF_AXIOM(!prim.HasAPI<UsdCollectionAPI>());

    printf("--------Applying UsdCollectionAPI -------\n");

    UsdCollectionAPI coll = UsdCollectionAPI::Apply(prim, TfToken("testColl"));
    TF_AXIOM(prim.HasAPI<UsdCollectionAPI>());
    TF_AXIOM(prim.HasAPIInFamily<UsdCollectionAPI>(
        UsdSchemaRegistry::VersionPolicy::All));

    TF_AXIOM(prim.HasAPI<UsdCollectionAPI>(/*instanceName*/ TfToken("testColl")));
    TF_AXIOM(prim.HasAPIInFamily<UsdCollectionAPI>(
        UsdSchemaRegistry::VersionPolicy::All, TfToken("testColl")));

    TF_AXIOM(!prim.HasAPI<UsdCollectionAPI>(
            /*instanceName*/ TfToken("nonExistentColl")));
    TF_AXIOM(!prim.HasAPIInFamily<UsdCollectionAPI>(
        UsdSchemaRegistry::VersionPolicy::All, TfToken("nonExistentColl")));

    printf("--------Removing UsdCollectionAPI -------\n");

    prim.RemoveAPI<UsdCollectionAPI>(/*instanceName*/ TfToken("testColl"));

    TF_AXIOM(!prim.HasAPI<UsdCollectionAPI>());

    TF_AXIOM(!prim.HasAPI<UsdCollectionAPI>(/*instanceName*/ TfToken("testColl")));

    printf("--------Applying UsdCollectionAPI through UsdPrim API -------\n");

    prim.ApplyAPI<UsdCollectionAPI>(/*instanceName*/ TfToken("testColl"));

    TF_AXIOM(prim.HasAPI<UsdCollectionAPI>());

    TF_AXIOM(prim.HasAPI<UsdCollectionAPI>(/*instanceName*/ TfToken("testColl")));

    printf("--------Finding UsdCollectionAPI SchemaInfo -------\n");

    const UsdSchemaRegistry::SchemaInfo *schemaInfo = 
        UsdSchemaRegistry::FindSchemaInfo<UsdCollectionAPI>();
    TF_AXIOM(schemaInfo);
    TF_AXIOM(schemaInfo->type == TfType::Find<UsdCollectionAPI>());
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
