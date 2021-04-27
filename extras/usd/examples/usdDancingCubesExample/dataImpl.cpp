//
// Copyright 2019 Pixar
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
#include "dataImpl.h"
#include "pxr/usd/sdf/schema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// All leaf prims have the same properties, so we set up some static data about
// these properties that will always be true.

// Define tokens for the property names we know about from usdGeom
TF_DEFINE_PRIVATE_TOKENS(
    _propertyNameTokens,
    ((xformOpOrder, "xformOpOrder"))
    ((xformOpTranslate, "xformOp:translate"))
    ((xformOpRotateXYZ, "xformOp:rotateXYZ"))
    ((displayColor, "primvars:displayColor"))
);

// We create a static map from property names to the info about them that
// we'll be querying for specs.
struct _LeafPrimPropertyInfo
{
    VtValue defaultValue;
    TfToken typeName;
    // Most of our properties are aniated.
    bool isAnimated{true};
};

using _LeafPrimPropertyMap =
    std::map<TfToken, _LeafPrimPropertyInfo, TfTokenFastArbitraryLessThan>;

TF_MAKE_STATIC_DATA(
    (_LeafPrimPropertyMap), _LeafPrimProperties) {

    // Define the default value types for our animated properties.
    (*_LeafPrimProperties)[_propertyNameTokens->xformOpTranslate].defaultValue = 
        VtValue(GfVec3d(0));
    (*_LeafPrimProperties)[_propertyNameTokens->xformOpRotateXYZ].defaultValue = 
        VtValue(GfVec3f(0));
    (*_LeafPrimProperties)[_propertyNameTokens->displayColor].defaultValue = 
        VtValue(VtVec3fArray({GfVec3f(1)}));

    // xformOpOrder is a non-animated property and is specifically translate, 
    // rotate for all our geom prims.
    (*_LeafPrimProperties)[_propertyNameTokens->xformOpOrder].defaultValue = 
        VtValue(VtTokenArray{_propertyNameTokens->xformOpTranslate,
                             _propertyNameTokens->xformOpRotateXYZ});
    (*_LeafPrimProperties)[_propertyNameTokens->xformOpOrder].isAnimated = false;

    // Use the schema to derive the type name tokens from each property's 
    // default value.
    for (auto &it: *_LeafPrimProperties) {
        it.second.typeName =
            SdfSchema::GetInstance().FindType(it.second.defaultValue).GetAsToken();
    }
}

// Helper function for getting the root prim path.
static const SdfPath &_GetRootPrimPath()
{
    static const SdfPath rootPrimPath("/Root");
    return rootPrimPath;
}

};

// Helper macro for many of our functions need to optionally set an output 
// VtValue when returning true. 
#define RETURN_TRUE_WITH_OPTIONAL_VALUE(val) \
    if (value) { *value = VtValue(val); } \
    return true;

UsdDancingCubesExample_DataImpl::UsdDancingCubesExample_DataImpl() 
{
    _params.perSide = 0;
    _InitFromParams();
}

UsdDancingCubesExample_DataImpl::UsdDancingCubesExample_DataImpl(
    const UsdDancingCubesExample_DataParams &params) 
    : _params(params)
{
    _InitFromParams();
}

SdfSpecType 
UsdDancingCubesExample_DataImpl::GetSpecType(
    const SdfPath &path) const
{
    // All specs are generated.
    if (path.IsPropertyPath()) {
        // A specific set of defined properties exist on the leaf prims only
        // as attributes. Non leaf prims have no properties.
        if (_LeafPrimProperties->count(path.GetNameToken()) && 
            _leafPrimDataMap.count(path.GetAbsoluteRootOrPrimPath())) {
            return SdfSpecTypeAttribute;
        }
    } else {
        // Special case for pseudoroot.
        if (path == SdfPath::AbsoluteRootPath()) {
            return SdfSpecTypePseudoRoot;
        }
        // All other valid prim spec paths are cached.
        if (_primSpecPaths.count(path)) {
            return SdfSpecTypePrim;
        }
    }

    return SdfSpecTypeUnknown;
}

