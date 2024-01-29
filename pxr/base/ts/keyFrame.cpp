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

#include "pxr/pxr.h"
#include "pxr/base/ts/keyFrame.h"
#include "pxr/base/ts/data.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/typeRegistry.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/gf/range2d.h"

PXR_NAMESPACE_OPEN_SCOPE

using std::string;

// Tolerance for deciding whether tangent slopes are parallel.
static const double SLOPE_DIFF_THRESHOLD = 1e-4f;

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TsKeyFrame>();
}

TsKeyFrame::TsKeyFrame( const TsTime & time,
                            const VtValue & val,
                            TsKnotType knotType,
                            const VtValue & leftTangentSlope,
                            const VtValue & rightTangentSlope,
                            TsTime leftTangentLength,
                            TsTime rightTangentLength)
{
    TsTypeRegistry::GetInstance().InitializeDataHolder(&_holder,val);

    _Initialize(time, knotType, leftTangentSlope, rightTangentSlope,
        leftTangentLength, rightTangentLength);
}

TsKeyFrame::TsKeyFrame( const TsTime & time,
                            const VtValue & lhv,
                            const VtValue & rhv,
                            TsKnotType knotType,
                            const VtValue & leftTangentSlope,
                            const VtValue & rightTangentSlope,
                            TsTime leftTangentLength,
                            TsTime rightTangentLength)
{
    TsTypeRegistry::GetInstance().InitializeDataHolder(&_holder,rhv);

    SetIsDualValued( true );
    SetLeftValue( lhv );

    _Initialize(time, knotType, leftTangentSlope, rightTangentSlope,
        leftTangentLength, rightTangentLength);
}

void
TsKeyFrame::_Initialize(
    const TsTime & time,
    TsKnotType knotType,
    const VtValue & leftTangentSlope,
    const VtValue & rightTangentSlope,
    TsTime leftTangentLength,
    TsTime rightTangentLength)
{
    SetTime( time );

    _InitializeKnotType(knotType);

    if (SupportsTangents()) {
        if (!leftTangentSlope.IsEmpty())
            SetLeftTangentSlope( leftTangentSlope );
        if (!rightTangentSlope.IsEmpty())
            SetRightTangentSlope( rightTangentSlope );
    }

    _InitializeTangentLength(leftTangentLength,rightTangentLength);
}

void
TsKeyFrame::_InitializeKnotType(TsKnotType knotType)
{
    if (!IsInterpolatable() && knotType != TsKnotHeld) {
        knotType = TsKnotHeld;
    }
    else if (IsInterpolatable() && !SupportsTangents() &&
        knotType == TsKnotBezier) {
        knotType = TsKnotLinear;
    }

    SetKnotType( knotType );
}

void
TsKeyFrame::_InitializeTangentLength(TsTime left, TsTime right)
{
    if (SupportsTangents()) {
        SetLeftTangentLength(left);
        SetRightTangentLength(right);
        ResetTangentSymmetryBroken();
    }
}

TsKeyFrame::TsKeyFrame()
{
    _holder.New(TsTraits<double>::zero);

    SetKnotType( TsKnotLinear );
}

TsKeyFrame::TsKeyFrame( const TsKeyFrame & kf )
{
    kf._holder.Get()->CloneInto(&_holder);
}

TsKeyFrame::~TsKeyFrame()
{
    _holder.Destroy();
}

TsKeyFrame &
TsKeyFrame::operator=(const TsKeyFrame &rhs)
{
    if (this != &rhs) {
        _holder.Destroy();
        rhs._holder.Get()->CloneInto(&_holder);
    }
    return *this;
}

bool
TsKeyFrame::operator==(const TsKeyFrame &rhs) const
{
    return this == &rhs || (*_holder.Get() == *rhs._holder.Get());
}

bool
TsKeyFrame::operator!=(const TsKeyFrame &rhs) const
{
    return !(*this == rhs);
}

bool
TsKeyFrame::IsEquivalentAtSide(const TsKeyFrame &keyFrame, TsSide side) const
{
    if (GetKnotType() != keyFrame.GetKnotType() ||
        GetTime() != keyFrame.GetTime() ||
        HasTangents() != keyFrame.HasTangents()) {
        return false;
    }

    if (side == TsLeft) {
        if (HasTangents()) {
            if (GetLeftTangentLength() != keyFrame.GetLeftTangentLength() ||
                GetLeftTangentSlope() != keyFrame.GetLeftTangentSlope()) {
                return false;
            }
        }
        return GetLeftValue() == keyFrame.GetLeftValue(); 
    } else {
        if (HasTangents()) {
            if (GetRightTangentLength() != keyFrame.GetRightTangentLength() ||
                GetRightTangentSlope() != keyFrame.GetRightTangentSlope()) {
                return false;
            }
        }
        return GetValue() == keyFrame.GetValue();
    }
}

TsKnotType
TsKeyFrame::GetKnotType() const
{
    return _holder.Get()->GetKnotType();
}

void
TsKeyFrame::SetKnotType( TsKnotType newType )
{
    _holder.GetMutable()->SetKnotType( newType );
}

