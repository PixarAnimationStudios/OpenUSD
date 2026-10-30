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
    TF_WRAP_NB( Authoring );
    TF_WRAP_NB( CoalescingDiagnosticDelegate );
    TF_WRAP_NB( ConditionalAbortDiagnosticDelegate );
    TF_WRAP_NB( Dependencies );
    TF_WRAP_NB( FlattenLayerStack );
    TF_WRAP_NB( Introspection );
    TF_WRAP_NB( Pipeline );
    TF_WRAP_NB( RegisteredVariantSet );
    TF_WRAP_NB( SparseValueWriter );
    TF_WRAP_NB( StageCache );
    TF_WRAP_NB( Stitch );
    TF_WRAP_NB( StitchClips );
    TF_WRAP_NB( TimeCodeRange );
    TF_WRAP_NB( UserProcessingFunc );
    TF_WRAP_NB( LocalizeAsset );
}
