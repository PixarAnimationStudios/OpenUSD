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
    TF_WRAP(Asset);
    TF_WRAP(AssetInfo);
    TF_WRAP(ResolvedPath);
    TF_WRAP(Timestamp);

    TF_WRAP(Resolver);
    TF_WRAP(ResolverContext);
    TF_WRAP(ResolverContextBinder);
    TF_WRAP(ResolverScopedCache);

    TF_WRAP(DefaultResolver);
    TF_WRAP(DefaultResolverContext);

    TF_WRAP(PackageUtils);
    TF_WRAP(Notice);
}
