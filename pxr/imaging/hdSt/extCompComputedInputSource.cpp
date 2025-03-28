//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extCompComputedInputSource.h"
#include "pxr/imaging/hdSt/extCompCpuComputation.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSt_ExtCompComputedInputSource::HdSt_ExtCompComputedInputSource(
    const TfToken &name,
    const HdStExtCompCpuComputationSharedPtr &source,
    const TfToken &sourceOutputName)
    : HdSt_ExtCompInputSource(name)
    , _source(source)
    , _sourceOutputIdx(HdStExtCompCpuComputation::INVALID_OUTPUT_INDEX)
{
    _sourceOutputIdx = source->GetOutputIndex(sourceOutputName);
}

HdSt_ExtCompComputedInputSource::~HdSt_ExtCompComputedInputSource() = default;

bool
HdSt_ExtCompComputedInputSource::Resolve()
{
    bool sourceValid = _source->IsValid();
    if (sourceValid) {
        if (!_source->IsResolved()) {
            return false;
        }
    }

    if (!_TryLock()) return false;

    if (!sourceValid || _source->HasResolveError()) {
        _SetResolveError();
        return true;
    }

    _SetResolved();
    return true;
}

const VtValue &
HdSt_ExtCompComputedInputSource::GetValue() const
{
    return _source->GetOutputByIndex(_sourceOutputIdx);
}


bool
HdSt_ExtCompComputedInputSource::_CheckValid() const
{
    return (_source &&
            (_sourceOutputIdx !=
                        HdStExtCompCpuComputation::INVALID_OUTPUT_INDEX));
}


PXR_NAMESPACE_CLOSE_SCOPE

