//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/knotData.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/ts/valueTypeDispatch.h"

#include "pxr/base/tf/enum.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
// Construction and value semantics

TsKnot::TsKnot()
    : TsKnot(Ts_GetType<double>())
{
}

TsKnot::TsKnot(
    TfType valueType)
    : _data(Ts_KnotData::Create(valueType)),
      _proxy(Ts_KnotDataProxy::Create(_data, valueType))
{
}

TsKnot::TsKnot(
    TfType valueType,
    TsCurveType curveType)
    : TsKnot(valueType)
{
    // SetCurveType has been deprecated and will be removed in a future release.
    SetCurveType(curveType);
}

TsKnot::TsKnot(
    Ts_KnotData* const data,
    const TfType valueType,
    VtDictionary &&customData)
    : _data(data),
      _proxy(Ts_KnotDataProxy::Create(_data, valueType)),
      _customData(std::move(customData))
{
}

TsKnot::TsKnot(const TsKnot &other)
    : _data(other._proxy->CloneData()),
      _proxy(Ts_KnotDataProxy::Create(_data, other._proxy->GetValueType())),
      _customData(other._customData)
{
}

TsKnot::TsKnot(TsKnot &&other)
    : _data(other._data),
      _proxy(other._proxy.release()),
      _customData(std::move(other._customData))
{
    // Leave other with valid data.
    const TfType valueType = _proxy->GetValueType();
    other._data = Ts_KnotData::Create(valueType);
    other._proxy = Ts_KnotDataProxy::Create(other._data, valueType);
}

TsKnot::~TsKnot()
{
    if (_proxy && _data)
    {
        _proxy->DeleteData();
    }
}

TsKnot& TsKnot::operator=(const TsKnot &other)
{
    // Deallocate old data.
    _proxy->DeleteData();

    // Copy member data.
    _data = other._proxy->CloneData();
    _proxy = Ts_KnotDataProxy::Create(_data, other._proxy->GetValueType());
    _customData = other._customData;

    return *this;
}

TsKnot& TsKnot::operator=(TsKnot &&other)
{
    // Deallocate old data.
    _proxy->DeleteData();

    // Move member data.
    _data = other._data;
    _proxy = std::move(other._proxy);
    _customData = std::move(other._customData);

    // Leave other with valid data.
    const TfType valueType = _proxy->GetValueType();
    other._data = Ts_KnotData::Create(valueType);
    other._proxy = Ts_KnotDataProxy::Create(other._data, valueType);

    return *this;
}

bool TsKnot::operator==(const TsKnot &other) const
{
    if (other.GetValueType() != GetValueType())
    {
        return false;
    }

    return _proxy->IsDataEqualTo(*other._data)
        && _customData == other._customData;
}

bool TsKnot::operator!=(const TsKnot &other) const
{
    return !(*this == other);
}

////////////////////////////////////////////////////////////////////////////////
// Knot Time

bool TsKnot::SetTime(const TsTime time)
{
    if (!Ts_IsFinite(time))
    {
        TF_CODING_ERROR("Knot time must be finite.");
        return false;
    }

    _data->time = time;
    return true;
}

TsTime TsKnot::GetTime() const
{
    return _data->time;
}

////////////////////////////////////////////////////////////////////////////////
// Interpolation mode

bool TsKnot::SetNextInterpolation(
    const TsInterpMode mode)
{
    _data->nextInterp = mode;
    return true;
}

TsInterpMode TsKnot::GetNextInterpolation() const
{
    return _data->nextInterp;
}

////////////////////////////////////////////////////////////////////////////////
// Knot Value

TfType TsKnot::GetValueType() const
{
    return _proxy->GetValueType();
}

bool TsKnot::SetValue(const VtValue value)
{
    if (!_CheckInParamVt(value))
    {
        return false;
    }

    _proxy->SetValue(value);
    return true;
}

