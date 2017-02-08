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
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/subdivTags.h"

#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSceneDelegate::HdSceneDelegate()
    : _index(new HdRenderIndex)
    , _delegateID(SdfPath::AbsoluteRootPath())
{
}

HdSceneDelegate::HdSceneDelegate(HdRenderIndexSharedPtr const& parentIndex, 
                    SdfPath const& delegateID)
    : _index(parentIndex)
    , _delegateID(delegateID)
{
}

HdSceneDelegate::~HdSceneDelegate()
{
}

/*virtual*/
void
HdSceneDelegate::Sync(HdSyncRequestVector* request)
{

}

void
HdSceneDelegate::PostSyncCleanup()
{
}

/*virtual*/
bool
HdSceneDelegate::IsEnabled(TfToken const& option) const
{
    if (option == HdOptionTokens->parallelRprimSync) {
        return true;
    }

    return false;
}

/*virtual*/
bool
HdSceneDelegate::IsInCollection(SdfPath const& id, 
                            TfToken const& collectionName)
{
    return true;
}

/*virtual*/
TfToken
HdSceneDelegate::GetRenderTag(SdfPath const& id)
{
    return HdTokens->geometry;
}

// -----------------------------------------------------------------------//
/// \name Rprim Aspects
// -----------------------------------------------------------------------//

/*virtual*/
HdMeshTopology
HdSceneDelegate::GetMeshTopology(SdfPath const& id)
{
    return HdMeshTopology();
}

/*virtual*/
HdBasisCurvesTopology
HdSceneDelegate::GetBasisCurvesTopology(SdfPath const& id)
{
    return HdBasisCurvesTopology();
}

/*virtual*/
PxOsdSubdivTags
HdSceneDelegate::GetSubdivTags(SdfPath const& id)
{
    return PxOsdSubdivTags();
}

/*virtual*/
GfRange3d
HdSceneDelegate::GetExtent(SdfPath const & id)
{
    return GfRange3d();
}

/*virtual*/
GfMatrix4d
HdSceneDelegate::GetTransform(SdfPath const & id)
{
    return GfMatrix4d();
}

/*virtual*/
bool
HdSceneDelegate::GetVisible(SdfPath const & id)
{
    return true;
}

/*virtual*/
bool
HdSceneDelegate::GetDoubleSided(SdfPath const & id)
{
    return false;
}

/*virtual*/
HdCullStyle
HdSceneDelegate::GetCullStyle(SdfPath const &id)
{
    return HdCullStyleDontCare;
}

/*virtual*/
int
HdSceneDelegate::GetRefineLevel(SdfPath const& id)
{
    return 0;
}

/*virtual*/
VtValue
HdSceneDelegate::Get(SdfPath const& id, TfToken const& key)
{
    return VtValue();
}

/*virtual*/
TfToken
HdSceneDelegate::GetReprName(SdfPath const &id)
{
    return TfToken();
}

// -----------------------------------------------------------------------//
/// \name Instancer prototypes
// -----------------------------------------------------------------------//

/*virtual*/
VtIntArray
HdSceneDelegate::GetInstanceIndices(SdfPath const &instancerId,
                                      SdfPath const &prototypeId)
{
    return VtIntArray();
}

/*virtual*/
GfMatrix4d
HdSceneDelegate::GetInstancerTransform(SdfPath const &instancerId,
                                         SdfPath const &prototypeId)
{
    return GfMatrix4d();
}

/*virtual*/
SdfPath
HdSceneDelegate::GetPathForInstanceIndex(const SdfPath &protoPrimPath,
                                         int instanceIndex,
                                         int *absoluteInstanceIndex,
                                         SdfPath * rprimPath,
                                         SdfPathVector *instanceContext)
{
    return SdfPath();
}



// -----------------------------------------------------------------------//
/// \name SurfaceShader Aspects
// -----------------------------------------------------------------------//

/*virtual*/
std::string
HdSceneDelegate::GetSurfaceShaderSource(SdfPath const &shaderId)
{
    return std::string("");
}

/*virtual*/
std::string
HdSceneDelegate::GetDisplacementShaderSource(SdfPath const &shaderId)
{
    std::string shaderSource(
        "vec4 displacementShader(int index, vec4 Peye, vec3 Neye, vec4 patchCoord) {\n"
        "    return Peye;\n"
        "}\n"
        );
    return shaderSource;
}

/*virtual*/
TfTokenVector
HdSceneDelegate::GetSurfaceShaderParamNames(SdfPath const &shaderId)
{
    return TfTokenVector();
}

/*virtual*/
VtValue
HdSceneDelegate::GetSurfaceShaderParamValue(SdfPath const &shaderId, 
                              TfToken const &paramName)
{
    return VtValue();
}

/*virtual*/
HdShaderParamVector
HdSceneDelegate::GetSurfaceShaderParams(SdfPath const &shaderId)
{
    return HdShaderParamVector();
}

/*virtual*/
SdfPathVector
HdSceneDelegate::GetSurfaceShaderTextures(SdfPath const &shaderId)
{
    return SdfPathVector();
}

// -----------------------------------------------------------------------//
/// \name Texture Aspects
// -----------------------------------------------------------------------//

/*virtual*/
HdTextureResource::ID
HdSceneDelegate::GetTextureResourceID(SdfPath const& textureId)
{
    return HdTextureResource::ID();
}

/*virtual*/
HdTextureResourceSharedPtr
HdSceneDelegate::GetTextureResource(SdfPath const& textureId)
{
    return HdTextureResourceSharedPtr();
}


// -----------------------------------------------------------------------//
/// \name Camera Aspects
// -----------------------------------------------------------------------//
/*virtual*/
std::vector<GfVec4d>
HdSceneDelegate::GetClipPlanes(SdfPath const& cameraId)
{
    return std::vector<GfVec4d>();
}



// -----------------------------------------------------------------------//
/// \name Primitive Variables
// -----------------------------------------------------------------------//

/*virtual*/
TfTokenVector
HdSceneDelegate::GetPrimVarVertexNames(SdfPath const& id)
{
    return TfTokenVector();
}

/*virtual*/
TfTokenVector
HdSceneDelegate::GetPrimVarVaryingNames(SdfPath const& id)
{
    return TfTokenVector();
}

/*virtual*/
TfTokenVector
HdSceneDelegate::GetPrimVarFacevaryingNames(SdfPath const& id)
{
    return TfTokenVector();
}

/*virtual*/
TfTokenVector
HdSceneDelegate::GetPrimVarUniformNames(SdfPath const& id)
{
    return TfTokenVector();
}

/*virtual*/
TfTokenVector
HdSceneDelegate::GetPrimVarConstantNames(SdfPath const& id)
{
    return TfTokenVector();
}

/*virtual*/
TfTokenVector
HdSceneDelegate::GetPrimVarInstanceNames(SdfPath const& id)
{
    return TfTokenVector();
}

/*virtual*/
int
HdSceneDelegate::GetPrimVarDataType(SdfPath const& id, TfToken const& key)
{
    return 0;
}

/*virtual*/
int
HdSceneDelegate::GetPrimVarComponents(SdfPath const& id, TfToken const& key)
{
    return 0;
}

PXR_NAMESPACE_CLOSE_SCOPE

