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
#ifndef PXRUSDMAYA_TRANSLATOR_UTIL_H
#define PXRUSDMAYA_TRANSLATOR_UTIL_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/usd/usd/prim.h"

#include <maya/MObject.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE



/// \brief Provides helper functions for other readers to use.
struct UsdMayaTranslatorUtil
{
    /// \brief Often when creating a prim, we want to first create a Transform
    /// node. This is a small helper to do this. If the \p args provided
    /// indicate that animation should be read, any transform animation from
    /// the prim is transferred onto the Maya transform node. If \p context is
    /// non-NULL, the new Maya node will be registered to the path of
    /// \p usdPrim.
    PXRUSDMAYA_API
    static bool
    CreateTransformNode(
            const UsdPrim& usdPrim,
            MObject& parentNode,
            const UsdMayaPrimReaderArgs& args,
            UsdMayaPrimReaderContext* context,
            MStatus* status,
            MObject* mayaNodeObj);

    /// \brief Creates a "dummy" transform node for the given prim, where the
    /// dummy transform has all transform properties locked.
    /// A UsdMayaAdaptor-compatible attribute for the typeName metadata will
    /// be generated. If \p importTypeName is \c true, this attribute will
    /// contain the \c typeName metadata of \p usdPrim, so the \c typeName will
    /// be applied on export. Otherwise, this attribute will be set to the
    /// empty string, so a typeless def will be generated on export.
    PXRUSDMAYA_API
    static bool
    CreateDummyTransformNode(
            const UsdPrim& usdPrim,
            MObject& parentNode,
            bool importTypeName,
            const UsdMayaPrimReaderArgs& args,
            UsdMayaPrimReaderContext* context,
            MStatus* status,
            MObject* mayaNodeObj);

    /// \brief Helper to create a node for \p usdPrim of type \p
    /// nodeTypeName under \p parentNode. If \p context is non-NULL,
    /// the new Maya node will be registered to the path of \p usdPrim.
    PXRUSDMAYA_API
    static bool
    CreateNode(
            const UsdPrim& usdPrim,
            const MString& nodeTypeName,
            MObject& parentNode,
            UsdMayaPrimReaderContext* context,
            MStatus* status,
            MObject* mayaNodeObj);

    /// \brief Helper to create a node for \p usdPath of type \p
    /// nodeTypeName under \p parentNode. If \p context is non-NULL,
    /// the new Maya node will be registered to the path of \p usdPrim.
    PXRUSDMAYA_API
    static bool
    CreateNode(
            const SdfPath& usdPath,
            const MString& nodeTypeName,
            MObject& parentNode,
            UsdMayaPrimReaderContext* context,
            MStatus* status,
            MObject* mayaNodeObj);

    /// \brief Helper to create a node named \p nodeName of type \p
    /// nodeTypeName under \p parentNode. Note that this version does
    /// NOT take a context and cannot register the newly created Maya node
    /// since it does not know the SdfPath to an originating object.
    PXRUSDMAYA_API
    static bool
    CreateNode(
            const MString& nodeName,
            const MString& nodeTypeName,
            MObject& parentNode,
            MStatus* status,
            MObject* mayaNodeObj);

    /// \brief Helper to create a shadingNode.  When \p asShader is \c true,
    /// this is intended to mimic the mel command "shadingNode ... -asShader".
    ///
    /// In particular, this hooks up the shader to "defaultShadingList1.shaders"
    /// which makes sure the node shows up in the Hypershade UI.
    ///
    /// If there are other side-effects of using "shadingNode" (as opposed to
    /// "createNode" directly), this should be udpated accordingly.
    PXRUSDMAYA_API
    static bool
    CreateShaderNode(
            const MString& nodeName,
            const MString& nodeTypeName,
            const bool asShader,
            MStatus* status,
            MObject* shaderObj);

    /// \brief Helper to set up a light node as a default light.  
    /// This is intended to mimic the mel command "shadingNode ... -asLight".
    ///
    /// In particular, this makes sure the light nodes are members of the
    /// "defaultLightSet" which allows lights to be recognized on the stage
    ///
    /// If there are other side-effects of using "shadingNode" (as opposed to
    /// "createNode" directly), this should be updated accordingly.
    PXRUSDMAYA_API
    static bool
    ConnectDefaultLightNode(
            MObject& lightNode,
            MStatus* status);

    /// Gets an API schema of the requested type for the given \p usdPrim.
    ///
    /// This ensures that the USD prim has the API schema applied to it if it
    /// does not already.
    template <typename APISchemaType>
    static APISchemaType GetAPISchemaForAuthoring(const UsdPrim& usdPrim)
    {
        if (!usdPrim.HasAPI<APISchemaType>()) {
            return APISchemaType::Apply(usdPrim);
        }

        return APISchemaType(usdPrim);
    }

};



PXR_NAMESPACE_CLOSE_SCOPE

#endif
