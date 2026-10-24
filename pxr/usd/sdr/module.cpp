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
    TF_WRAP_NB( Declare );
    TF_WRAP_NB( DiscoveryPlugin );
    TF_WRAP_NB( FilesystemDiscovery );
    TF_WRAP_NB( FilesystemDiscoveryHelpers );
    TF_WRAP_NB( SdfTypeIndicator );
    TF_WRAP_NB( ShaderMetadataHelpers );
    TF_WRAP_NB( ShaderNode );
    TF_WRAP_NB( ShaderNodeDiscoveryResult );
    TF_WRAP_NB( ShaderNodeMetadata );
    TF_WRAP_NB( ShaderNodeQuery );
    TF_WRAP_NB( ShaderNodeQueryUtils );
    TF_WRAP_NB( ShaderProperty );
    TF_WRAP_NB( ShaderPropertyMetadata );
    TF_WRAP_NB( Registry );
}
