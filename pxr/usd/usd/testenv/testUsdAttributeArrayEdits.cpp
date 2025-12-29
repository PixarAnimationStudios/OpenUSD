//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/arrayEditBuilder.h"

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestBasics()
{
    auto stage = UsdStage::CreateInMemory();

    const UsdPrim prim = stage->DefinePrim(SdfPath("/TestBasics"));

    const UsdAttribute attr =
        prim.CreateAttribute(TfToken("attr"), SdfValueTypeNames->IntArray);

    VtIntArray iarray { 3, 2, 1 };
    TF_AXIOM(!attr.Get(&iarray));

    TF_AXIOM(attr.Set(iarray));

    iarray.clear();
    TF_AXIOM(attr.Get(&iarray));
    TF_AXIOM((iarray == VtIntArray { 3, 2, 1 }));

    VtArrayEdit zeroNine = VtIntArrayEditBuilder()
        .Prepend(0)
        .Append(9)
        .FinalizeAndReset();

    // Author the zeroNine edit to the session layer.
    {
        UsdEditContext toSessionLayer(stage, stage->GetSessionLayer());
        attr.Set(zeroNine);
    }

    // Now the value should be the edited value.
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray));
    TF_AXIOM((iarray == VtIntArray { 0, 3, 2, 1, 9 }));
}


int main()
{
    TestBasics();

    printf("SUCCEEDED\n");
    return 0;
}
