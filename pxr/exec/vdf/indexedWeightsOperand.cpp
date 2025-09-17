//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/indexedWeightsOperand.h"

#include "pxr/base/gf/math.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

#include <algorithm>
#include <limits>
#include <cmath>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

static const float _mathError = std::numeric_limits<float>::quiet_NaN();

TF_REGISTRY_FUNCTION(TfType)
{
    // Register VdfIndexedWeightsOperand so it can be used in libExpr.
    TfType::Define<VdfIndexedWeightsOperand>();
}

VdfIndexedWeightsOperand::VdfIndexedWeightsOperand(
    SetOperation setOperation,
    const VdfIndexedWeights *externalWeights)
    : _setOperation(setOperation),
      _externalWeights(externalWeights),
      _mayHaveMathErrors(false)
{
}

void
VdfIndexedWeightsOperand::Swap(VdfIndexedWeights *v)
{
    TF_AXIOM(v);

    // In case we have external weights, first copy them.
    _CopyExternalWeights();

    _GetWriteIndices().swap(VdfIndexedWeights::_GetWriteIndices(v));
    _GetWriteData().swap(VdfIndexedWeights::_GetWriteData(v));
}

void
VdfIndexedWeightsOperand::PruneZeros(
    const std::vector<VdfIndexedWeightsOperand> &operands)
{
    // Make sure all set operations are the same.
    TF_FOR_ALL(it, operands) {
        TF_AXIOM(_setOperation == it->_setOperation);
    }

    // In case we have external weights, first copy them.
    _CopyExternalWeights();

    std::vector<int> &dstIndices = _GetWriteIndices();
    std::vector<float> &dstWeights = _GetWriteData();
    TF_AXIOM(dstIndices.size() == dstWeights.size());

    std::vector<const std::vector<int> *> operandIndices;
    std::vector<const std::vector<float> *> operandWeights;

    size_t numOperands = operands.size();
    operandIndices.reserve(numOperands);
    operandWeights.reserve(numOperands);

    TF_FOR_ALL(it, operands) {
        operandIndices.push_back(&it->_GetReadIndices());
        operandWeights.push_back(&it->_GetReadData());

        TF_AXIOM(operandIndices.back()->size() ==
                 operandWeights.back()->size());
    }

    size_t size = dstIndices.size();
    size_t numSkippedWeights = 0;

    std::vector<size_t> operandPos(numOperands, 0);

    // Iterate over all the indices.
    for (size_t i=0; i<size; ++i) {
        const int index = dstIndices[i];

        // Count the number of matching operands and how many of those that
        // have non-zero weights.
        size_t numMatchingOperands = 0;
        size_t numNonZeroOperands = 0;

        for (size_t n=0; n<numOperands; ++n) {
            size_t &j = operandPos[n];
            const std::vector<int> &indices = *operandIndices[n];
            const std::vector<float> &weights = *operandWeights[n];

            while (j < indices.size()) {
                if (indices[j] >= index) {
                    if (indices[j] == index) {
                        // XXX:optimization We could do early outs here
                        //                  (depending on the set operation).
                        numMatchingOperands++;
                        numNonZeroOperands += (weights[j] != 0.0f) ? 1 : 0;
                    }
                    break;
                }
                j++;
            }
        }

        // Whether or not this weight should be skipped depends on the set
        // operation.
        bool skipWeight =
            (_setOperation == Union && numNonZeroOperands == 0) ||
            (_setOperation == Intersection &&
                numNonZeroOperands < numMatchingOperands);

        if (skipWeight) {
            numSkippedWeights++;
        } else if (numSkippedWeights) {
            dstIndices[i - numSkippedWeights] = dstIndices[i];
            dstWeights[i - numSkippedWeights] = dstWeights[i];
        }
    }

    // Resize the index and weight vectors.
    dstIndices.resize(size - numSkippedWeights);
    dstWeights.resize(size - numSkippedWeights);
}