bool TsKnot::GetValue(VtValue* const valueOut) const
{
    if (!_CheckOutParamVt(valueOut))
    {
        return false;
    }

    _proxy->GetValue(valueOut);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Dual Values

bool TsKnot::IsDualValued() const
{
    return _data->dualValued;
}

bool TsKnot::SetPreValue(const VtValue value)
{
    if (!_CheckInParamVt(value))
    {
        return false;
    }

    _data->dualValued = true;
    _proxy->SetPreValue(value);
    return true;
}

bool TsKnot::GetPreValue(VtValue* const valueOut) const
{
    if (!_CheckOutParamVt(valueOut))
    {
        return false;
    }

    if (_data->dualValued)
    {
        _proxy->GetPreValue(valueOut);
    }
    else
    {
        _proxy->GetValue(valueOut);
    }

    return true;
}

bool TsKnot::ClearPreValue()
{
    _data->dualValued = false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Curve type

bool TsKnot::SetCurveType(const TsCurveType curveType)
{
    TF_WARN("TsKnot::SetCurveType has been deprecated.");
    _data->curveType = curveType;
    return true;
}

TsCurveType TsKnot::GetCurveType() const
{
    TF_WARN("TsKnot::GetCurveType has been deprecated.");
    return _data->curveType;
}

////////////////////////////////////////////////////////////////////////////////
// Pre-Tangent

bool TsKnot::SetPreTanWidth(const TsTime width)
{
    if (!_CheckSetWidth(width))
    {
        return false;
    }

    _data->preTanWidth = width;
    return true;
}

TsTime TsKnot::GetPreTanWidth() const
{
    return _data->GetPreTanWidth();
}

bool TsKnot::SetPreTanSlope(const VtValue slope)
{
    if (!_CheckInParamVt(slope))
    {
        return false;
    }

    _proxy->SetPreTanSlope(slope);
    return true;
}

bool TsKnot::GetPreTanSlope(VtValue* const slopeOut) const
{
    if (!_CheckOutParamVt(slopeOut))
    {
        return false;
    }

    _proxy->GetPreTanSlope(slopeOut);
    return true;
}

bool TsKnot::SetPreTanAlgorithm(const TsTangentAlgorithm algorithm)
{
    _data->preTanAlgorithm = algorithm;
    return true;
}

TsTangentAlgorithm TsKnot::GetPreTanAlgorithm() const
{
    return _data->preTanAlgorithm;
}


////////////////////////////////////////////////////////////////////////////////
// Post-Tangent

bool TsKnot::SetPostTanWidth(const TsTime width)
{
    if (!_CheckSetWidth(width))
    {
        return false;
    }

    _data->postTanWidth = width;
    return true;
}

TsTime TsKnot::GetPostTanWidth() const
{
    return _data->GetPostTanWidth();
}

bool TsKnot::SetPostTanSlope(const VtValue slope)
{
    if (!_CheckInParamVt(slope))
    {
        return false;
    }

    _proxy->SetPostTanSlope(slope);
    return true;
}

bool TsKnot::GetPostTanSlope(VtValue* const slopeOut) const
{
    if (!_CheckOutParamVt(slopeOut))
    {
        return false;
    }

    _proxy->GetPostTanSlope(slopeOut);
    return true;
}

bool TsKnot::SetPostTanAlgorithm(const TsTangentAlgorithm algorithm)
{
    _data->postTanAlgorithm = algorithm;
    return true;
}

TsTangentAlgorithm TsKnot::GetPostTanAlgorithm() const
{
    return _data->postTanAlgorithm;
}

////////////////////////////////////////////////////////////////////////////////
// Algorithmic Tangents

bool TsKnot::UpdateTangents(
    const std::optional<TsKnot> prevKnot,
    const std::optional<TsKnot> nextKnot,
    const TsCurveType curveType /* = TsCurveTypeBezier */)
{
    // Verify knot value types match.
    if (prevKnot &&
        !TF_VERIFY(prevKnot->GetValueType() == GetValueType(),
                   "Invalid prevKnot value type: `%s` instead of `%s`",
                   prevKnot->GetValueType().GetTypeName().c_str(),
                   GetValueType().GetTypeName().c_str()))
    {
        return false;
    }
        
    if (nextKnot &&
        !TF_VERIFY(nextKnot->GetValueType() == GetValueType(),
                   "Invalid nextKnot value type: `%s` instead of `%s`",
                   nextKnot->GetValueType().GetTypeName().c_str(),
                   GetValueType().GetTypeName().c_str()))
    {
        return false;
    }

    Ts_KnotDataProxy* prevProxy = (prevKnot ? prevKnot->_proxy.get() : nullptr);
    Ts_KnotDataProxy* nextProxy = (nextKnot ? nextKnot->_proxy.get() : nullptr);
    return _proxy->UpdateTangents(prevProxy, nextProxy, curveType);
}
    
////////////////////////////////////////////////////////////////////////////////
// Custom Data

bool TsKnot::SetCustomData(const VtDictionary customData)
{
    _customData = customData;
    return true;
}

VtDictionary TsKnot::GetCustomData() const
{
    return _customData;
}

bool TsKnot::SetCustomDataByKey(
    const std::string &keyPath,
    const VtValue value)
{
    _customData.SetValueAtPath(keyPath, value);
    return true;
}

VtValue TsKnot::GetCustomDataByKey(
    const std::string &keyPath) const
{
    return *(_customData.GetValueAtPath(keyPath));
}

////////////////////////////////////////////////////////////////////////////////
// Helpers

bool TsKnot::_CheckSetWidth(const TsTime width) const
{
    if (width < 0)
    {
        TF_CODING_ERROR("Cannot set negative tangent width");
        return false;
    }

    if (!Ts_IsFinite(width))
    {
        TF_CODING_ERROR("Tangent width values must be finite");
        return false;
    }

    return true;
}

namespace
{
    template <typename T>
    struct _ValueChecker
    {
        void operator()(const VtValue value, bool* const ok)
        {
            *ok = true;

            if (!Ts_IsFinite(value.UncheckedGet<T>()))
            {
                TF_CODING_ERROR("Cannot set undefined value");
                *ok = false;
            }
        }
    };
}

bool TsKnot::_CheckInParamVt(const VtValue value) const
{
    if (value.GetType() != GetValueType())
    {
        TF_CODING_ERROR(
            "Cannot set '%s' VtValue into knot of type '%s'",
            value.GetType().GetTypeName().c_str(),
            GetValueType().GetTypeName().c_str());
        return false;
    }

    bool ok = false;
    TsDispatchToValueTypeTemplate<_ValueChecker>(
        value.GetType(), value, &ok);
    if (!ok)
    {
        return false;
    }

    return true;
}

bool TsKnot::_CheckOutParamVt(VtValue* const valueOut) const
{
    if (!valueOut)
    {
        TF_CODING_ERROR("Null pointer");
        return false;
    }

    return true;
}

#define GET_VT_VALUE(method) [&](){ VtValue v; knot.method(&v); return v; }()

std::ostream& operator<<(std::ostream& out, const TsKnot &knot)
{
    out << "Knot:" << std::endl
        << "  value type " << knot.GetValueType().GetTypeName() << std::endl
        << "  time " << TfStringify(knot.GetTime()) << std::endl
        << "  value " << GET_VT_VALUE(GetValue) << std::endl
        << "  next interp "
        << TfEnum::GetName(knot.GetNextInterpolation()).substr(8) << std::endl;

    if (knot.IsDualValued())
    {
        out << "  preValue " << GET_VT_VALUE(GetPreValue) << std::endl;
    }

    out << "  pre-tan width "
        << TfStringify(knot.GetPreTanWidth()) << std::endl
        << "  pre-tan slope "
        << GET_VT_VALUE(GetPreTanSlope) << std::endl
        << "  post-tan width "
        << TfStringify(knot.GetPostTanWidth()) << std::endl
        << "  post-tan slope "
        << GET_VT_VALUE(GetPostTanSlope) << std::endl;

    const VtDictionary customData = knot.GetCustomData();
    if (!customData.empty())
    {
        out << "  custom data " << customData << std::endl;
    }

    return out;
}


PXR_NAMESPACE_CLOSE_SCOPE
