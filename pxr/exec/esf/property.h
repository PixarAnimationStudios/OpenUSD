//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_PROPERTY_H
#define PXR_EXEC_ESF_PROPERTY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/api.h"
#include "pxr/exec/esf/fixedSizePolymorphicHolder.h"
#include "pxr/exec/esf/object.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

class EsfJournal;

/// Property abstraction for scene adapter implementations.
///
/// The property abstraction closely resembles the read-only interface of
/// UsdProperty. 
///
/// The public methods of this class are called by the exec network compiler.
/// Each method takes an argument of type EsfJournal* which captures the
/// conditions for recompilation.
///
class ESF_API_TYPE EsfPropertyInterface : public EsfObjectInterface
{
public:
    ESF_API ~EsfPropertyInterface() override;

    /// \see UsdProperty::GetBaseName
    ESF_API TfToken GetBaseName(EsfJournal *journal) const;

    /// \see UsdProperty::GetNamespace
    ESF_API TfToken GetNamespace(EsfJournal *journal) const;

protected:
    /// This constructor may only be called by the scene adapter implementation.
    EsfPropertyInterface(const SdfPath &path) : EsfObjectInterface(path) {}

private:
    // These methods must be implemented by the scene adapter implementation.
    virtual TfToken _GetBaseName() const = 0;
    virtual TfToken _GetNamespace() const = 0;
};

/// Holds an implementation of EsfPropertyInterface in a fixed-size buffer.
///
/// The buffer is large enough to fit an implementation that wraps a
/// UsdProperty. The size is specified as an integer literal to prevent
/// introducing Usd as a dependency.
///
class EsfProperty
    : public EsfFixedSizePolymorphicHolder<EsfPropertyInterface, 48>
{
public:
    using EsfFixedSizePolymorphicHolder::EsfFixedSizePolymorphicHolder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