void
VdfIndexedWeightsOperand::Fill(
    const std::vector<VdfIndexedWeightsOperand> &operands,
    double fillWeight,
    bool nonZeroSetOperation)
{
    // Make sure all set operations are the same.
    TF_FOR_ALL(it, operands) {
        TF_AXIOM(_setOperation == it->_setOperation);
    }

    // Ignore external weights.
    _externalWeights = NULL;

    std::vector<int> &dstIndices = _GetWriteIndices();
    std::vector<float> &dstWeights = _GetWriteData();
    dstIndices.clear();
    dstWeights.clear();

    std::vector<const std::vector<int> *> operandIndices;
    std::vector<const std::vector<float> *> operandWeights;

    size_t numOperands = operands.size();
    operandIndices.reserve(numOperands);
    operandWeights.reserve(numOperands);

    TF_FOR_ALL(it, operands) {
        operandIndices.push_back(&it->_GetReadIndices());
        operandWeights.push_back(&it->_GetReadData());

        TF_AXIOM(operandIndices.back()->size() ==
                 operandWeights.back()->size());
    }

    std::vector<size_t> operandPos(numOperands, 0);

    // Iterate over all operand indices.
    while (true) {
        // Find the next smallest operand index.
        constexpr int maxIndex = std::numeric_limits<int>::max();
        int index = maxIndex;

        for (size_t n=0; n<numOperands; ++n) {
            const size_t &j = operandPos[n];
            const std::vector<int> &indices = *operandIndices[n];

            if (j < indices.size() && indices[j] < index) {
                index = indices[j];
            }
        }

        // If we did not find any smaller index, we are done.
        if (index == maxIndex) {
            break;
        }

        // Count the number of matching operands and how many of those that
        // have non-zero weights.
        size_t numMatchingOperands = 0;
        size_t numNonZeroOperands = 0;

        for (size_t n=0; n<numOperands; ++n) {
            size_t &j = operandPos[n];
            const std::vector<int> &indices = *operandIndices[n];
            const std::vector<float> &weights = *operandWeights[n];

            while (j < indices.size()) {
                if (indices[j] >= index) {
                    if (indices[j] == index) {
                        // XXX:optimization We could do early outs here
                        //                  (depending on the set operation).
                        numMatchingOperands++;
                        numNonZeroOperands += (weights[j] != 0.0f) ? 1 : 0;
                        j++;
                    }
                    break;
                }
                j++;
            }
        }

        bool addIndex;

        if (nonZeroSetOperation) {
            addIndex =
                (_setOperation == Union && numNonZeroOperands > 0) ||
                (_setOperation == Intersection &&
                    numNonZeroOperands == numMatchingOperands);
        } else {
            addIndex =
                _setOperation == Union ||
                numMatchingOperands == numOperands;
        }

        if (addIndex) {
            dstIndices.push_back(index);
            dstWeights.push_back(fillWeight);
        }
    }
}

static bool
_IsMathError(float val)
{
    return std::isnan(val) || std::isinf(val);
}

size_t
VdfIndexedWeightsOperand::GetNumMathErrors() const
{
    size_t numMathErrors = 0;

    if (_mayHaveMathErrors) {
        const std::vector<int> &indices = _GetReadIndices();
        const std::vector<float> &weights = _GetReadData();
        TF_AXIOM(indices.size() == weights.size());

        for (size_t i=0; i<indices.size(); ++i) {
            if (_IsMathError(weights[i])) {
                numMathErrors++;
            }
        }
    }

    return numMathErrors;
}

void
VdfIndexedWeightsOperand::ClearMathErrors()
{
    if (_mayHaveMathErrors) {
        const std::vector<int> &indices = _GetReadIndices();
        std::vector<float> &weights = _GetWriteData();
        TF_AXIOM(indices.size() == weights.size());

        for (size_t i=0; i<indices.size(); ++i) {
            if (_IsMathError(weights[i])) {
                weights[i] = 0.0f;
            }
        }
    }
}

