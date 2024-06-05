//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
