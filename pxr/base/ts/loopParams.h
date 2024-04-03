//
// Copyright 2023 Pixar
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

#ifndef PXR_BASE_TS_LOOP_PARAMS_H
#define PXR_BASE_TS_LOOP_PARAMS_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class TsLoopParams
{
public:
    TS_API
    TsLoopParams(
        bool looping,
        TsTime start,
        TsTime period,
        TsTime preRepeatFrames,
        TsTime repeatFrames,
        double valueOffset);

    TS_API
    TsLoopParams();

    TS_API
    void SetLooping(bool looping);

    TS_API
    bool GetLooping() const;

    TS_API
    double GetStart() const;

    TS_API
    double GetPeriod() const;

    TS_API
    double GetPreRepeatFrames() const;

    TS_API
    double GetRepeatFrames() const;

    TS_API
    const GfInterval &GetMasterInterval() const;

    TS_API
    const GfInterval &GetLoopedInterval() const;

    TS_API
    bool IsValid() const;

    TS_API
    void SetValueOffset(double valueOffset);

    TS_API
    double GetValueOffset() const;

    TS_API
    bool operator==(const TsLoopParams &rhs) const {
        return _looping == rhs._looping &&
            _loopedInterval == rhs._loopedInterval &&
            _masterInterval == rhs._masterInterval &&
            _valueOffset == rhs._valueOffset;
    }

    TS_API
    bool operator!=(const TsLoopParams &rhs) const {
        return !(*this == rhs);
    }

private:
    bool _looping;
    GfInterval _loopedInterval;
    GfInterval _masterInterval;
    double _valueOffset;
};

TS_API
std::ostream& operator<<(std::ostream& out, const TsLoopParams& lp);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
