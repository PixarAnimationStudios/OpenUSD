//
// Copyright 2019 Google LLC
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_PLUGIN_USD_DRACO_FLAG_H
#define PXR_USD_PLUGIN_USD_DRACO_FLAG_H


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdDracoFlag
///
/// Stores command-line flag value and whether it has been explicitly specified
/// by the user.
///
template <class T>
class UsdDracoFlag {
public:
    UsdDracoFlag() : _specified(false), _value() {}
    UsdDracoFlag(const T &value) : _specified(true), _value(value) {}
    bool HasValue() const { return _specified; }
    const T &GetValue() const { return _value; }
    static UsdDracoFlag<bool> MakeBooleanFlag(int value) {
        // The value can be 0 (false), 1 (true), and -1 (unspecified).
        if (value == 0 || value == 1)
            return UsdDracoFlag<bool>(value);
        return UsdDracoFlag<bool>();
    }

private:
    const bool _specified;
    const T _value;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_PLUGIN_USD_DRACO_FLAG_H
