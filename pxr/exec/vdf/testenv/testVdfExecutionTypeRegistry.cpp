//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/indexedData.h"

#include "pxr/base/tf/errorMark.h"

#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

// assumption: copy-ctor ok (also assumed elsewhere)
struct A
{
    A() = default;
    A(int a, int b) : _m(a+b)  {}
    A(const A &a)   : _m(a._m) {}

    // Make sure equality comparison detection compiles when the operator has
    // the nodiscard attribute.
    [[nodiscard]] friend bool operator==(const A &lhs, const A &rhs) {
        return lhs._m == rhs._m;
    }

    int _m;
};

struct B
{
    B() = default;
    B(float m)    : _m(m)    {}
    B(const B &b) : _m(b._m) {}

    // Make sure equality comparison detection works on types that aren't
    // equality comparable.
    bool operator==(const B &) = delete;

    float _m;
};

TF_REGISTRY_FUNCTION(VdfExecutionTypeRegistry)
{
    TfErrorMark mark;

    // Register a fallback for type A.
    VdfExecutionTypeRegistry::Define(A(4700, 11));
    TF_AXIOM(mark.IsClean());

    // Register a fallback for type B.
    VdfExecutionTypeRegistry::Define(B(0.815f));
    TF_AXIOM(mark.IsClean());

    // Register a different fallback value for type A. This fails a verify,
    // since A is equality comparable, so we are able to detect the difference
    // in the registered value.
    VdfExecutionTypeRegistry::Define(A(4700, 12));
    TF_AXIOM(!mark.IsClean());
    mark.Clear();

    VdfExecutionTypeRegistry::Define(28);
    TF_AXIOM(mark.IsClean());

    // Register a different fallback value for type B. B is not equality
    // comparable, so we are not able to detect the mismatch in values in
    // this case.
    VdfExecutionTypeRegistry::Define(B());
    TF_AXIOM(mark.IsClean());

    // Register container types for A, which is equality comparable.
    VdfExecutionTypeRegistry::Define(VdfIndexedData<A>());
    TF_AXIOM(mark.IsClean());
    VdfExecutionTypeRegistry::Define(std::pair<A, A>());
    TF_AXIOM(mark.IsClean());
    VdfExecutionTypeRegistry::Define(std::vector<A>());
    TF_AXIOM(mark.IsClean());
    VdfExecutionTypeRegistry::Define(std::map<int, A>());
    TF_AXIOM(mark.IsClean());
    VdfExecutionTypeRegistry::Define(std::unordered_map<int, A>());
    TF_AXIOM(mark.IsClean());

    // Register container types for B, which is not equality comparable.
    VdfExecutionTypeRegistry::Define(VdfIndexedData<B>());
    TF_AXIOM(mark.IsClean());
    VdfExecutionTypeRegistry::Define(std::pair<B, B>());
    TF_AXIOM(mark.IsClean());
    VdfExecutionTypeRegistry::Define(std::vector<B>());
    TF_AXIOM(mark.IsClean());
    VdfExecutionTypeRegistry::Define(std::map<int, B>());
    TF_AXIOM(mark.IsClean());
    VdfExecutionTypeRegistry::Define(std::unordered_map<int, B>());
    TF_AXIOM(mark.IsClean());
}

int 
main(int argc, char **argv) 
{
    printf("> Hello, testVdfExecutionTypeRegistry!\n");

    VdfExecutionTypeRegistry &r = VdfExecutionTypeRegistry::GetInstance();

    int i = r.GetFallback<int>();
    printf("> int fb= %d\n", i);
    TF_AXIOM(i == 28);

    A a = r.GetFallback<A>();
    printf("> A fb= %d\n", a._m);
    TF_AXIOM(a._m == 4711);

    B b = r.GetFallback<B>();
    printf("> B fb= %f\n", b._m);
    TF_AXIOM(b._m == 0.815f);

    return 0;
}