void
VdfIndexedWeightsOperand::_CopyExternalWeights()
{
    if (_externalWeights) {
        _GetWriteIndices() =
            VdfIndexedWeights::_GetReadIndices(_externalWeights);
        _GetWriteData() =
            VdfIndexedWeights::_GetReadData(_externalWeights);
        _externalWeights = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Private mutating arithmetic-assignment operators

VdfIndexedWeightsOperand &
VdfIndexedWeightsOperand::operator+=(const VdfIndexedWeightsOperand &v)
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    std::vector<int> &dstIndices = _GetWriteIndices();
    std::vector<float> &dstWeights = _GetWriteData();

    // Temporary vectors used to swap out the destination indices and weights
    // (unless we have external weights).
    std::vector<int> tmpIndices;
    std::vector<float> tmpWeights;
    const std::vector<int> *pIndices0;
    const std::vector<float> *pWeights0;

    // Check if we have external weights.
    if (_externalWeights) {
        pIndices0 = &_GetReadIndices();
        pWeights0 = &_GetReadData();
        _externalWeights = NULL;
    } else {
        dstIndices.swap(tmpIndices);
        dstWeights.swap(tmpWeights);
        pIndices0 = &tmpIndices;
        pWeights0 = &tmpWeights;
    }

    const std::vector<int> &indices0 = *pIndices0;
    const std::vector<float> &weights0 = *pWeights0;
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(weights0[i]);
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(weights1[j]);
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(weights0[i] + weights1[j]);
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(weights1[j]);
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    _mayHaveMathErrors |= v._mayHaveMathErrors;

    return *this;
}

VdfIndexedWeightsOperand &
VdfIndexedWeightsOperand::operator-=(const VdfIndexedWeightsOperand &v)
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    std::vector<int> &dstIndices = _GetWriteIndices();
    std::vector<float> &dstWeights = _GetWriteData();

    // Temporary vectors used to swap out the destination indices and weights
    // (unless we have external weights).
    std::vector<int> tmpIndices;
    std::vector<float> tmpWeights;
    const std::vector<int> *pIndices0;
    const std::vector<float> *pWeights0;

    // Check if we have external weights.
    if (_externalWeights) {
        pIndices0 = &_GetReadIndices();
        pWeights0 = &_GetReadData();
        _externalWeights = NULL;
    } else {
        dstIndices.swap(tmpIndices);
        dstWeights.swap(tmpWeights);
        pIndices0 = &tmpIndices;
        pWeights0 = &tmpWeights;
    }

    const std::vector<int> &indices0 = *pIndices0;
    const std::vector<float> &weights0 = *pWeights0;
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(weights0[i]);
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(-weights1[j]);
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(weights0[i] - weights1[j]);
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(-weights1[j]);
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    _mayHaveMathErrors |= v._mayHaveMathErrors;

    return *this;
}

VdfIndexedWeightsOperand &
VdfIndexedWeightsOperand::operator*=(const VdfIndexedWeightsOperand &v)
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    std::vector<int> &dstIndices = _GetWriteIndices();
    std::vector<float> &dstWeights = _GetWriteData();

    // Temporary vectors used to swap out the destination indices and weights
    // (unless we have external weights).
    std::vector<int> tmpIndices;
    std::vector<float> tmpWeights;
    const std::vector<int> *pIndices0;
    const std::vector<float> *pWeights0;

    // Check if we have external weights.
    if (_externalWeights) {
        pIndices0 = &_GetReadIndices();
        pWeights0 = &_GetReadData();
        _externalWeights = NULL;
    } else {
        dstIndices.swap(tmpIndices);
        dstWeights.swap(tmpWeights);
        pIndices0 = &tmpIndices;
        pWeights0 = &tmpWeights;
    }

    const std::vector<int> &indices0 = *pIndices0;
    const std::vector<float> &weights0 = *pWeights0;
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(0.0f);
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(0.0f);
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(weights0[i] * weights1[j]);
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(0.0f);
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    _mayHaveMathErrors |= v._mayHaveMathErrors;

    return *this;
}

