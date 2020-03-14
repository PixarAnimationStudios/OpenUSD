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
#ifndef USD_KATANA_USD_IN_PRIVATE_DATA_H
#define USD_KATANA_USD_IN_PRIVATE_DATA_H

#include "pxr/pxr.h"
#include "usdKatana/usdInArgs.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

#include <FnGeolib/op/FnGeolibOp.h>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


/// \brief Private data for each non-root invocation of \c PxrUsdIn. 
///
/// \sa PxrUsdKatanaUsdInArgs
class PxrUsdKatanaUsdInPrivateData : public Foundry::Katana::GeolibPrivateData
{

public:
    /// \brief Material specialization hierarchy for Usd shading.
    struct MaterialHierarchy {
        std::map<SdfPath, SdfPath> baseMaterialPath;
        // Maintain order of derivedMaterials, for presentation.
        std::map<SdfPath, std::vector<SdfPath>> derivedMaterialPaths;
    };

    /// \brief Pair for associating a USD time with a Katana time.
    struct UsdKatanaTimePair {
        bool operator==(const UsdKatanaTimePair& o) const {
            return usdTime == o.usdTime && katanaTime == o.katanaTime;
        }
        bool operator!=(const UsdKatanaTimePair& o) const {
            return !(*this == o);
        }

        double usdTime;
        double katanaTime;
    };

    PxrUsdKatanaUsdInPrivateData(
            const UsdPrim& prim,
            PxrUsdKatanaUsdInArgsRefPtr usdInArgs,
            const PxrUsdKatanaUsdInPrivateData* parentData = NULL);

    virtual ~PxrUsdKatanaUsdInPrivateData()
    {
        delete _extGb;
    }

    const UsdPrim& GetUsdPrim() const {
        return _prim;
    }

    const PxrUsdKatanaUsdInArgsRefPtr GetUsdInArgs() const {
        return _usdInArgs;
    }

    const SdfPath& GetInstancePath() const {
        return _instancePath;
    }

    const SdfPath& GetMasterPath() const {
        return _masterPath;
    }

    const double GetCurrentTime() const {
        return _currentTime;
    }

    const double GetShutterOpen() const {
        return _shutterOpen;
    }

    const double GetShutterClose() const {
        return _shutterClose;
    }

    /// \brief Return true if motion blur is backward.
    ///
    /// PxrUsdIn supports both forward and backward motion blur. Motion
    /// blur is considered backward if multiple samples are requested
    /// and the first specified sample is later than the last sample.
    const bool IsMotionBackward() const;

    /// \brief Return frame-relative sample times based on how the given
    ///        attribute is sampled with respect to the shutter range.
    ///        If an attribute is not provided, the motion sample times
    ///        specified at a parent location or the default motion sample
    ///        times as specified via the usdInArgs will be used.
    ///
    /// If motion is desired and the given attribute does not have samples
    /// authored within the shutter range, the closest samples to the shutter
    /// boundary will be used for determining the result. If no closest samples
    /// could be found, a single sample time (no motion) will be returned,
    /// unless \p fallBackToShutterBoundary is true, in which case the shutter
    /// start time and/or end time will be used.
    ///
    /// This utility respects the notion of motion sample times overrides
    /// as specified in the usdInArgs' session data. Motion sample times
    /// overrides take precedence over any of the aforementioned logic.
    ///
    const std::vector<double> GetMotionSampleTimes(
        const UsdAttribute& attr = UsdAttribute(),
        bool fallBackToShutterBoundary = false) const;

    /// \brief Returns a list of <usd, katana> times for use in clients that
    ///        wish to multi-sample USD data and build corresponding Katana 
    ///        attributes.
    std::vector<UsdKatanaTimePair> GetUsdAndKatanaTimes(
        const UsdAttribute& attr = UsdAttribute()) const;


    /// \brief Allows a registered op or location decorator function to set
    ///        share and accumulate state during traversal.
    void setExtensionOpArg(const std::string & name,
                FnAttribute::Attribute attr) const;

    /// \brief Allows a registered op or location decorator function to
    ///        retrieve state accumulated during traversal. Arguments set via
    ///        previous consumer's calls to setExtensionOpArg are visible as
    ///        part of the opArgs sent in the op or function.
    FnAttribute::Attribute getExtensionOpArg(const std::string & name,
                FnAttribute::GroupAttribute opArgs) const;

    /// \brief Called by the hosting op to flush the results of
    ///        setExtensionOpArg and apply back onto the provided opArgs.
    ///        NOTE: This should not be called by an executed op or function as
    ///              it's intended for use the callers of those. 
    FnAttribute::GroupAttribute updateExtensionOpArgs(
            FnAttribute::GroupAttribute opArgs) const;
    
    
    /// \brief Access to shared caches relevant to efficient binding of
    ///        materials across the hierarchy.
    UsdShadeMaterialBindingAPI::CollectionQueryCache *
    GetCollectionQueryCache() const;
    UsdShadeMaterialBindingAPI::BindingsCache *
    GetBindingsCache() const;
    


    /// \brief extract private data from either the interface (its natural
    ///        location) with room for future growth
    static PxrUsdKatanaUsdInPrivateData * GetPrivateData(
            const FnKat::GeolibCookInterface& interface);

private:

    UsdPrim _prim;

    PxrUsdKatanaUsdInArgsRefPtr _usdInArgs;

    SdfPath _instancePath;
    SdfPath _masterPath;

    double _currentTime;
    double _shutterOpen;
    double _shutterClose;

    std::vector<double> _motionSampleTimesOverride;
    std::vector<double> _motionSampleTimesFallback;
    
    
    mutable FnAttribute::GroupBuilder * _extGb;



    typedef std::shared_ptr<UsdShadeMaterialBindingAPI::CollectionQueryCache>
            _CollectionQueryCachePtr;
    typedef std::shared_ptr<UsdShadeMaterialBindingAPI::BindingsCache>
            _BindingsCachePtr;

    _CollectionQueryCachePtr _collectionQueryCache;
    _BindingsCachePtr _bindingsCache;


};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_KATANA_USD_IN_PRIVATE_DATA_H
