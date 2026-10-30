//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP_NB(Asset);
    TF_WRAP_NB(AssetInfo);
    TF_WRAP_NB(ResolvedPath);
    TF_WRAP_NB(Timestamp);

    TF_WRAP_NB(Resolver);
    TF_WRAP_NB(ResolverContext);
    TF_WRAP_NB(ResolverContextBinder);
    TF_WRAP_NB(ResolverScopedCache);

    TF_WRAP_NB(DefaultResolver);
    TF_WRAP_NB(DefaultResolverContext);

    TF_WRAP_NB(PackageUtils);
    TF_WRAP_NB(Notice);
}