bool 
UsdDancingCubesExample_DataImpl::Has(
    const SdfPath &path, const TfToken &field, VtValue *value) const
{
    if (_primSpecPaths.empty()) {
        return false;
    }

    // If property spec, check property fields
    if (path.IsPropertyPath()) {

        if (field == SdfFieldKeys->TypeName) {
            return _HasPropertyTypeNameValue(path, value);
        } else if (field == SdfFieldKeys->Default) {
            return _HasPropertyDefaultValue(path, value);
        } else if (field == SdfFieldKeys->TimeSamples) {
            // Only animated properties have time samples.
            if (_IsAnimatedProperty(path)) {
                // Will need to generate the full SdfTimeSampleMap with a 
                // time sample value for each discrete animated frame if the 
                // value of the TimeSamples field is requested. Use a generator
                // function in case we don't need to output the value as this
                // can be expensive.
                auto _MakeTimeSampleMap = [this, &path]() {
                    SdfTimeSampleMap sampleMap;
                    for (auto time : _animTimeSampleTimes) {
                         QueryTimeSample(path, time, &sampleMap[time]);
                    }
                    return sampleMap;
                };
                
                RETURN_TRUE_WITH_OPTIONAL_VALUE(_MakeTimeSampleMap());
            }
        } 
    } else if (path == SdfPath::AbsoluteRootPath()) {
        // Special case check for the pseudoroot prim spec.
        if (field == SdfChildrenKeys->PrimChildren) {
            // Pseudoroot only has the root prim as a child
            static TfTokenVector rootChildren(
                {_GetRootPrimPath().GetNameToken()});
            RETURN_TRUE_WITH_OPTIONAL_VALUE(rootChildren);
        }
        // Default prim is always the root prim.
        if (field == SdfFieldKeys->DefaultPrim) {
            RETURN_TRUE_WITH_OPTIONAL_VALUE(_GetRootPrimPath().GetNameToken());
        }
        // Start time code is always 0
        if (field == SdfFieldKeys->StartTimeCode) {
            RETURN_TRUE_WITH_OPTIONAL_VALUE(0.0);
        }
        // End time code is always num frames - 1
        if (field == SdfFieldKeys->EndTimeCode) {
            RETURN_TRUE_WITH_OPTIONAL_VALUE(double(_params.numFrames - 1));
        }

    } else {
        // Otherwise check prim spec fields.
        if (field == SdfFieldKeys->Specifier) {
            // All our prim specs use the "def" specifier.
            if (_primSpecPaths.count(path)) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(SdfSpecifierDef);
            }
        }

        if (field == SdfFieldKeys->TypeName) {
            // Only the leaf prim specs have a type name determined from the 
            // params.
            if (_leafPrimDataMap.count(path)) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(_params.geomType);
            }
        }

        if (field == SdfChildrenKeys->PrimChildren) {
            // Non-leaf prims have the prim children. The list is the same set 
            // of prim child names for each non-leaf prim regardless of depth.
            if (_primSpecPaths.count(path) && !_leafPrimDataMap.count(path)) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(_primChildNames);
            }
        }

        if (field == SdfChildrenKeys->PropertyChildren) {
            // Leaf prims have the same specified set of property children.
            if (_leafPrimDataMap.count(path)) {
                RETURN_TRUE_WITH_OPTIONAL_VALUE(_propertyNameTokens->allTokens);
            }
        }
    }

    return false;
}

void 
UsdDancingCubesExample_DataImpl::VisitSpecs(
    const SdfAbstractData &data, SdfAbstractDataSpecVisitor *visitor) const
{
    // Visit the pseudoroot.
    if (!visitor->VisitSpec(data, SdfPath::AbsoluteRootPath())) {
        return;
    }
    // Visit all the cached prim spec paths.
    for (const auto &path: _primSpecPaths) {
        if (!visitor->VisitSpec(data, path)) {
            return;
        }
    }
    // Visit the property specs which exist only on leaf prims.
    for (const auto &it : _leafPrimDataMap) {
        for (const TfToken &propertyName : _propertyNameTokens->allTokens) {
            if (!visitor->VisitSpec(
                    data, it.first.AppendProperty(propertyName))) {
                return;
            }
        }
    }
}

