//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP( ArrayAssetPath );
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
    TF_WRAP( Path );
    TF_WRAP( Payload );
    TF_WRAP( Reference );
    TF_WRAP( Types );
    TF_WRAP( ValueType );

    TF_WRAP( Spec );
    TF_WRAP( MapperSpec );
    TF_WRAP( MapperArgSpec );
    TF_WRAP( VariantSpec );
    TF_WRAP( VariantSetSpec );

    TF_WRAP( PropertySpec );
    TF_WRAP( AttributeSpec );
    TF_WRAP( RelationshipSpec );

    TF_WRAP( PrimSpec );
    TF_WRAP( PseudoRootSpec );
}
