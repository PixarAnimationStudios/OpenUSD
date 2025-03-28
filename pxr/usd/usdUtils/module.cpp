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
    TF_WRAP( Authoring );
    TF_WRAP( CoalescingDiagnosticDelegate );
    TF_WRAP( ConditionalAbortDiagnosticDelegate );
    TF_WRAP( Dependencies );
    TF_WRAP( FlattenLayerStack );
    TF_WRAP( Introspection );
    TF_WRAP( Pipeline );
    TF_WRAP( RegisteredVariantSet );
    TF_WRAP( SparseValueWriter );
    TF_WRAP( StageCache );
    TF_WRAP( Stitch );
    TF_WRAP( StitchClips );
    TF_WRAP( TimeCodeRange );
    TF_WRAP( UserProcessingFunc );
    TF_WRAP( LocalizeAsset );
}