VdfIndexedWeightsOperand &
VdfIndexedWeightsOperand::operator/=(const VdfIndexedWeightsOperand &v)
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    std::vector<int> &dstIndices = _GetWriteIndices();
    std::vector<float> &dstWeights = _GetWriteData();

    // Temporary vectors used to swap out the destination indices and weights
    // (unless we have external weights).
    std::vector<int> tmpIndices;
    std::vector<float> tmpWeights;
    const std::vector<int> *pIndices0;
    const std::vector<float> *pWeights0;

    // Check if we have external weights.
    if (_externalWeights) {
        pIndices0 = &_GetReadIndices();
        pWeights0 = &_GetReadData();
        _externalWeights = NULL;
    } else {
        dstIndices.swap(tmpIndices);
        dstWeights.swap(tmpWeights);
        pIndices0 = &tmpIndices;
        pWeights0 = &tmpWeights;
    }

    const std::vector<int> &indices0 = *pIndices0;
    const std::vector<float> &weights0 = *pWeights0;
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(_mathError);
                    _mayHaveMathErrors = true;
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(0.0f);
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                if (weights1[j] != 0.0f) {
                    dstWeights.push_back(weights0[i] / weights1[j]);
                } else {
                    dstWeights.push_back(_mathError);
                    _mayHaveMathErrors = true;
                }
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(0.0f);
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    _mayHaveMathErrors |= v._mayHaveMathErrors;

    return *this;
}

VdfIndexedWeightsOperand &
VdfIndexedWeightsOperand::operator+=(double s)
{
    if (s != 0.0) {
        // Check if we have external weights.
        _CopyExternalWeights();

        size_t size = _GetWriteIndices().size();
        std::vector<float> &dstWeights = _GetWriteData();
        TF_AXIOM(size == dstWeights.size());

        float a = s;
        for (size_t i=0; i<size; ++i) {
            dstWeights[i] += a;
        }
    }

    return *this;
}

VdfIndexedWeightsOperand &
VdfIndexedWeightsOperand::operator-=(double s)
{
    return operator+= (-s);
}

VdfIndexedWeightsOperand &
VdfIndexedWeightsOperand::operator*=(double s)
{
    if (s != 1.0) {
        // Check if we have external weights.
        _CopyExternalWeights();

        size_t size = _GetWriteIndices().size();
        std::vector<float> &dstWeights = _GetWriteData();
        TF_AXIOM(size == dstWeights.size());

        float a = s;
        for (size_t i=0; i<size; ++i) {
            dstWeights[i] *= a;
        }
    }

    return *this;
}

