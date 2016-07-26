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
#ifndef PXRUSDMAYA_PRIMREADERARGS_H
#define PXRUSDMAYA_PRIMREADERARGS_H

/// \file primReaderArgs.h

#include "pxr/usd/usd/prim.h"

/// \class PxrUsdMayaPrimReaderArgs
/// \brief This class holds read-only arguments that are passed into reader plugins for
/// the usdMaya library.
/// 
/// \sa PxrUsdMayaPrimReaderContext
class PxrUsdMayaPrimReaderArgs
{
public:
    PxrUsdMayaPrimReaderArgs(
            const UsdPrim& prim,
            const TfToken& shadingMode,
            const TfToken& defaultMeshScheme,
            const bool readAnimData);

    /// \brief return the usd prim that should be read.
    const UsdPrim& GetUsdPrim() const;

    const TfToken& GetShadingMode() const;

    const TfToken& GetDefaultMeshScheme() const;

    const bool& GetReadAnimData() const;
    
    bool ShouldImportUnboundShaders() const {
        // currently this is disabled.
        return false;
    }

private:
    const UsdPrim& _prim;
    const TfToken& _shadingMode;
    const TfToken& _defaultMeshScheme;
    const bool _readAnimData;
};

#endif // PXRUSDMAYA_PRIMREADERARGS_H
