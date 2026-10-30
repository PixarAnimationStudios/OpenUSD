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
    TF_WRAP_NB( Dependency );
    TF_WRAP_NB( DynamicFileFormatDependencyData );
    TF_WRAP_NB( Cache );
    TF_WRAP_NB( Errors );
    TF_WRAP_NB( InstanceKey );
    TF_WRAP_NB( LayerRelocatesEditBuilder );

    TF_WRAP_NB( ExpressionVariablesSource ); // Required by LayerStackIdentifier
    TF_WRAP_NB( LayerStackIdentifier );

    TF_WRAP_NB( LayerStack );
    TF_WRAP_NB( MapExpression );
    TF_WRAP_NB( MapFunction );
    TF_WRAP_NB( Node );
    TF_WRAP_NB( PathTranslation );
    TF_WRAP_NB( PrimIndex );
    TF_WRAP_NB( PropertyIndex );
    TF_WRAP_NB( Site );
    TF_WRAP_NB( ExpressionVariables );
    TF_WRAP_NB( TestChangeProcessor );
    TF_WRAP_NB( Types );
}