VdfIndexedWeightsOperand &
VdfIndexedWeightsOperand::operator/=(double s)
{
    // Check if we have external weights.
    _CopyExternalWeights();

    size_t size = _GetWriteIndices().size();
    std::vector<float> &dstWeights = _GetWriteData();
    TF_AXIOM(size == dstWeights.size());

    if (s != 0.0) {
        float a = 1.0/s;
        for (size_t i=0; i<size; ++i) {
            dstWeights[i] *= a;
        }

    } else {
        for (size_t i=0; i<size; ++i) {
            dstWeights[i] = _mathError;
        }
        _mayHaveMathErrors = true;
    }

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// Comparison operators

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator<(const VdfIndexedWeightsOperand &v) const
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices0 = _GetReadIndices();
    const std::vector<float> &weights0 = _GetReadData();
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(weights0[i] < 0.0f ? 1.0f : 0.0f);
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(0.0f < weights1[j] ? 1.0f : 0.0f);
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(weights0[i] < weights1[j] ? 1.0f : 0.0f);
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(0.0f < weights1[j] ? 1.0f : 0.0f);
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;

    return w;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator<=(const VdfIndexedWeightsOperand &v) const
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices0 = _GetReadIndices();
    const std::vector<float> &weights0 = _GetReadData();
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(weights0[i] <= 0.0f ? 1.0f : 0.0f);
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(0.0f <= weights1[j] ? 1.0f : 0.0f);
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(weights0[i] <= weights1[j] ? 1.0f : 0.0f);
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(0.0f <= weights1[j] ? 1.0f : 0.0f);
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;

    return w;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator>(const VdfIndexedWeightsOperand &v) const
{
    return v < *this;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator>=(const VdfIndexedWeightsOperand &v) const
{
    return v <= *this;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator==(const VdfIndexedWeightsOperand &v) const
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices0 = _GetReadIndices();
    const std::vector<float> &weights0 = _GetReadData();
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(weights0[i] == 0.0f ? 1.0f : 0.0f);
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(0.0f == weights1[j] ? 1.0f : 0.0f);
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(weights0[i] == weights1[j] ? 1.0f : 0.0f);
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(0.0f == weights1[j] ? 1.0f : 0.0f);
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;

    return w;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator!=(const VdfIndexedWeightsOperand &v) const
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices0 = _GetReadIndices();
    const std::vector<float> &weights0 = _GetReadData();
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(weights0[i] != 0.0f ? 1.0f : 0.0f);
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(0.0f != weights1[j] ? 1.0f : 0.0f);
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(weights0[i] != weights1[j] ? 1.0f : 0.0f);
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(0.0f != weights1[j] ? 1.0f : 0.0f);
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;

    return w;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator<(double x) const
{
    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices = _GetReadIndices();
    const std::vector<float> &weights = _GetReadData();
    TF_AXIOM(indices.size() == weights.size());

    size_t size = indices.size();

    dstIndices.reserve(size);
    dstWeights.reserve(size);

    float a = x;
    for (size_t i=0; i<size; ++i) {
        dstIndices.push_back(indices[i]);
        dstWeights.push_back(weights[i] < a ? 1.0f : 0.0f);
    }

    return w;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator<=(double x) const
{
    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices = _GetReadIndices();
    const std::vector<float> &weights = _GetReadData();
    TF_AXIOM(indices.size() == weights.size());

    size_t size = indices.size();

    dstIndices.reserve(size);
    dstWeights.reserve(size);

    float a = x;
    for (size_t i=0; i<size; ++i) {
        dstIndices.push_back(indices[i]);
        dstWeights.push_back(weights[i] <= a ? 1.0f : 0.0f);
    }

    return w;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator>(double x) const
{
    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices = _GetReadIndices();
    const std::vector<float> &weights = _GetReadData();
    TF_AXIOM(indices.size() == weights.size());

    size_t size = indices.size();

    dstIndices.reserve(size);
    dstWeights.reserve(size);

    float a = x;
    for (size_t i=0; i<size; ++i) {
        dstIndices.push_back(indices[i]);
        dstWeights.push_back(weights[i] > a ? 1.0f : 0.0f);
    }

    return w;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator>=(double x) const
{
    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices = _GetReadIndices();
    const std::vector<float> &weights = _GetReadData();
    TF_AXIOM(indices.size() == weights.size());

    size_t size = indices.size();

    dstIndices.reserve(size);
    dstWeights.reserve(size);

    float a = x;
    for (size_t i=0; i<size; ++i) {
        dstIndices.push_back(indices[i]);
        dstWeights.push_back(weights[i] >= a ? 1.0f : 0.0f);
    }

    return w;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator==(double x) const
{
    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices = _GetReadIndices();
    const std::vector<float> &weights = _GetReadData();
    TF_AXIOM(indices.size() == weights.size());

    size_t size = indices.size();

    dstIndices.reserve(size);
    dstWeights.reserve(size);

    float a = x;
    for (size_t i=0; i<size; ++i) {
        dstIndices.push_back(indices[i]);
        dstWeights.push_back(weights[i] == a ? 1.0f : 0.0f);
    }

    return w;
}

VdfIndexedWeightsOperand 
VdfIndexedWeightsOperand::operator!=(double x) const
{
    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices = _GetReadIndices();
    const std::vector<float> &weights = _GetReadData();
    TF_AXIOM(indices.size() == weights.size());

    size_t size = indices.size();

    dstIndices.reserve(size);
    dstWeights.reserve(size);

    float a = x;
    for (size_t i=0; i<size; ++i) {
        dstIndices.push_back(indices[i]);
        dstWeights.push_back(weights[i] != a ? 1.0f : 0.0f);
    }

    return w;
}

////////////////////////////////////////////////////////////////////////////////
// Math library functions

template <bool CheckForMathErrors, typename ModifyFn>
VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::_ApplyFunctionToCopy(
    ModifyFn modify) const
{
    // Copy the weights operand and set it up to be mutated.
    VdfIndexedWeightsOperand copy(*this);
    copy._CopyExternalWeights();

    // Apply the function to all of the weights in the copy.
    std::vector<float>& copyWeights = copy._GetWriteData();
    size_t size = copyWeights.size();
    for (size_t i = 0; i < size; ++i) {
        copyWeights[i] = modify(copyWeights[i]);

        // Check the result if requested and if there isn't already an error
        // registered (optimization).
        if (CheckForMathErrors && !copy._mayHaveMathErrors) {
            copy._mayHaveMathErrors = _IsMathError(copyWeights[i]);
        }
    }

    return copy;
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::acos() const
{
    // Possible error: if weight is not in [-1.0, 1.0]
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        static_cast<float(*)(float)>(&std::acos));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::acosh() const
{
    // Possible error: if weight is not in [1.0, inf]
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        static_cast<float(*)(float)>(&std::acosh));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::asin() const
{
    // Possible error: if weight is not in [-1.0, 1.0]
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        static_cast<float(*)(float)>(&std::asin));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::asinh() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::asinh));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::atan() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::atan));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::atanh() const
{
    // Possible error: if weight is not in [-1.0, 1.0]
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        static_cast<float(*)(float)>(&std::atanh));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::atan2(const VdfIndexedWeightsOperand &v) const
{
    // create a bool to test if you are doing a union
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices0 = _GetReadIndices();
    const std::vector<float> &weights0 = _GetReadData();
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(std::atan2(weights0[i], 0.0f));
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(std::atan2(0.0f, weights1[j]));
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(std::atan2(weights0[i], weights1[j]));
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(std::atan2(0.0f, weights1[j]));
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;

    return w;
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::ceil() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::ceil));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::cos() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::cos));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::cosh() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::cosh));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::exp() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::exp));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::fabs() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::fabs));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::floor() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::floor));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::fmod(float denominator) const
{
    // Possible error: if denominator == 0.0
    // Note: we don't early-terminate on that condition here, as we have to
    // return a VdfIndexedWeightsOperand filled with NaNs at the correct
    // indices anyway.
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        [denominator](float x) { return std::fmod(x, denominator); });
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::log() const
{
    // Possible error: if weight <= 0.0
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        static_cast<float(*)(float)>(&std::log));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::log10() const
{
    // Possible error: if weight <= 0.0
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        static_cast<float(*)(float)>(&std::log10));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::pow(float exponent) const
{
    // Possible error: if weight < 0.0 and exponent is non-integer
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        [exponent](float x) { return std::pow(x, exponent); });
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::sin() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::sin));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::sinh() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::sinh));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::sqrt() const
{
    // Possible error: if weight < 0.0
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        static_cast<float(*)(float)>(&std::sqrt));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::tan() const
{
    // Possible error: if weight == pi/2 + n*pi
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ true>(
        static_cast<float(*)(float)>(&std::tan));
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::tanh() const
{
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        static_cast<float(*)(float)>(&std::tanh));
}

