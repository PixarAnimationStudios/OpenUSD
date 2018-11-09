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
#include "pxr/pxr.h"
#include "pxrUsdTranslators/particleWriter.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/transformWriter.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/points.h"

#include <maya/MAnimControl.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MIntArray.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MVectorArray.h>

#include <limits>
#include <memory>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_REGISTER_WRITER(particle, PxrUsdTranslators_ParticleWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(particle, UsdGeomPoints);

PXRUSDMAYA_REGISTER_WRITER(nParticle, PxrUsdTranslators_ParticleWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(nParticle, UsdGeomPoints);


namespace {
    template <typename T>
    inline void _convertVector(T& t, const MVector& v) {
        // I wish we had concepts
        // We are guaranteed to have arithmetic types
        using _t = decltype(+t[0]);
        t[0] = static_cast<_t>(v.x);
        t[1] = static_cast<_t>(v.y);
        t[2] = static_cast<_t>(v.z);
    }

    template <typename T>
    using _sharedVtArray = std::shared_ptr<VtArray<T>>;

    template <typename T>
    _sharedVtArray<T> _convertVectorArray(const MVectorArray& a) {
        const auto count = a.length();
        auto* ret = new VtArray<T>(count);
        for (auto i = decltype(count){0}; i < count; ++i) {
            _convertVector<T>(ret->operator[](i), a[i]);
        }
        return _sharedVtArray<T>(ret);
    }

    template <typename T>
    _sharedVtArray<T> _convertArray(const MDoubleArray& a) {
        const auto count = a.length();
        auto* ret = new VtArray<T>(count);
        for (auto i = decltype(count){0}; i < count; ++i) {
            ret->operator[](i) = static_cast<T>(a[i]);
        }
        return _sharedVtArray<T>(ret);
    }

    template <typename T>
    _sharedVtArray<T> _convertArray(const MIntArray& a) {
        const auto count = a.length();
        auto* ret = new VtArray<T>(count);
        for (auto i = decltype(count){0}; i < count; ++i) {
            ret->operator[](i) = static_cast<T>(a[i]);
        }
        return _sharedVtArray<T>(ret);
    }

    template <typename T>
    using _strVecPair = std::pair<TfToken, _sharedVtArray<T>>;

    template <typename T>
    using _strVecPairVec = std::vector<_strVecPair<T>>;

    template <typename T>
    size_t _minCount(const _strVecPairVec<T>& a) {
        auto mn = std::numeric_limits<size_t>::max();
        for (const auto& v : a) {
            mn = std::min(mn, v.second->size());
        }

        return mn;
    }

    template <typename T>
    void _resizeVectors(_strVecPairVec<T>& a, size_t size) {
        for (const auto& v : a) {
            v.second->resize(size);
        }
    }

    template <typename T>
    inline void _addAttr(UsdGeomPoints& points, const TfToken& name,
                         const SdfValueTypeName& typeName,
                         const VtArray<T>& a, const UsdTimeCode& usdTime,
                         UsdUtilsSparseValueWriter *valueWriter) {
        auto attr = points.GetPrim().CreateAttribute(name, typeName, false, SdfVariabilityVarying);
        VtValue val(a);
        valueWriter->SetAttribute(attr, &val, usdTime);
    }

    const TfToken _rgbName("rgb");
    const TfToken _emissionName("emission");
    const TfToken _opacityName("opacity");
    const TfToken _lifespanName("lifespan");
    const TfToken _massName("mass");

    template <typename T>
    void _addAttrVec(UsdGeomPoints& points, const SdfValueTypeName& typeName,
                     const _strVecPairVec<T>& a,
                     const UsdTimeCode& usdTime,
                     UsdUtilsSparseValueWriter *valueWriter) {
        for (const auto& v : a) {
            _addAttr(points, v.first, typeName, *v.second, usdTime,
                     valueWriter);
        }
    }

    // The logic of filtering the user attributes is based on partio4Maya/PartioExport.
    // https://github.com/redpawfx/partio/blob/redpawfx-rez/contrib/partio4Maya/scripts/partioExportGui.mel
    // We either don't want these or already export them using one of the builtin functions.
    const std::set<std::string> _supressedAttrs = {
        "emitterDataPosition", "emitterDataVelocity", "fieldDataMass",
        "fieldDataPosition", "fieldDataVelocity", "inputGeometryPoints",
        "lastCachedPosition", "lastPosition", "lastVelocity",
        "lastWorldPosition", "lastWorldVelocity", "worldVelocityInObjectSpace",
        "position", "velocity", "acceleration", "rgb", "rgbPP", "incandescencePP",
        "radius", "radiusPP", "age", "opacity", "opacityPP", "lifespan", "lifespanPP",
        "id", "particleId", "mass",
    };

    // All the initial state attributes end with 0
    bool _isInitialAttribute(const std::string& attrName) {
        return attrName.back() == '0';
    }

    bool _isCachedAttribute(const std::string& attrName) {
        constexpr auto _cached = "cached";
        constexpr auto _Cache = "Cache";
        return TfStringEndsWith(attrName, _Cache) || TfStringStartsWith(attrName, _cached);
    }

    bool _isValidAttr(const std::string& attrName) {
        if (attrName.empty()) { return false; }
        if (_isInitialAttribute(attrName)) { return false; }
        if (_isCachedAttribute(attrName)) { return false; }
        return _supressedAttrs.find(attrName) == _supressedAttrs.end();
    }
}

PxrUsdTranslators_ParticleWriter::PxrUsdTranslators_ParticleWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx) :
    UsdMayaTransformWriter(depNodeFn, usdPath, jobCtx),
    mInitialFrameDone(false)
{
    TF_AXIOM(GetDagPath().isValid());

    auto primSchema = UsdGeomPoints::Define(GetUsdStage(), GetUsdPath());
    TF_AXIOM(primSchema);
    _usdPrim = primSchema.GetPrim();
    TF_AXIOM(_usdPrim);

    initializeUserAttributes();
}

