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
    TF_WRAP( Dependency );
    TF_WRAP( DynamicFileFormatDependencyData );
    TF_WRAP( Cache );
    TF_WRAP( Errors );
    TF_WRAP( InstanceKey );
    TF_WRAP( LayerRelocatesEditBuilder );

    TF_WRAP( ExpressionVariablesSource ); // Required by LayerStackIdentifier
    TF_WRAP( LayerStackIdentifier );

    TF_WRAP( LayerStack );
    TF_WRAP( MapExpression );
    TF_WRAP( MapFunction );
    TF_WRAP( Node );
    TF_WRAP( PathTranslation );
    TF_WRAP( PrimIndex );
    TF_WRAP( PropertyIndex );
    TF_WRAP( Site );
    TF_WRAP( ExpressionVariables );
    TF_WRAP( TestChangeProcessor );
    TF_WRAP( Types );
}
