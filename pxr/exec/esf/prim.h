//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_PRIM_H
#define PXR_EXEC_ESF_PRIM_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/api.h"
#include "pxr/exec/esf/fixedSizePolymorphicHolder.h"
#include "pxr/exec/esf/object.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

class EsfJournal;
class EsfPrim;
class EsfAttribute;

/// Prim abstraction for scene adapter implementations.
///
/// The prim abstraction closely resembles the read-only interface of UsdPrim. 
///
/// The public methods of this class are called by the exec network compiler.
/// Each method takes an argument of type EsfJournal* which captures the
/// conditions for recompilation.
///
class ESF_API_TYPE EsfPrimInterface : public EsfObjectInterface
{
public:
    ESF_API ~EsfPrimInterface() override;

    /// \see UsdPrim::GetAppliedSchemas
    ESF_API const TfTokenVector &GetAppliedSchemas(EsfJournal *journal) const;

    /// \see UsdPrim::GetAttribute
    ESF_API EsfAttribute GetAttribute(
        const TfToken &attributeName,
        EsfJournal *journal) const;

    /// \see UsdPrim::GetRelationship
    ESF_API EsfRelationship GetRelationship(
        const TfToken &relationshipName,
        EsfJournal *journal) const;

    /// \see UsdPrim::GetParent
    ESF_API EsfPrim GetParent(EsfJournal *journal) const;

    /// \see UsdPrim::GetPrimTypeInfo and \see UsdPrimTypeInfo::GetSchemaType
    ESF_API TfType GetType(EsfJournal *journal) const;

    /// \see UsdPrim::IsPseudoRoot
    virtual bool IsPseudoRoot() const = 0;

protected:
    /// This constructor may only be called by the scene adapter implementation.
    EsfPrimInterface(const SdfPath &path) : EsfObjectInterface(path) {}

private:
    // These methods must be implemented by the scene adapter implementation.
    virtual const TfTokenVector &_GetAppliedSchemas() const = 0;
    virtual EsfAttribute _GetAttribute(
        const TfToken &attributeName) const = 0;
    virtual EsfPrim _GetParent() const = 0;
    virtual EsfRelationship _GetRelationship(
        const TfToken &relationshipName) const = 0;
    virtual TfType _GetType() const = 0;
};

/// Holds an implementation of EsfPrimInterface in a fixed-size buffer.
///
/// The buffer is large enough to fit an implementation that wraps a UsdPrim.
/// The size is specified as an integer literal to prevent introducing Usd as
/// a dependency.
///
class EsfPrim : public EsfFixedSizePolymorphicHolder<EsfPrimInterface, 48>
{
public:
    using EsfFixedSizePolymorphicHolder::EsfFixedSizePolymorphicHolder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