const std::vector<TfToken> &
UsdDancingCubesExample_DataImpl::List(const SdfPath &path) const
{
    static std::vector<TfToken> empty;
    if (_primSpecPaths.empty()) {
        return empty;
    }

    if (path.IsPropertyPath()) {
        // For properties, check that it's a valid leaf prim property
        const _LeafPrimPropertyInfo *propInfo = 
            TfMapLookupPtr(*_LeafPrimProperties, path.GetNameToken());
        if (propInfo &&_leafPrimDataMap.count(path.GetAbsoluteRootOrPrimPath())) {
            // Include time sample field in the property is animated.
            if (propInfo->isAnimated) {
                static std::vector<TfToken> animPropFields(
                    {SdfFieldKeys->TypeName,
                     SdfFieldKeys->Default,
                     SdfFieldKeys->TimeSamples});
                return animPropFields;
            } else {
                static std::vector<TfToken> nonAnimPropFields(
                    {SdfFieldKeys->TypeName,
                     SdfFieldKeys->Default});
                return nonAnimPropFields;
            }
        }
    } else if (path == SdfPath::AbsoluteRootPath()) {
        // Pseudoroot fields.
        static std::vector<TfToken> pseudoRootFields(
            {SdfChildrenKeys->PrimChildren,
             SdfFieldKeys->DefaultPrim,
             SdfFieldKeys->StartTimeCode,
             SdfFieldKeys->EndTimeCode});
        return pseudoRootFields;
    } else if (_primSpecPaths.count(path)) {
        // Prim spec. Different fields for leaf and non-leaf prims.
        if (_leafPrimDataMap.count(path)) {
            static std::vector<TfToken> leafPrimFields(
                {SdfFieldKeys->Specifier,
                 SdfFieldKeys->TypeName,
                 SdfChildrenKeys->PropertyChildren});
            return leafPrimFields;
        } else {
            static std::vector<TfToken> nonLeafPrimFields(
                {SdfFieldKeys->Specifier,
                 SdfChildrenKeys->PrimChildren});
            return nonLeafPrimFields;
        }
    }

    return empty;
}

const std::set<double> &
UsdDancingCubesExample_DataImpl::ListAllTimeSamples() const
{
    // The set of all time sample times is cached.
    return _animTimeSampleTimes;
}

const std::set<double> &
UsdDancingCubesExample_DataImpl::ListTimeSamplesForPath(
    const SdfPath &path) const
{
    // All animated properties use the same set of time samples; all other
    // specs return empty.
    if (_IsAnimatedProperty(path)) {
        return _animTimeSampleTimes;
    }
    static std::set<double> empty;
    return empty;
}

bool 
UsdDancingCubesExample_DataImpl::GetBracketingTimeSamples(
    double time, double *tLower, double *tUpper) const
{
    // A time sample time will exist at each discrete integer frame for the 
    // duration of the generated animation and will already be cached.
    if (_animTimeSampleTimes.empty()) {
        return false;
    }

    // First time sample is always zero.
    if (time <= 0) {
        *tLower = *tUpper = 0;
        return true;
    }
    // Last time sample will alway be size - 1.
    if (time >= _animTimeSampleTimes.size() - 1) {
        *tLower = *tUpper = _animTimeSampleTimes.size() - 1;
        return true;
    }
    // Lower bound is the integer time. Upper bound will be the same unless the
    // time itself is non-integer, in which case it'll be the next integer time.
    *tLower = *tUpper = int(time);
    if (time > *tUpper) {
        *tUpper += 1.0;
    }
    return true;
}

size_t 
UsdDancingCubesExample_DataImpl::GetNumTimeSamplesForPath(
    const SdfPath &path) const
{
    // All animated properties use the same set of time samples; all other specs
    // have no time samples.
    if (_IsAnimatedProperty(path)) {
        return _animTimeSampleTimes.size();
    }
    return 0;
}

bool 
UsdDancingCubesExample_DataImpl::GetBracketingTimeSamplesForPath(
    const SdfPath &path, double time, 
    double *tLower, double *tUpper) const
{
    // All animated properties use the same set of time samples.
    if (_IsAnimatedProperty(path)) {
        return GetBracketingTimeSamples(time, tLower, tUpper);
    }

    return false;
}