/* virtual */
void
PxrUsdTranslators_ParticleWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaTransformWriter::Write(usdTime);

    UsdGeomPoints primSchema(_usdPrim);
    writeParams(usdTime, primSchema);
}

void
PxrUsdTranslators_ParticleWriter::writeParams(
        const UsdTimeCode& usdTime,
        UsdGeomPoints& points)
{
    // XXX: Check this properly, static particles are uncommon, but used.
    if (usdTime.IsDefault()) {
        return;
    }

    MFnParticleSystem particleSys(GetDagPath());
    MFnParticleSystem deformedParticleSys(GetDagPath());

    if (particleSys.isDeformedParticleShape()) {
        const auto origObj = particleSys.originalParticleShape();
        particleSys.setObject(origObj);
    } else {
        const auto defObj = particleSys.deformedParticleShape();
        deformedParticleSys.setObject(defObj);
    }

    const auto particleNode = GetMayaObject();
    if (particleNode.apiType() != MFn::kNParticle) {
        auto currentTime = MAnimControl::currentTime();
        if (mInitialFrameDone) {
            particleSys.evaluateDynamics(currentTime, false);
            deformedParticleSys.evaluateDynamics(currentTime, false);
        } else {
            particleSys.evaluateDynamics(currentTime, true);
            deformedParticleSys.evaluateDynamics(currentTime, true);
            mInitialFrameDone = true;
        }
    }

    // In some cases, especially whenever particles are dying,
    // the length of the attribute vector returned
    // from Maya is smaller than the total number of particles.
    // So we have to first read all the attributes, then
    // determine the minimum amount of particles that all have valid data
    // then write the data out for them in one go.

    const auto particleCount = particleSys.count();
    if (particleCount == 0) {
        return;
    }

    _strVecPairVec<GfVec3f> vectors;
    _strVecPairVec<float> floats;
    _strVecPairVec<int> ints;

    MVectorArray mayaVectors;
    MDoubleArray mayaDoubles;
    MIntArray mayaInts;

    deformedParticleSys.position(mayaVectors);
    auto positions = _convertVectorArray<GfVec3f>(mayaVectors);
    particleSys.velocity(mayaVectors);
    auto velocities = _convertVectorArray<GfVec3f>(mayaVectors);
    particleSys.particleIds(mayaInts);
    auto ids = _convertArray<int64_t>(mayaInts);
    particleSys.radius(mayaDoubles);
    auto radii = _convertArray<float>(mayaDoubles);
    particleSys.mass(mayaDoubles);
    auto masses = _convertArray<float>(mayaDoubles);

    if (particleSys.hasRgb()) {
        particleSys.rgb(mayaVectors);
        vectors.emplace_back(_rgbName, _convertVectorArray<GfVec3f>(mayaVectors));
    }

    if (particleSys.hasEmission()) {
        particleSys.rgb(mayaVectors);
        vectors.emplace_back(_emissionName, _convertVectorArray<GfVec3f>(mayaVectors));
    }

    if (particleSys.hasOpacity()) {
        particleSys.opacity(mayaDoubles);
        floats.emplace_back(_opacityName, _convertArray<float>(mayaDoubles));
    }

    if (particleSys.hasLifespan()) {
        particleSys.lifespan(mayaDoubles);
        floats.emplace_back(_lifespanName, _convertArray<float>(mayaDoubles));
    }

    for (const auto& attr : mUserAttributes) {
        MStatus status;
        switch (std::get<2>(attr)) {
        case PER_PARTICLE_INT:
            particleSys.getPerParticleAttribute(std::get<1>(attr), mayaInts, &status);
            if (status) {
                ints.emplace_back(std::get<0>(attr), _convertArray<int>(mayaInts));
            }
            break;
        case PER_PARTICLE_DOUBLE:
            particleSys.getPerParticleAttribute(std::get<1>(attr), mayaDoubles, &status);
            if (status) {
                floats.emplace_back(std::get<0>(attr), _convertArray<float>(mayaDoubles));
            }
            break;
        case PER_PARTICLE_VECTOR:
            particleSys.getPerParticleAttribute(std::get<1>(attr), mayaVectors, &status);
            if (status) {
                vectors.emplace_back(std::get<0>(attr), _convertVectorArray<GfVec3f>(mayaVectors));
            }
            break;
        }
    }

    const auto minSize = std::min(
        {
            _minCount(vectors), _minCount(floats), _minCount(ints),
            positions->size(), velocities->size(), ids->size(), radii->size(), masses->size()
        }
    );

    if (minSize == 0) {
        return;
    }

    _resizeVectors(vectors, minSize);
    _resizeVectors(floats, minSize);
    _resizeVectors(ints, minSize);
    positions->resize(minSize);
    velocities->resize(minSize);
    ids->resize(minSize);
    radii->resize(minSize);
    masses->resize(minSize);

    _SetAttribute(points.GetPointsAttr(), positions.get(), usdTime);
    _SetAttribute(points.GetVelocitiesAttr(), velocities.get(), usdTime);
    _SetAttribute(points.GetIdsAttr(), ids.get(), usdTime);
    // radius -> width conversion
    for (auto& r : *radii) { r = r * 2.0f; }
    _SetAttribute(points.GetWidthsAttr(), radii.get(), usdTime);

    _addAttr(points, _massName, SdfValueTypeNames->FloatArray, *masses, usdTime,
             _GetSparseValueWriter());
    // TODO: check if we need the array suffix!!
    _addAttrVec(points, SdfValueTypeNames->Vector3fArray, vectors, usdTime,
                _GetSparseValueWriter());
    _addAttrVec(points, SdfValueTypeNames->FloatArray, floats, usdTime,
                _GetSparseValueWriter());
    _addAttrVec(points, SdfValueTypeNames->IntArray, ints, usdTime,
                _GetSparseValueWriter());
}

