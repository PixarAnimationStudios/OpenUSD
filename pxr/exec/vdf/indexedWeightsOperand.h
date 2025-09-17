//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_INDEXED_WEIGHTS_OPERAND_H
#define PXR_EXEC_VDF_INDEXED_WEIGHTS_OPERAND_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/indexedWeights.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class VdfIndexedWeightsOperand
///
/// \brief Used to perform math operations on VdfIndexedWeights.
///
/// This class adds functionality for adding, multiplying, dividing, comparing,
/// etc. VdfIndexedWeights with other VdfIndexedWeights or scalars.  The
/// weights data can be externally referenced, in which case the lifetime of
/// the operand object must not exceed the lifetime of the referenced weights.
/// The reason for allowing externally referenced weights is to avoid
/// unnecessary copies of VdfIndexedWeights.  VdfIndexedWeightsOperand are
/// typically transient objects that only exist during a computation and
/// the result is stored as a VdfIndexedWeights.
///
/// XXX:exec
/// BE VERY CAREFUL NOT TO USE THESE AS YOU WOULD A VdfIndexedWeights.
/// The API here can be tightened to make it harder for client code to
/// misuse this class.  This class has very unusual read/write semantics.
///
class VdfIndexedWeightsOperand : public VdfIndexedWeights
{
    typedef VdfIndexedWeightsOperand This;

public:
    /// The set operation used by binary operations.
    ///
    /// When applying a binary operator to two indexed weights operands there
    /// are really two (independent) operations that get applied to compute
    /// the resulting indexed weights.  The first is the operation that is
    /// applied to the weights (this is typically an arithmetic or comparison
    /// operation), and then there is the set operation that gets applied to
    /// the operand index sets to determine which indices should be part of
    /// the result.
    ///
    /// For convenience the set operation is for now not specified when
    /// invoking an operator, but instead it is part of the operand.  This
    /// requires that all operands in an expression have the same set
    /// operations.  If we ever want to allow more general expressions we
    /// would need to come up with new operators, e.g. operator +& could
    /// mean addition of weight with index set union, and *| could mean
    /// multiplication of weights with index set intersection.
    ///
    enum SetOperation {
        Union,
        Intersection,
    };

    /// Creates an indexed weights operand with the given \p setOperation and
    /// optional external weights.
    ///
    /// Note that the indexed weights operand does not take over ownership
    /// of the external weights, see class documentation for details.
    ///
    VDF_API
    explicit VdfIndexedWeightsOperand(
        SetOperation setOperation,
        const VdfIndexedWeights *externalWeights = NULL);

    /// Swaps the indexed weights held by this opernad with the given
    /// indexed weights
    ///
    /// If this indexed weights operand has external weights these will get
    /// copied before the swap.
    ///
    VDF_API
    void Swap(VdfIndexedWeights *v);

    /// Prunes zeros according to the set operation and the indices in
    /// \p operands.
    ///
    /// All \p operands must have the same set operation as this operand.
    /// If the set operation is union, this removes all indices whose
    /// corresponding weights from \p operands are all zero.  If the set
    /// operation is intersection, this removes all indices that has a single
    /// corresponding weight from \p operands which is zero.  Note that this
    /// removes indices regardless of actual weight values in the operand
    /// itself.
    ///
    VDF_API
    void PruneZeros(const std::vector<This> &operands);

    /// Fills this operand with \p fillWeight according to the set operation
    /// and the indices in \p operands.
    ///
    /// All \p operands must have the same set operation as this operand, and
    /// note that the previous indices of this operand are discarded.  If the
    /// set operation is union, indices that have at least one corresponding
    /// index in \p operands are set (and if \p nonZeroSetOperation is true at
    /// least one of the corresponding weights must also be non-zero).  If the
    /// set operation is intersection, indices that are in all \p operands are
    /// set (and if \p nonZeroSetOperation is true all their weights must be
    /// non-zero).
    ///
    VDF_API
    void Fill(
        const std::vector<This> &operands,
        double fillWeight,
        bool nonZeroSetOperation);

    /// Returns the number of math errors (weights which are \c inf or \c NaN).
    ///
    VDF_API
    size_t GetNumMathErrors() const;

    /// Clears any pending math errors.
    ///
    /// Note that this sets all weights with math errors to 0.
    ///
    VDF_API
    void ClearMathErrors();

    /// Returns a new VdfIndexedWeightsOperand having the weights of this
    /// object negated.
    ///
    This operator-() const {
        This w(*this);
        return w *= -1.0;
    }

    /// Returns a new VdfIndexedWeightsOperand having the scalar \p s added to
    /// the weights of this object.
    ///
    This operator+(double s) const {
        This w(*this);
        return w += s;
    }

    /// Returns a new VdfIndexedWeightsOperand having the scalar \p s subtracted
    /// from the weights of this object.
    ///
    This operator-(double s) const {
        This w(*this);
        return w -= s;
    }

    /// Returns a new VdfIndexedWeightsOperand having the weights of this
    /// object multiplied by scalar \p s.
    ///
    This operator*(double s) const {
        This w(*this);
        return w *= s;
    }