bool 
UsdDancingCubesExample_DataImpl::QueryTimeSample(
    const SdfPath &path, double time, VtValue *value) const
{
    // Only leaf prim properties have time samples
    const _LeafPrimData *val =
        TfMapLookupPtr(_leafPrimDataMap, path.GetAbsoluteRootOrPrimPath());
    if (!val) {
        return false;
    }

    // Each leaf prim has an animation offset time that has been precomputed
    // based off its location in the layout of the geom prims. This offset
    // is added to the query time to offset the animation loop for each prim.
    double offsetTime = time + val->frameOffset;

    if (path.GetNameToken() == _propertyNameTokens->xformOpTranslate) {
        // Animated position, anchored at the prim's layout position.
        RETURN_TRUE_WITH_OPTIONAL_VALUE(
            val->pos + GfVec3d(_GetTranslateOffset(offsetTime)));
    }
    if (path.GetNameToken() == _propertyNameTokens->xformOpRotateXYZ) {
        // Animated rotation.
        RETURN_TRUE_WITH_OPTIONAL_VALUE(
            GfVec3f(_GetRotateAmount(offsetTime)));
    }
    if (path.GetNameToken() == _propertyNameTokens->displayColor) {
        // Animated color value.
        RETURN_TRUE_WITH_OPTIONAL_VALUE(
            VtVec3fArray({_GetColor(offsetTime)}));
    }
    return false;
}

void 
UsdDancingCubesExample_DataImpl::_InitFromParams()
{
    if (_params.perSide <= 0) {
        return;
    }

    // Layer always has a root spec that is the default prim of the layer.
    _primSpecPaths.insert(_GetRootPrimPath());

    // Cache the list of prim child names, numbered 0 to perSide
    _primChildNames.resize(_params.perSide);
    for (int i = 0; i < _params.perSide; ++ i) {
        _primChildNames[i] = TfToken(TfStringPrintf("prim_%d",i));
    }

    // Origin of the containing cube.
    const GfVec3d origin(-0.5 * _params.perSide);
    // Step value used in computing the animation time offset base on position
    // in the cube layout
    const double frameOffsetAmount = 
        _params.framesPerCycle / (3.0 * _params.perSide);

    // The layout is a cube of geom prims. We build up each dimension of this
    // cube as a hierarchy of child prims.
    for (int i = 0; i < _params.perSide; ++i) {
        // Cache prim spec paths at depth 1 as children of the root prim
        SdfPath iPath = _GetRootPrimPath().AppendChild(_primChildNames[i]);
        _primSpecPaths.insert(iPath);
        for (int j = 0; j < _params.perSide; ++j) {
            // Cache prim spec paths at depth 2 as children of each depth 1 prim
            SdfPath jPath = iPath.AppendChild(_primChildNames[j]);
            _primSpecPaths.insert(jPath);
            for (int k = 0; k < _params.perSide; ++k) {
                // Cache prim spec paths at depth 3 as children of each depth 2 
                // prim.
                SdfPath kPath = jPath.AppendChild(_primChildNames[k]);
                _primSpecPaths.insert(kPath);
                // These are leaf prims which will have geometry and animation
                // Cache the starting locations of this prims and the animation
                // offset frame for each.
                _LeafPrimData &indexData = _leafPrimDataMap[kPath];
                indexData.pos = (origin + GfVec3d(i, j, k)) * _params.distance;
                indexData.frameOffset = frameOffsetAmount * (i+j+k);
            }
        }
    }

    // Skip animation data if there will be no frames.
    if (_params.numFrames <= 0 || _params.framesPerCycle <= 0) {
        return;
    }

    // Cache the anim time sample times as there will always be one per each 
    // discrete frame.
    for (int f = 0; f < _params.numFrames; ++f) {
        _animTimeSampleTimes.insert(_animTimeSampleTimes.end(), f);
    }

    // Cache the sin wave based animation values, which are used for translation
    // and color, for each distinct frame. We only store one value per discrete 
    // frame in a single cycle and share this among all animate prims. Each
    // animated prim has a frame offset that may cause its animation time to 
    // fall between these store frames, but we handle that by lerping between
    // the sample values.
    _animCycleSampleData.resize(_params.framesPerCycle);
    for (int f = 0; f < _params.framesPerCycle; ++f) {
        double t = double(f) / _params.framesPerCycle;
        double angle = (t) * 2.0 * M_PI;
        double cos, sin;
        GfSinCos(angle, &sin, &cos);
        _AnimData &animData = _animCycleSampleData[f];
        animData.transOffset = sin * _params.distance * _params.moveScale * 0.5;
        animData.color = GfVec3f((sin+1)/2, (cos+1)/2, (1-sin)/2);
    }
}

