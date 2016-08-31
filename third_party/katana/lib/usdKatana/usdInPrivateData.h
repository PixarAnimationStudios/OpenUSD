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
#ifndef PXRUSDKATANA_USDIN_PRIVATEDATA_H
#define PXRUSDKATANA_USDIN_PRIVATEDATA_H

#include "usdKatana/api.h"
#include "usdKatana/usdInArgs.h"

#include "pxr/usd/usd/prim.h"
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif
#undef interface // Defined on Windows.
#include <FnGeolib/op/FnGeolibOp.h>

/// \brief Private data for each non-root invocation of \c PxrUsdIn. 
///
/// \sa PxrUsdKatanaUsdInArgs
class PxrUsdKatanaUsdInPrivateData : public Foundry::Katana::GeolibPrivateData
{

public:
    USDKATANA_API
    PxrUsdKatanaUsdInPrivateData(
            const UsdPrim& prim,
            PxrUsdKatanaUsdInArgsRefPtr usdInArgs,
            const PxrUsdKatanaUsdInPrivateData* parentData = NULL);

    virtual ~PxrUsdKatanaUsdInPrivateData()
    {
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

    USDKATANA_API
    const std::vector<double> GetMotionSampleTimes(const UsdAttribute& attr = UsdAttribute()) const;

private:

    UsdPrim _prim;
    
    PxrUsdKatanaUsdInArgsRefPtr _usdInArgs;

    SdfPath _instancePath;
    SdfPath _masterPath;
    
};

#endif // PXRUSDKATANA_USDIN_PRIVATEDATA_H