    /// Returns a new VdfIndexedWeightsOperand having the weights of this
    /// object divided by scalar \p s.
    ///
    This operator/(double s) const {
        This w(*this);
        return w /= s;
    }

    /// Returns a new VdfIndexedWeightsOperand having the weights of
    /// VdfIndexedWeightsOperand \p v added to the weights of this object.
    ///
    This operator+(const This &v) const {
        This w(*this);
        return w += v;
    }

    /// Returns a new VdfIndexedWeightsOperand having the weights of
    /// VdfIndexedWeightsOperand \p v subtracted from the weights of this
    /// object.
    ///
    This operator-(const This &v) const {
        This w(*this);
        return w -= v;
    }

    /// Returns a new VdfIndexedWeightsOperand having the weights of this
    /// object multiplied by the weights of VdfIndexedWeightsOperand \p v.
    ///
    This operator*(const This &v) const {
        This w(*this);
        return w *= v;
    }

    /// Returns a new VdfIndexedWeightsOperand having the weights of this
    /// object divided by the weights of VdfIndexedWeightsOperand \p v.
    ///
    This operator/(const This &v) const {
        This w(*this);
        return w /= v;
    }

    /// Component-wise comparisons.
    ///
    /// Each of these functions returns a new VdfIndexedWeightsOperand in which
    /// the weight value at at each index is 1.0 if the comparison holds true
    /// for the corresponding weights in this and the compared object (or the
    /// weight in this object and the provided scalar value) and 0.0 if not.
    /// In effect, the returned object consists of the boolean result of the
    /// comparison at each indexed weight, cast to floating-point values.
    ///
    /// @{

    VDF_API This operator<(const This &v) const;
    VDF_API This operator<=(const This &v) const;
    VDF_API This operator>(const This &v) const;
    VDF_API This operator>=(const This &v) const;
    VDF_API This operator==(const This &v) const;
    VDF_API This operator!=(const This &v) const;
    VDF_API This operator<(double x) const;
    VDF_API This operator<=(double x) const;
    VDF_API This operator>(double x) const;
    VDF_API This operator>=(double x) const;
    VDF_API This operator==(double x) const;
    VDF_API This operator!=(double x) const;

    /// @}

    /// Standard math library functions.
    ///
    /// Each of these functions returns a new VdfIndexedWeightsOperand in which
    /// the specified math function is applied to the weights of this object.
    ///
    /// @{

    VDF_API This acos() const;
    VDF_API This acosh() const;
    VDF_API This asin() const;
    VDF_API This asinh() const;
    VDF_API This atan() const;
    VDF_API This atanh() const;
    VDF_API This atan2(const This &v) const;
    VDF_API This ceil() const;
    VDF_API This cos() const;
    VDF_API This cosh() const;
    VDF_API This exp() const;
    VDF_API This fabs() const;
    VDF_API This floor() const;
    VDF_API This fmod(float denominator) const;
    VDF_API This log() const;
    VDF_API This log10() const;
    VDF_API This pow(float exponent) const;
    VDF_API This sin() const;
    VDF_API This sinh() const;
    VDF_API This sqrt() const;
    VDF_API This tan() const;
    VDF_API This tanh() const;

    /// @}

    /// Range-of-weights methods.
    ///
    /// @{

    /// Returns a new VdfIndexedWeightsOperand where each indexed weight is the
    /// minimum of that weight in this object and the corresponding weight in
    /// VdfIndexedWeightsOperand \p v.
    ///
    VDF_API
    This min(const This &v) const;

    /// Returns a new VdfIndexedWeightsOperand where each indexed weight is the
    /// maximum of that weight in this object and the corresponding weight in
    /// VdfIndexedWeightsOperand \p v.
    ///
    VDF_API
    This max(const This &v) const;

    /// Returns a new VdfIndexedWeightsOperand where each indexed weight is the
    /// minimum of that weight in this object and the scalar \p min.
    ///
    VDF_API
    This min(float min) const;

    /// Returns a new VdfIndexedWeightsOperand where each indexed weight is the
    /// minimum of that weight in this object and the scalar \p min.
    ///
    VDF_API
    This max(float max) const;

    /// Returns a new VdfIndexedWeightsOperand where each indexed weight is
    /// clamped between the scalars \p min and \p max.
    ///
    VDF_API
    This clamp(float min, float max) const;

    /// Returns a new VdfIndexedWeightsOperand where each indexed weight is
    /// smoothsteped between the scalars \p min and \p max with slopes
    /// \p slope0 and \p slope1.
    ///
    VDF_API
    This smoothstep(float min, float max,
        float slope0=0, float slope1=0) const;

    /// Returns a new VdfIndexedWeightsOperand where each indexed weight is
    /// smoothramped between the scalars \p min and \p max with
    /// "shoulder lengths" \p shoulder0 and \p shoulder1.
    ///
    VDF_API
    This smoothramp(float min, float max,
        float shoulder0, float shoulder1) const;

