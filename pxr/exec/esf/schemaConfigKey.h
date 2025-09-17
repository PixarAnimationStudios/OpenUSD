//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_SCHEMA_CONFIG_KEY_H
#define PXR_EXEC_ESF_SCHEMA_CONFIG_KEY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// An opaque type that can be used to identify the configuration of typed
/// and applied schemas for a prim.
///
class EsfSchemaConfigKey {
  public:
    /// Only null keys can be constructed publicly.
    constexpr EsfSchemaConfigKey() = default;

    constexpr bool operator==(EsfSchemaConfigKey key) const {
        return _key == key._key;
    }

    constexpr bool operator!=(EsfSchemaConfigKey key) const {
        return !(*this == key);
    }

    template <class HashState>
    friend void TfHashAppend(HashState &h, const EsfSchemaConfigKey key) {
        h.Append(key._key);
    }

  private:
    // Derived classes can construct an EsfSchemaConfigKey by calling
    // EsfPrimInterface::CreateEsfSchemaConfigKey.
    //
    friend class EsfObjectInterface;
    constexpr explicit EsfSchemaConfigKey(const void *const key)
        : _key(key)
    {
    }

  private:
    const void *_key = nullptr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
