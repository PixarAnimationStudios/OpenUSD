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
    TF_WRAP( ArrayAssetPath );
    TF_WRAP( ArrayPath );
    TF_WRAP( ArrayTimeCode );
    TF_WRAP( AssetPath );
    TF_WRAP( ChangeBlock );
    TF_WRAP( CleanupEnabler );
    TF_WRAP( CopyUtils );
    TF_WRAP( FileFormat );
    TF_WRAP( Layer );
    TF_WRAP( LayerOffset );
    TF_WRAP( LayerTree );
    TF_WRAP( NamespaceEdit );
    TF_WRAP( Notice );
    TF_WRAP( OpaqueValue );
    TF_WRAP( Path );
    TF_WRAP( PredicateExpression );
    TF_WRAP( PathPattern );
    TF_WRAP( PathExpression ); // needs PathPattern & PredicateExpression.
    TF_WRAP( Payload );
    TF_WRAP( PredicateFunctionResult );
    TF_WRAP( Reference );
    TF_WRAP( TimeCode );
    TF_WRAP( Types );
    TF_WRAP( ValueType );
    TF_WRAP( VariableExpression );

    TF_WRAP( Spec );
    TF_WRAP( VariantSpec );
    TF_WRAP( VariantSetSpec );

    TF_WRAP( PropertySpec );
    TF_WRAP( AttributeSpec );
    TF_WRAP( RelationshipSpec );

    TF_WRAP( PrimSpec );
    TF_WRAP( PseudoRootSpec );
}