bool
TsKeyFrame::CanSetKnotType( TsKnotType newType,
                              std::string *reason ) const
{
    return _holder.Get()->CanSetKnotType( newType, reason );
}

VtValue
TsKeyFrame::GetValue() const
{
    return _holder.Get()->GetValue();
}

VtValue
TsKeyFrame::GetLeftValue() const
{
    return _holder.Get()->GetLeftValue();
}

void
TsKeyFrame::SetValue( VtValue val )
{
    _holder.GetMutable()->SetValue( val );
}

VtValue
TsKeyFrame::GetValue( TsSide side ) const
{
    return (side == TsLeft) ? GetLeftValue() : GetValue();
}

void
TsKeyFrame::SetValue( VtValue val, TsSide side )
{
    if (side == TsLeft) {
        SetLeftValue(val);
    } else {
        SetValue(val);
    }
}

VtValue
TsKeyFrame::GetValueDerivative() const
{
    return _holder.Get()->GetValueDerivative();
}

VtValue
TsKeyFrame::GetZero() const
{
    return _holder.Get()->GetZero();
}

void
TsKeyFrame::SetLeftValue( VtValue val )
{
    _holder.GetMutable()->SetLeftValue( val );
}

VtValue
TsKeyFrame::GetLeftValueDerivative() const
{
    return _holder.Get()->GetLeftValueDerivative();
}

bool
TsKeyFrame::GetIsDualValued() const
{
    return _holder.Get()->GetIsDualValued();
}

void
TsKeyFrame::SetIsDualValued( bool isDual )
{
    _holder.GetMutable()->SetIsDualValued(isDual);
}

bool
TsKeyFrame::IsInterpolatable() const
{
    return _holder.Get()->ValueCanBeInterpolated();
}

bool
TsKeyFrame::SupportsTangents() const
{
    return _holder.Get()->ValueTypeSupportsTangents();
}

bool
TsKeyFrame::HasTangents() const
{
    return _holder.Get()->HasTangents();
}

TsTime
TsKeyFrame::GetLeftTangentLength() const
{
    return _holder.Get()->GetLeftTangentLength();
}

VtValue
TsKeyFrame::GetLeftTangentSlope() const
{
    return _holder.Get()->GetLeftTangentSlope();
}

TsTime
TsKeyFrame::GetRightTangentLength() const
{
    return _holder.Get()->GetRightTangentLength();
}

VtValue
TsKeyFrame::GetRightTangentSlope() const
{
    return _holder.Get()->GetRightTangentSlope();
}

bool
TsKeyFrame::_ValidateTangentSetting() const
{
    if (!SupportsTangents()) {
        TF_CODING_ERROR("value type %s does not support tangents",
            GetValue().GetTypeName().c_str());
        return false;
    }

    return true;
}

void
TsKeyFrame::SetLeftTangentLength( TsTime newLen )
{
    if (!_ValidateTangentSetting())
        return;

    _holder.GetMutable()->SetLeftTangentLength( newLen );
}

void
TsKeyFrame::SetLeftTangentSlope( VtValue newSlope )
{
    if (!_ValidateTangentSetting())
        return;

    _holder.GetMutable()->SetLeftTangentSlope( newSlope );
}

void
TsKeyFrame::SetRightTangentLength( TsTime newLen )
{
    if (!_ValidateTangentSetting())
        return;

    _holder.GetMutable()->SetRightTangentLength( newLen );
}

void
TsKeyFrame::SetRightTangentSlope( VtValue newSlope)
{
    if (!_ValidateTangentSetting())
        return;

    _holder.GetMutable()->SetRightTangentSlope( newSlope );
}

bool
TsKeyFrame::GetTangentSymmetryBroken() const
{
    return _holder.Get()->GetTangentSymmetryBroken();
}

void
TsKeyFrame::SetTangentSymmetryBroken( bool broken )
{
    if (!_ValidateTangentSetting())
        return;

    _holder.GetMutable()->SetTangentSymmetryBroken( broken );
}

void
TsKeyFrame::ResetTangentSymmetryBroken()
{
    if (!_ValidateTangentSetting())
        return;

    _holder.GetMutable()->ResetTangentSymmetryBroken();
}

static std::string
_GetValue(const TsKeyFrame & val)
{
    if (val.GetIsDualValued()) {
        return TfStringify(val.GetLeftValue()) + " - " +
            TfStringify(val.GetValue());
    }

    return TfStringify(val.GetValue());
}

std::ostream& operator<<(std::ostream& out, const TsKeyFrame & val)
{
    if (val.SupportsTangents()) {
        return out << "Ts.KeyFrame("
                   << val.GetTime() << ", "
                   << _GetValue(val) << ", "
                   << val.GetKnotType() << ", "
                   << val.GetLeftTangentSlope() << ", "
                   << val.GetRightTangentSlope() << ", "
                   << val.GetLeftTangentLength() << ", "
                   << val.GetRightTangentLength() << ")";
    } else {
        return out << "Ts.KeyFrame("
            << val.GetTime() << ", "
            << _GetValue(val) << ", "
            << val.GetKnotType() << ")";
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