////////////////////////////////////////////////////////////////////////////////
// "Range" functions
// mhessler - As The Man Up Top says, there's a bunch of common code in the
//            operators (now copied by me for these min/max funcs) that would
//            be nice to factor out into a common implementation.  I'm a bad
//            person for contributing to the problem rather than the solution.

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::min(const VdfIndexedWeightsOperand &v) const
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices0 = _GetReadIndices();
    const std::vector<float> &weights0 = _GetReadData();
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(std::min(weights0[i], 0.0f));
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(std::min(0.0f, weights1[j]));
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(std::min(weights0[i], weights1[j]));
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(std::min(0.0f, weights1[j]));
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;

    return w;
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::max(const VdfIndexedWeightsOperand &v) const
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices0 = _GetReadIndices();
    const std::vector<float> &weights0 = _GetReadData();
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(std::max(weights0[i], 0.0f));
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(std::max(0.0f, weights1[j]));
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(std::max(weights0[i], weights1[j]));
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(std::max(0.0f, weights1[j]));
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;

    return w;
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::min(float min) const
{
    // As the compare value is a scalar and not a set of indexed weights to
    // iterate through, we can use the simple-math-function helper.
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        [min](float x) { return std::min(x, min); });
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::max(float max) const
{
    // As the compare value is a scalar and not a set of indexed weights to
    // iterate through, we can use the simple-math-function helper.
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        [max](float x) { return std::max(x, max); });
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::clamp(float min, float max) const
{
    // As the clamp values are scalars and not sets of indexed weights to
    // iterate through, we can use the simple-math-function helper.
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        [min, max](float x) { return GfClamp(x, min, max); });
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::smoothstep(
    float min, float max, float slope0, float slope1) const
{
    // As the smoothstep values are scalars and not sets of indexed weights to
    // iterate through, we can use the simple-math-function helper.
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        [min, max, slope0, slope1](float x) {
            return GfSmoothStep(min, max, x, slope0, slope1);
        });
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::smoothramp(
    float min, float max, float shoulder0, float shoulder1) const
{
    // As the smoothramp values are scalars and not sets of indexed weights to
    // iterate through, we can use the simple-math-function helper.
    return _ApplyFunctionToCopy</*CheckForMathErrors = */ false>(
        [min, max, shoulder0, shoulder1](float x) {
            return GfSmoothRamp(min, max, x, shoulder0, shoulder1);
        });
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::lerp(const VdfIndexedWeightsOperand &v, 
    float a) const
{
    TF_AXIOM(_setOperation == v._setOperation);
    bool computeUnion = _setOperation == Union;

    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices0 = _GetReadIndices();
    const std::vector<float> &weights0 = _GetReadData();
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;

        if (read0) {
            if (!read1 || indices0[i] < indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(GfLerp(a, weights0[i], 0.0f));
                }
                i++;
            } else if (indices0[i] > indices1[j]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(GfLerp(a, 0.0f, weights1[j]));
                }
                j++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(GfLerp(a, weights0[i], weights1[j]));
                i++;
                j++;
            }
        } else if (read1) {
            if (computeUnion) {
                dstIndices.push_back(indices1[j]);
                dstWeights.push_back(GfLerp(a, 0.0f, weights1[j]));
            }
            j++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;

    return w;
}