    /// Returns a new VdfIndexedWeightsOperand where each indexed weight is
    /// lerped from itself and a corresponding weight in 
    /// VdfIndexedWeightsOperand \p v using scalar \p a.
    ///
    VDF_API
    This lerp(const This &v, float a) const;
    
    /// Returns a new VdfIndexedWeightsOperand where each indexed weight is
    /// lerped from itself and a corresponding weight in 
    /// VdfIndexedWeightsOperand \p v using VdfIndexedWeightsOperand \p a.
    ///
    VDF_API
    This lerp(const This &v, const This &a) const;

    /// @}

    /// \name Debugging API
    /// @{
    ///

    /// Returns whether or not this object references external weights.
    ///
    bool HasExternalWeights() const {
        return _externalWeights;
    }

    /// @}

private:
    // Operators ---------------------------------------------------------------

    // These mutating arithmetic-assignment operators are kept private and only
    // used by the public operators to generate a modified copied to return.

    // Adds the weights of v to our weights.
    VDF_API
    This &operator+=(const This &v);

    // Subtracts the weights of v from our weights.
    VDF_API
    This &operator-=(const This &v);

    // Scales our weights by the weights of v.
    VDF_API
    This &operator*=(const This &v);

    // Divides our weights by the weights of v.
    VDF_API
    This &operator/=(const This &v);

    // Adds a scalar to all our weights.
    VDF_API
    This &operator+=(double s);

    // Subtracts a scalar from all our weights.
    VDF_API
    This &operator-=(double s);

    // Scales all our weights by a scalar.
    VDF_API
    This &operator*=(double s);

    // Divides all our weights by a scalar.
    VDF_API
    This& operator/=(double s);

    // Math library implementation helpers -------------------------------------

    // Common implementation for math functions.  A new VdfIndexedWeightOperand
    // is returned having a copy of this object's weights, mutated by calling
    // \p modify on each weight.  If template parameter \p CheckForMathErrors
    // is \c true, the result of each call is checked for math errors (as
    // defined above) and the return object is flagged if present.  This check
    // is opt-in due to the potential cost; callers should take great care in
    // the state of this check based on the requirements of \c ModifyFn.
    //
    // \c ModifyFn should have the effective call signature of a unary function
    // of type \c float.
    template <bool CheckForMathErrors, typename ModifyFn>
    This _ApplyFunctionToCopy(ModifyFn modify) const;

    // Read/write helpers ------------------------------------------------------

    // Makes a local copy of the external weights (that can be modified).
    void _CopyExternalWeights();

    // Returns the non-const indices.
    std::vector<int> &_GetWriteIndices() {
        return VdfIndexedWeights::_GetWriteIndices();
    }

    // Returns the indices.
    const std::vector<int> &_GetReadIndices() const {
        return _externalWeights
            ? VdfIndexedWeights::_GetReadIndices(_externalWeights)
            : VdfIndexedWeights::_GetReadIndices();
    }

    // Returns the non-const data.
    std::vector<float> &_GetWriteData() {
        return VdfIndexedWeights::_GetWriteData();
    }

    // Returns the data.
    const std::vector<float> &_GetReadData() const {
        return _externalWeights
            ? VdfIndexedWeights::_GetReadData(_externalWeights)
            : VdfIndexedWeights::_GetReadData();
    }

    // Allow access to the free function overloads that can't be implemented
    // in terms of other public operators due to the write semantics of this
    // class or for performance reasons.
    friend VDF_API This operator-(double, const This&);
    friend VDF_API This operator/(double, const This&);

private:
    // The set operation to be used for binary operators.
    SetOperation _setOperation;

    // The external weights (NULL when we do not have external weights).
    // Note that it is fine to copy this pointer as part of copy construction/
    // assignment since that just means to share the same external weights.
    const VdfIndexedWeights *_externalWeights;

    // Flag indicating that there might be math errors (to avoid always having
    // to check all weights).
    bool _mayHaveMathErrors;
};

/// Declare the \c double arithmetic and comparison free function overloads.
//
VDF_API VdfIndexedWeightsOperand operator+(double, const VdfIndexedWeightsOperand&);
VDF_API VdfIndexedWeightsOperand operator-(double, const VdfIndexedWeightsOperand&);
VDF_API VdfIndexedWeightsOperand operator*(double, const VdfIndexedWeightsOperand&);
VDF_API VdfIndexedWeightsOperand operator/(double, const VdfIndexedWeightsOperand&);

VDF_API VdfIndexedWeightsOperand operator>(double, const VdfIndexedWeightsOperand&);
VDF_API VdfIndexedWeightsOperand operator<(double, const VdfIndexedWeightsOperand&);
VDF_API VdfIndexedWeightsOperand operator>=(double, const VdfIndexedWeightsOperand&);
VDF_API VdfIndexedWeightsOperand operator<=(double, const VdfIndexedWeightsOperand&);
VDF_API VdfIndexedWeightsOperand operator==(double, const VdfIndexedWeightsOperand&);
VDF_API VdfIndexedWeightsOperand operator!=(double, const VdfIndexedWeightsOperand&);

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // #ifndef PXR_EXEC_VDF_INDEXED_WEIGHTS_OPERAND_H
