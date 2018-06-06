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
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "pxr/usd/usdSkel/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _WrapStaticToken {
public:
    _WrapStaticToken(const TfToken* token) : _token(token) { }

    std::string operator()() const
    {
        return _token->GetString();
    }

private:
    const TfToken* _token;
};

template <typename T>
void
_AddToken(T& cls, const char* name, const TfToken& token)
{
    cls.add_static_property(name,
                            boost::python::make_function(
                                _WrapStaticToken(&token),
                                boost::python::return_value_policy<
                                    boost::python::return_by_value>(),
                                boost::mpl::vector1<std::string>()));
}

} // anonymous

void wrapUsdSkelTokens()
{
    boost::python::class_<UsdSkelTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "blendShapes", UsdSkelTokens->blendShapes);
    _AddToken(cls, "blendShapeWeights", UsdSkelTokens->blendShapeWeights);
    _AddToken(cls, "joints", UsdSkelTokens->joints);
    _AddToken(cls, "offsets", UsdSkelTokens->offsets);
    _AddToken(cls, "pointIndices", UsdSkelTokens->pointIndices);
    _AddToken(cls, "primvarsSkelGeomBindTransform", UsdSkelTokens->primvarsSkelGeomBindTransform);
    _AddToken(cls, "primvarsSkelJointIndices", UsdSkelTokens->primvarsSkelJointIndices);
    _AddToken(cls, "primvarsSkelJointWeights", UsdSkelTokens->primvarsSkelJointWeights);
    _AddToken(cls, "restTransforms", UsdSkelTokens->restTransforms);
    _AddToken(cls, "rotations", UsdSkelTokens->rotations);
    _AddToken(cls, "scales", UsdSkelTokens->scales);
    _AddToken(cls, "skelAnimationSource", UsdSkelTokens->skelAnimationSource);
    _AddToken(cls, "skelBlendShapes", UsdSkelTokens->skelBlendShapes);
    _AddToken(cls, "skelBlendShapeTargets", UsdSkelTokens->skelBlendShapeTargets);
    _AddToken(cls, "skelJoints", UsdSkelTokens->skelJoints);
    _AddToken(cls, "skelSkeleton", UsdSkelTokens->skelSkeleton);
    _AddToken(cls, "skelSkeletonInstance", UsdSkelTokens->skelSkeletonInstance);
    _AddToken(cls, "translations", UsdSkelTokens->translations);
    _AddToken(cls, "weight", UsdSkelTokens->weight);
}