VdfIndexedWeightsOperand
VdfIndexedWeightsOperand::lerp(const VdfIndexedWeightsOperand &v, 
    const VdfIndexedWeightsOperand &a) const
{
    TF_AXIOM(_setOperation == v._setOperation);
    TF_AXIOM(_setOperation == a._setOperation);
    bool computeUnion = _setOperation == Union;

    VdfIndexedWeightsOperand w(_setOperation);

    std::vector<int> &dstIndices = w._GetWriteIndices();
    std::vector<float> &dstWeights = w._GetWriteData();

    const std::vector<int> &indices0 = _GetReadIndices();
    const std::vector<float> &weights0 = _GetReadData();
    TF_AXIOM(indices0.size() == weights0.size());

    const std::vector<int> &indices1 = v._GetReadIndices();
    const std::vector<float> &weights1 = v._GetReadData();
    TF_AXIOM(indices1.size() == weights1.size());

    const std::vector<int> &indices2 = a._GetReadIndices();
    const std::vector<float> &weights2 = a._GetReadData();
    TF_AXIOM(indices2.size() == weights2.size());

    size_t size0 = indices0.size();
    size_t size1 = indices1.size();
    size_t size2 = indices2.size();

    // We could maybe reserve more memory here (for unions).
    dstIndices.reserve(size0);
    dstWeights.reserve(size0);

    for (size_t i=0, j=0, k=0; ; ) {
        bool read0 = i < size0;
        bool read1 = j < size1;
        bool read2 = k < size2;

        if (read0) {
            if ((!read1 || indices0[i] < indices1[j]) && 
                (!read2 || indices0[i] < indices2[k])) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(weights0[i]);
                }
                i++;
            } else if ((read1 && indices0[i] > indices1[j]) && 
                (!read2 || indices1[j] < indices2[k])) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(0.0f);
                }
                j++;
            } else if ((read2 && indices0[i] > indices2[k]) && 
                (!read1 || indices1[j] > indices2[k])) {
                if (computeUnion) {
                    dstIndices.push_back(indices2[k]);
                    dstWeights.push_back(0.0f);
                }
                k++;
            } else if ((read1 && indices0[i] == indices1[j]) && 
                (!read2 || indices0[i] < indices2[k])) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(weights0[i]);
                }
                i++;
                j++;
            } else if ((read2 && indices0[i] == indices2[k]) && 
                (!read1 || indices0[i] < indices1[j])) {
                if (computeUnion) {
                    dstIndices.push_back(indices0[i]);
                    dstWeights.push_back(GfLerp(weights2[k], 
                        weights0[i], 0.0f));
                }
                i++;
                k++;
            } else if ((indices1[j] == indices2[k]) && 
                (indices0[i] > indices1[j])) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(GfLerp(weights2[k], 
                        0.0f, weights1[j]));
                }
                j++;
                k++;
            } else {
                dstIndices.push_back(indices0[i]);
                dstWeights.push_back(GfLerp(weights2[k], 
                        weights0[i], weights1[j]));
                i++;
                j++;
                k++;
            }
        } else if (read1) {
            if (!read2 || indices1[j] < indices2[k]) {
                if (computeUnion) {
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(0.0f);
                }
                j++;
            } else if (indices1[j] > indices2[k]) {
                if (computeUnion) {
                    dstIndices.push_back(indices2[k]);
                    dstWeights.push_back(0.0f);
                }
                k++;
            } else {
                if (computeUnion) {                
                    dstIndices.push_back(indices1[j]);
                    dstWeights.push_back(GfLerp(weights2[k], 
                        0.0f, weights1[j]));
                }
                j++;
                k++;
            }
        } else if (read2) {
            if (computeUnion) {
                dstIndices.push_back(indices2[k]);
                dstWeights.push_back(0.0f);
            }
            k++;
        } else {
            break;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;
    w._mayHaveMathErrors |= a._mayHaveMathErrors;

    return w;
}

