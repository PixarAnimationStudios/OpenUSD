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
    TF_WRAP_NB( Types );
    TF_WRAP_NB( ArrayAssetPath );
    TF_WRAP_NB( ArrayPath );
    TF_WRAP_NB( ArrayPathExpression );
    TF_WRAP_NB( AssetPath );
    TF_WRAP_NB( ChangeBlock );
    TF_WRAP_NB( CleanupEnabler );
    TF_WRAP_NB( CopyUtils );
    TF_WRAP_NB( CrateInfo );
    TF_WRAP_NB( FileFormat );
    TF_WRAP_NB( Layer );
    TF_WRAP_NB( LayerOffset );
    TF_WRAP_NB( LayerTree );
    TF_WRAP_NB( NamespaceEdit );
    TF_WRAP_NB( Notice );
    TF_WRAP_NB( OpaqueValue );
    TF_WRAP_NB( Path );
    TF_WRAP_NB( PredicateExpression );
    TF_WRAP_NB( PathPattern );
    TF_WRAP_NB( PathExpression ); // needs PathPattern & PredicateExpression.
    TF_WRAP_NB( Payload );
    TF_WRAP_NB( PredicateFunctionResult );
    TF_WRAP_NB( Reference );
    TF_WRAP_NB( UsdFileFormat );
    TF_WRAP_NB( ValueType );
    TF_WRAP_NB( VariableExpression );
    TF_WRAP_NB( VariableExpressionAST );
    TF_WRAP_NB( ZipFile );

    TF_WRAP_NB( Spec );
    TF_WRAP_NB( VariantSpec );
    TF_WRAP_NB( VariantSetSpec );

    TF_WRAP_NB( PropertySpec );
    TF_WRAP_NB( AttributeSpec );
    TF_WRAP_NB( RelationshipSpec );

    TF_WRAP_NB( PrimSpec );
    TF_WRAP_NB( PseudoRootSpec );
}
