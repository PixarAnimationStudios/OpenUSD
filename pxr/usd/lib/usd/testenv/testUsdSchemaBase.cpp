#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaBase.h"

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

int main(int argc, char** argv)
{
    TestEnsureParentCtorForCopying();
    
    printf("Passed!\n");
    
    TF_AXIOM(not Py_IsInitialized());

    return EXIT_SUCCESS;
}
