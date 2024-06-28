//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_AR_PY_ASSET_H
#define PXR_USD_AR_PY_ASSET_H

/// \file ar/pyAsset.h
/// Structures for creating Python bindings for objects inheriting from
/// \c ArAsset.

#include "pxr/pxr.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/usd/ar/asset.h"

#include <boost/python/return_arg.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class Ar_PyAsset
/// Data structure for exposing ArAsset objects exposed via ArResolver APIs to
/// Python.
class Ar_PyAsset
{
public:
    /// Create a Python representation of the given \c ArAsset resource.
    /// \param asset \c ArAsset for which to create a Python representation.
    explicit Ar_PyAsset(std::shared_ptr<ArAsset> asset): _asset{std::move(asset)} {}

    /// Return a buffer with the contents of the asset, or raise a
    /// \c ValueError if the content could not be retrieved.
    /// \return A buffer with the contents of the asset.
    boost::python::object GetBuffer() const
    {
        // In case of invalid asset reference, raise an exception to the Python
        // caller that an operation was attempted on an object whose context is
        // not appropriately opened:
        if (!_asset) {
            TfPyThrowValueError("Failed to open asset");
        }

        // In case of invalid asset buffer, raise an exception to the Python
        // caller that the buffer held by the asset could not be retrieved.
        //
        /// \see ArAsset::GetBuffer()
        std::shared_ptr<const char> buffer = _asset->GetBuffer();
        if (!buffer) {
            TfPyThrowValueError("Failed to retrieve asset buffer");
        }

        // Create a Python bytes object from the buffer:
        return boost::python::object(
            boost::python::handle<>(
                PyBytes_FromStringAndSize(buffer.get(), _asset->GetSize())));
    }

    /// Return a flag indicating whether the asset is considered valid.
    /// \return \c true if the asset is considered valid, \c false otherwise.
    bool IsValid() const
    {
        // When performing this smoke test, ensure that the referenced ArAsset
        // is valid.
        //
        /// \see ArResolver::OpenAsset()
        return _asset != nullptr;
    }

    /// Enter the Python Context Manager for the representation of the
    /// \c ArAsset.
    /// \return A reference to the Python Context Manager for the
    /// representation of the \c ArAsset.
    void Enter()
    {
        if (!_asset) {
            TfPyThrowValueError("Failed to open asset");
        }
    }

    /// Exit the Python Context Manager for the representation of the
    /// \c ArAsset.
    bool Exit(
        boost::python::object& /* exc_type */,
        boost::python::object& /* exc_value */,
        boost::python::object& /* exc_tb */)
    {
        _asset.reset();
        // Re-raise exceptions:
        return false;
    }

private:
    std::shared_ptr<ArAsset> _asset;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_PY_ASSET_H
