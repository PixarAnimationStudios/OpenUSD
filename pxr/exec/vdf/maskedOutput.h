//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_MASKED_OUTPUT_H
#define PXR_EXEC_VDF_MASKED_OUTPUT_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/mask.h"

#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class VdfOutput;

/// \class VdfMaskedOutput
///
/// \brief Class to hold on to an externally owned output and a mask.
///
///        Objects of this class explicitly have two operators defined so that
///        they can act as VdfOutput.
///
class VdfMaskedOutput
{
public :

    VdfMaskedOutput() :
        _output(nullptr) {}

    VdfMaskedOutput(VdfOutput *output, const VdfMask &mask) :
        _output(output), _mask(mask) {}

    VdfMaskedOutput(VdfOutput *output, VdfMask &&mask) :
        _output(output), _mask(std::move(mask)) {}

    /// Cast to bool: returns true if the output is non-null.
    ///
    explicit operator bool() const {
        return static_cast<bool>(_output);
    }

    /// Returns the VdfOutput.
    ///
    VdfOutput *GetOutput() const {
        return _output;
    }

    /// Sets the output to \p output. 
    ///
    void SetOutput(VdfOutput *output) {
        _output = output;
    }

    /// Returns the VdfMask.
    ///
    const VdfMask &GetMask() const {
        return _mask;
    }

    /// Sets the mask to \p mask.
    ///
    void SetMask(const VdfMask &mask) {
        _mask = mask;
    }

    /// Sets the mask to \p mask.
    ///
    void SetMask(VdfMask &&mask) {
        _mask = std::move(mask);
    }

    /// Equality comparison.
    ///
    bool operator==(const VdfMaskedOutput &rhs) const {
        return _output == rhs._output &&
               _mask   == rhs._mask;
    }

    bool operator!=(const VdfMaskedOutput &rhs) const {
        return !(*this == rhs);
    }

    struct Hash
    {
        size_t operator()(const VdfMaskedOutput &maskedOutput) const
        {
            // Note: maskedOutput.GetMask() is a flyweight.
            return (size_t)maskedOutput.GetOutput() + 
                           maskedOutput.GetMask().GetHash();
        }
    };

    bool operator<(const VdfMaskedOutput &rhs) const {
        std::less<const VdfOutput *> outputLT;
        if (outputLT(_output, rhs._output))
            return true;

        if (outputLT(rhs._output, _output))
            return false;

        return VdfMask::ArbitraryLessThan()(_mask, rhs._mask);
    }

    bool operator<=(const VdfMaskedOutput &rhs) const {
        return !(rhs < *this);
    }

    bool operator>(const VdfMaskedOutput &rhs) const {
        return rhs < *this;
    }

    bool operator>=(const VdfMaskedOutput &rhs) const {
        return !(*this < rhs);
    }

    friend inline void swap(VdfMaskedOutput &lhs, VdfMaskedOutput &rhs) {
        using std::swap;
        swap(lhs._output, rhs._output);
        swap(lhs._mask, rhs._mask);
    }

    /// Returns a string describing this masked output.
    ///
    VDF_API
    std::string GetDebugName() const;

// -----------------------------------------------------------------------------

private :

    VdfOutput *_output;
    VdfMask    _mask;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