void
PxrUsdTranslators_ParticleWriter::initializeUserAttributes()
{
    const auto particleNode = GetMayaObject();
    MFnParticleSystem particleSys(GetDagPath());

    const auto attributeCount = particleSys.attributeCount();

    for (auto i = decltype(attributeCount){0}; i < attributeCount; ++i) {
        MObject attrObj = particleSys.attribute(i);
        // we need custom attributes
        if (particleSys.attributeClass(attrObj) == MFnDependencyNode::kNormalAttr) {
            continue;
        }
        // only checking for parent attrs
        MPlug attrPlug(particleNode, attrObj);
        if (!attrPlug.parent().isNull()) { continue; }

        MFnAttribute attr(attrObj);

        const auto mayaAttrName = attr.name();
        const std::string attrName = mayaAttrName.asChar();
        if (!_isValidAttr(attrName)) { continue; }
        if (particleSys.isPerParticleIntAttribute(mayaAttrName)) {
            mUserAttributes.emplace_back(TfToken(attrName), mayaAttrName, PER_PARTICLE_INT);
        } else if (particleSys.isPerParticleDoubleAttribute(mayaAttrName)) {
            mUserAttributes.emplace_back(TfToken(attrName), mayaAttrName, PER_PARTICLE_DOUBLE);
        } else if (particleSys.isPerParticleVectorAttribute(mayaAttrName)) {
            mUserAttributes.emplace_back(TfToken(attrName), mayaAttrName, PER_PARTICLE_VECTOR);
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
