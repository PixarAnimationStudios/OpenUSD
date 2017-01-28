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
#include "usdMaya/primReaderArgs.h"

PXR_NAMESPACE_OPEN_SCOPE


PxrUsdMayaPrimReaderArgs::PxrUsdMayaPrimReaderArgs(
        const UsdPrim& prim,
        const TfToken& shadingMode,
        const TfToken& defaultMeshScheme,
        const bool readAnimData,
        const bool useCustomFrameRange,
        const double startTime,
        const double endTime)
    : 
        _prim(prim),
        _shadingMode(shadingMode),
        _defaultMeshScheme(defaultMeshScheme),
        _readAnimData(readAnimData),
        _useCustomFrameRange(useCustomFrameRange),
        _startTime(startTime),
        _endTime(endTime)
{
}
const UsdPrim&
PxrUsdMayaPrimReaderArgs::GetUsdPrim() const
{
    return _prim;
}
const TfToken&
PxrUsdMayaPrimReaderArgs::GetShadingMode() const
{
    return _shadingMode;
}
const TfToken&
PxrUsdMayaPrimReaderArgs::GetDefaultMeshScheme() const
{
    return _defaultMeshScheme;
}
const bool&
PxrUsdMayaPrimReaderArgs::GetReadAnimData() const
{
    return _readAnimData;
}
bool 
PxrUsdMayaPrimReaderArgs::HasCustomFrameRange() const
{
    return _useCustomFrameRange;
}
double 
PxrUsdMayaPrimReaderArgs::GetStartTime() const
{
    return _startTime;
}
double 
PxrUsdMayaPrimReaderArgs::GetEndTime() const
{
    return _endTime;
}

PXR_NAMESPACE_CLOSE_SCOPE

