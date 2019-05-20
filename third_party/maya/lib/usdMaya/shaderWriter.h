//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_SHADER_WRITER_H
#define PXRUSDMAYA_SHADER_WRITER_H

/// \file usdMaya/shaderWriter.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "usdMaya/primWriter.h"

#include "usdMaya/writeJobContext.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/property.h"

#include <maya/MFnDependencyNode.h>

#include <memory>


PXR_NAMESPACE_OPEN_SCOPE


/// Base class for USD prim writers that export Maya shading nodes as USD
/// shader prims.
class UsdMayaShaderWriter : public UsdMayaPrimWriter
{
    public:
        PXRUSDMAYA_API
        UsdMayaShaderWriter(
                const MFnDependencyNode& depNodeFn,
                const SdfPath& usdPath,
                UsdMayaWriteJobContext& jobCtx);

        /// Get the name of the USD shading property that corresponds to the
        /// Maya attribute named \p mayaAttrName.
        ///
        /// The property name should be the fully namespaced name in USD (e.g.
        /// "inputs:myInputProperty" or "outputs:myOutputProperty" for shader
        /// input and output properties, respectively).
        ///
        /// The default implementation always returns an empty token, which
        /// effectively prevents any connections from being authored to or from
        /// the exported prims in USD. Derived classes should override this and
        /// return the corresponding property names for the Maya attributes
        /// that should be considered for connections.
        PXRUSDMAYA_API
        virtual TfToken GetShadingPropertyNameForMayaAttrName(
                const TfToken& mayaAttrName);

        /// Get the USD shading property that corresponds to the Maya attribute
        /// named \p mayaAttrName.
        ///
        /// The default implementation calls
        /// GetShadingPropertyNameForMayaAttrName() with the given
        /// \p mayaAttrName and then attempts to get the USD property with that
        /// name from the shader writer's USD prim. Note that this means this
        /// method will only return valid USD properties that the shader writer
        /// has already authored on its privately held UsdPrim, so this method
        /// should only be called after Write() has been called at least once.
        PXRUSDMAYA_API
        virtual UsdProperty GetShadingPropertyForMayaAttrName(
                const TfToken& mayaAttrName);
};

typedef std::shared_ptr<UsdMayaShaderWriter> UsdMayaShaderWriterSharedPtr;


PXR_NAMESPACE_CLOSE_SCOPE


#endif
