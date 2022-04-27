//
// Unlicensed 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(UsdExecTypes);
    TF_WRAP(UsdExecUtils);
    TF_WRAP(UsdExecConnectableAPI);
    TF_WRAP(UsdExecInput);
    TF_WRAP(UsdExecOutput);
    TF_WRAP(UsdExecNode);
    TF_WRAP(UsdExecGraph);
    TF_WRAP(UsdExecTokens);
}