////////////////////////////////////////////////////////////////////////////////
// Free function operator overloads

VdfIndexedWeightsOperand
operator+(double s, const VdfIndexedWeightsOperand &v)
{
    return v + s;
}

VdfIndexedWeightsOperand
operator-(double s, const VdfIndexedWeightsOperand &v)
{
    // Directly subtract v from s.  This is an optimization over the
    // form: -v + s, which used operator overloading and ended up
    // calling duplicate functions and iterating over the weights
    // twice.
    VdfIndexedWeightsOperand w(v);   

    // Check if we have external weights.
    w._CopyExternalWeights();
    
    size_t size = w._GetWriteIndices().size();
    std::vector<float> &dstWeights = w._GetWriteData();
    if (size != dstWeights.size()) {
        TF_WARN("Write index size (%d) does not match write data size (%d). "
                "Using the data size.", (int)size, (int)dstWeights.size() );
        size = dstWeights.size();
    }

    // Perform the subtraction
    float a = s;
    for (size_t i=0; i<size; ++i) {
        dstWeights[i] = a - dstWeights[i];
    }

    return w;
}

VdfIndexedWeightsOperand
operator*(double s, const VdfIndexedWeightsOperand &v)
{
    return v * s;
}

VdfIndexedWeightsOperand
operator/(double s, const VdfIndexedWeightsOperand &v)
{
    VdfIndexedWeightsOperand w(v._setOperation);

    w._GetWriteIndices() = v._GetReadIndices();
    w._GetWriteData() = v._GetReadData();

    size_t size = w._GetWriteIndices().size();
    std::vector<float> &dstWeights = w._GetWriteData();
    TF_AXIOM(size == dstWeights.size());

    float a = s;
    for (size_t i=0; i<size; ++i) {
        if (dstWeights[i] != 0.0f) {
            dstWeights[i] = a / dstWeights[i];
        } else {
            dstWeights[i] = _mathError;
            w._mayHaveMathErrors = true;
        }
    }

    // Propagate math errors.
    w._mayHaveMathErrors |= v._mayHaveMathErrors;

    return w;
}

VdfIndexedWeightsOperand
operator<(double s, const VdfIndexedWeightsOperand &v)
{
    return v > s;
}

VdfIndexedWeightsOperand
operator<=(double s, const VdfIndexedWeightsOperand &v)
{
    return v >= s;
}

VdfIndexedWeightsOperand
operator>(double s, const VdfIndexedWeightsOperand &v)
{
    return v < s;
}

VdfIndexedWeightsOperand
operator>=(double s, const VdfIndexedWeightsOperand &v)
{
    return v <= s;
}

VdfIndexedWeightsOperand
operator==(double s, const VdfIndexedWeightsOperand &v)
{
    return v == s;
}

VdfIndexedWeightsOperand
operator!=(double s, const VdfIndexedWeightsOperand &v)
{
    return v != s;
}

PXR_NAMESPACE_CLOSE_SCOPE
