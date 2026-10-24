//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP_NB( Collector );
    TF_WRAP_NB( AggregateNode );
    TF_WRAP_NB( AggregateTree );
    TF_WRAP_NB( Reporter );

    TF_WRAP_NB( TestTrace );
}
