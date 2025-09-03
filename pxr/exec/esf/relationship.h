//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_RELATIONSHIP_H
#define PXR_EXEC_ESF_RELATIONSHIP_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/api.h"
#include "pxr/exec/esf/fixedSizePolymorphicHolder.h"
#include "pxr/exec/esf/property.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

class EsfJournal;

/// Relationship abstraction for scene adapter implementations.
///
/// The relationship abstraction closely resembles the read-only interface of
/// UsdRelationship. 
///
/// The public methods of this class are called by the exec network compiler.
/// Each method takes an argument of type EsfJournal* which captures the
/// conditions for recompilation.
///
class ESF_API_TYPE EsfRelationshipInterface : public EsfPropertyInterface
{
public:
    ESF_API ~EsfRelationshipInterface() override;

    /// \see UsdRelationship::GetTargets
    ESF_API SdfPathVector GetTargets(EsfJournal *journal) const;

    /// \see UsdRelationship::GetForwardedTargets
    ESF_API SdfPathVector GetForwardedTargets(EsfJournal *journal) const;

protected:
    /// This constructor may only be called by the scene adapter implementation.
    EsfRelationshipInterface(const SdfPath &path) : EsfPropertyInterface(path) {}

private:
    // These methods must be implemented by the scene adapter implementation.
    virtual SdfPathVector _GetTargets() const = 0;
};

/// Holds an implementation of EsfRelationshipInterface in a fixed-size buffer.
///
/// The buffer is large enough to fit an implementation that wraps a
/// UsdRelationship. The size is specified as an integer literal to prevent
/// introducing Usd as a dependency.
///
class EsfRelationship
    : public EsfFixedSizePolymorphicHolder<EsfRelationshipInterface, 48>
{
public:
    using EsfFixedSizePolymorphicHolder::EsfFixedSizePolymorphicHolder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
