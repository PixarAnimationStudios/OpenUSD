//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(PCP_CHANGES, "Pcp change processing");
    TF_DEBUG_ENVIRONMENT_SYMBOL(PCP_DEPENDENCIES, "Pcp dependencies");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PCP_PRIM_INDEX, 
        "Print debug output to terminal during prim indexing");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PCP_PRIM_INDEX_GRAPHS, 
        "Write graphviz 'dot' files during prim indexing "
        "(requires PCP_PRIM_INDEX)");
    
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PCP_PRIM_INDEX_GRAPHS_MAPPINGS,
        "Include namespace mappings in graphviz files generated "
        "during prim indexing (requires PCP_PRIM_INDEX_GRAPHS)");

    TF_DEBUG_ENVIRONMENT_SYMBOL(PCP_NAMESPACE_EDIT, "Pcp namespace edits");
}

PXR_NAMESPACE_CLOSE_SCOPE
