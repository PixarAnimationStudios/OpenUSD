//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_STAGE_H
#define PXR_EXEC_ESF_STAGE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/fixedSizePolymorphicHolder.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class EsfAttribute;
class EsfJournal;
class EsfObject;
class EsfPrim;
class EsfProperty;
class EsfRelationship;
class SdfPath;

/// Stage abstraction for scene adapter implementations.
///
/// The stage abstraction closely resembles the read-only interface of UsdStage.
///
/// The public methods of this class are called by the exec network compiler.
/// Each method takes an argument of type EsfJournal* which captures the
/// conditions for recompilation.
///
/// This class is compatible with EsfFixedSizePolymorphicHolder.
///
class ESF_API_TYPE EsfStageInterface : public EsfFixedSizePolymorphicBase
{
public:
    ESF_API ~EsfStageInterface() override;

    /// \see UsdStage::GetAttributeAtPath
    ESF_API EsfAttribute GetAttributeAtPath(
        const SdfPath &path,
        EsfJournal *journal) const;

    /// \see UsdStage::GetObjectAtPath
    ESF_API EsfObject GetObjectAtPath(
        const SdfPath &path,
        EsfJournal *journal) const;

    /// \see UsdStage::GetPrimAtPath
    ESF_API EsfPrim GetPrimAtPath(
        const SdfPath &path,
        EsfJournal *journal) const;

    /// \see UsdStage::GetPropertyAtPath
    ESF_API EsfProperty GetPropertyAtPath(
        const SdfPath &path,
        EsfJournal *journal) const;

    /// \see UsdStage::GetRelationshipAtPath
    ESF_API EsfRelationship GetRelationshipAtPath(
        const SdfPath &path,
        EsfJournal *journal) const;

    /// \see UsdSchemaRegistry::GetTypeNameAndInstance
    std::pair<TfToken, TfToken> GetTypeNameAndInstance(
        const TfToken &apiSchemaName) const {
        return _GetTypeNameAndInstance(apiSchemaName);
    }

    /// \see UsdSchemaRegistry::GetAPITypeFromSchemaTypeName
    TfType GetAPITypeFromSchemaTypeName(
        const TfToken &schemaTypeName) const {
        return _GetAPITypeFromSchemaTypeName(schemaTypeName);
    }

private:
    // These methods must be implemented by the scene adapter implementation.
    virtual EsfAttribute _GetAttributeAtPath(
        const SdfPath &path) const = 0;
    virtual EsfObject _GetObjectAtPath(
        const SdfPath &path) const = 0;
    virtual EsfPrim _GetPrimAtPath(
        const SdfPath &path) const = 0;
    virtual EsfProperty _GetPropertyAtPath(
        const SdfPath &path) const = 0;
    virtual EsfRelationship _GetRelationshipAtPath(
        const SdfPath &path) const = 0;
    virtual std::pair<TfToken, TfToken> _GetTypeNameAndInstance(
        const TfToken &apiSchemaName) const = 0;
    virtual TfType _GetAPITypeFromSchemaTypeName(
        const TfToken &schemaTypeName) const = 0;
};

/// Holds an implementation of EsfStageInterface in a fixed-size buffer.
///
/// The buffer is large enough to fit an implementation that wraps a
/// UsdStageConstRefPtr. The size is specified as an integer literal to prevent
/// introducing Usd as a dependency.
///
class EsfStage : public EsfFixedSizePolymorphicHolder<EsfStageInterface, 16>
{
public:
    using EsfFixedSizePolymorphicHolder::EsfFixedSizePolymorphicHolder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
