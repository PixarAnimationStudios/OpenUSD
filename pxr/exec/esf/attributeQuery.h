//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_ATTRIBUTE_QUERY_H
#define PXR_EXEC_ESF_ATTRIBUTE_QUERY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/api.h"
#include "pxr/exec/esf/fixedSizePolymorphicHolder.h"

#include "pxr/base/ts/spline.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

/// Attribute query abstraction for scene adapter implementations.
///
/// This abstraction closely resembles the UsdAttributeQuery.
///
/// The public methods of this class are called during initialization of inputs
/// to the execution network and during authored value invalidation.
///
class ESF_API_TYPE EsfAttributeQueryInterface :
    public EsfFixedSizePolymorphicBase
{
public:
    ESF_API ~EsfAttributeQueryInterface() override;

    /// Returns `true` if the query object is valid.
    ///
    /// \see UsdAttributeQuery::IsValid
    ///
    bool IsValid() const {
        return _IsValid();
    }

    /// Returns the path of the attribute that is being queried.
    SdfPath GetPath() const {
        return _GetPath();
    }

    /// Reinitialize the query object from the attribute it was initially
    /// constructed with.
    /// 
    /// This enables clients to "revive" the query object after changes that
    /// affect value resolution previously invalidated it.
    /// 
    void Initialize() {
        _Initialize();
    }

    /// Gets the resolved value of the attribute at a given time.
    ///
    /// \see UsdAttribute::Get
    ///
    bool Get(VtValue *value, UsdTimeCode time) const {
        return _Get(value, time);
    }

    /// Gets the authored spline if the strongest opinion is a spline.
    ///
    /// \see UsdAttribute::GetSpline
    ///
    std::optional<TsSpline> GetSpline() const {
        return _GetSpline();
    }

    /// Returns `true` if the attribute value might be varying over time, and
    /// `false` if the value is *definitely* not varying over time.
    /// 
    /// \see UsdAttribute::ValueMightBeTimeVarying
    /// 
    bool ValueMightBeTimeVarying() const {
        return _ValueMightBeTimeVarying();
    }

    /// Returns `true` if the resolved value of the attribute is different on
    /// time \p from and time \p to.
    /// 
    /// \note
    /// This does *not* examine times between \p from and \p to in order to
    /// determine if there is a difference in resolved values on in-between
    /// times.
    ///
    bool IsTimeVarying(UsdTimeCode from, UsdTimeCode to) const {
        return _IsTimeVarying(from, to);
    }

protected:
    /// This constructor may only be called by the scene adapter implementation.
    EsfAttributeQueryInterface() = default;

private:
    // These methods must be implemented by the scene adapter implementation.
    virtual bool _IsValid() const = 0;
    virtual SdfPath _GetPath() const = 0;
    virtual void _Initialize() = 0;
    virtual bool _Get(VtValue *value, UsdTimeCode time) const = 0;
    virtual std::optional<TsSpline> _GetSpline() const = 0;
    virtual bool _ValueMightBeTimeVarying() const = 0;
    virtual bool _IsTimeVarying(UsdTimeCode from, UsdTimeCode to) const = 0;
};

/// Holds an implementation of EsfAttributeQueryInterface in a fixed-size
/// buffer.
///
/// The buffer is large enough to fit an implementation that wraps a
/// UsdAttributeQuery. The size is specified as an integer literal to prevent
/// introducing Usd as a dependency.
///
class EsfAttributeQuery
    : public EsfFixedSizePolymorphicHolder<EsfAttributeQueryInterface, 160>
{
public:
    using EsfFixedSizePolymorphicHolder::EsfFixedSizePolymorphicHolder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