bool 
UsdDancingCubesExample_DataImpl::_IsAnimatedProperty(
    const SdfPath &path) const
{
    // Check that it is a property id.
    if (!path.IsPropertyPath()) {
        return false;
    }
    // Check that its one of our animated property names.
    const _LeafPrimPropertyInfo *propInfo = 
        TfMapLookupPtr(*_LeafPrimProperties, path.GetNameToken());
    if (!(propInfo && propInfo->isAnimated)) {
        return false;
    }
    // Check that it belongs to a leaf prim.
    return TfMapLookupPtr(_leafPrimDataMap, path.GetAbsoluteRootOrPrimPath());
}

bool 
UsdDancingCubesExample_DataImpl::_HasPropertyDefaultValue(
    const SdfPath &path, VtValue *value) const
{
    // Check that it is a property id.
    if (!path.IsPropertyPath()) {
        return false;
    }

    // Check that it is one of our property names.
    const _LeafPrimPropertyInfo *propInfo = 
        TfMapLookupPtr(*_LeafPrimProperties, path.GetNameToken());
    if (!propInfo) {
        return false;
    }

    // Check that it belongs to a leaf prim before getting the default value
    const _LeafPrimData *val = 
        TfMapLookupPtr(_leafPrimDataMap, path.GetAbsoluteRootOrPrimPath());
    if (val) {
        if (value) {
            // Special case for translate property. Each leaf prim has its own
            // default position.
            if (path.GetNameToken() == _propertyNameTokens->xformOpTranslate) {
                *value = VtValue(val->pos);
            } else {
                *value = propInfo->defaultValue;
            }
        }
        return true;
    }

    return false;
}

bool 
UsdDancingCubesExample_DataImpl::_HasPropertyTypeNameValue(
    const SdfPath &path, VtValue *value) const
{
    // Check that it is a property id.
    if (!path.IsPropertyPath()) {
        return false;
    }

    // Check that it is one of our property names.
    const _LeafPrimPropertyInfo *propInfo = 
        TfMapLookupPtr(*_LeafPrimProperties, path.GetNameToken());
    if (!propInfo) {
        return false;
    }

    // Check that it belongs to a leaf prim before getting the type name value
    const _LeafPrimData *val = 
        TfMapLookupPtr(_leafPrimDataMap, path.GetAbsoluteRootOrPrimPath());
    if (val) {
        if (value) {
            *value = VtValue(propInfo->typeName);
        }
        return true;
    }

    return false;
}

double 
UsdDancingCubesExample_DataImpl::_GetTranslateOffset(double time) const
{
    // Animated translation data is cached at integer frames. But each cube's
    // frame offset can be non-integer so we may have to lerp between two
    // samples. Anim data loops as well.
    int prevFrameIdx = int(time);
    double alpha = time - prevFrameIdx;
    int nextFrameIdx = (prevFrameIdx + 1) % _animCycleSampleData.size();
    prevFrameIdx = prevFrameIdx % _animCycleSampleData.size();
    return GfLerp(alpha, 
                  _animCycleSampleData[prevFrameIdx].transOffset,
                  _animCycleSampleData[nextFrameIdx].transOffset);
}

double 
UsdDancingCubesExample_DataImpl::_GetRotateAmount(double time) const
{
    // Rotation value wasn't cached as it's just a linear function over time.
    return 360.0 * time / _animCycleSampleData.size();
}

GfVec3f 
UsdDancingCubesExample_DataImpl::_GetColor(double time) const
{
    // Animated color data is cached and computed like translation data.
    int prevFrameIdx = int(time);
    double alpha = time - prevFrameIdx;
    int nextFrameIdx = (prevFrameIdx + 1) % _animCycleSampleData.size();
    prevFrameIdx = prevFrameIdx % _animCycleSampleData.size();
    return GfLerp(alpha,
                  _animCycleSampleData[prevFrameIdx].color,
                  _animCycleSampleData[nextFrameIdx].color);
}

PXR_NAMESPACE_CLOSE_SCOPE
