//
// Unlicensed 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP( ExecProperty );
    TF_WRAP( ExecNode );
    TF_WRAP( Registry );
}
