//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_VALIDATION_ERROR_H
#define PXR_USD_VALIDATION_USD_VALIDATION_ERROR_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdValidation/usdValidation/api.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;
class UsdValidationValidator;

/// \class UsdValidationErrorType
///
/// UsdValidationErrorType reflects severity of a validation error, which can
/// then be reported appropriately to the users.
///
/// None: No Error.
/// Error: Associates the UsdValidationErrorType with an actual Error reported
///        by the validation task.
/// Warn: Associates the UsdValidationErrorType with a less severe situation and
///       hence reported as warning by the validation task.
/// Info: Associates the UsdValidationErrorType with information which needs to
///       be reported to the users by the validation task.
enum class UsdValidationErrorType
{
    None = 0,
    Error,
    Warn,
    Info
};

/// \class UsdValidationErrorSite
///
/// UsdValidationErrorSite is important information available from a
/// ValidationError, which annotates the site where the Error was reported by a
/// validation task.
///
/// An Error could be reported in a SdfLayer (in layer metadata, for example),
/// or a UsdStage (in stage metadata, for example) or a Prim within a stage, or
/// a property of a prim.
struct UsdValidationErrorSite
{
public:
    UsdValidationErrorSite() = default;

    /// Initialize an UsdValidationErrorSite using a \p layer and an
    /// \p objectPath.
    ///
    /// Object Path here could be a prim or a property spec path.
    ///
    /// Note that to identify a layer metadata, objectPath can be set as the
    /// pseudoRoot.
    USDVALIDATION_API
    UsdValidationErrorSite(const SdfLayerHandle &layer,
                           const SdfPath &objectPath);

    /// Initialize an UsdValidationErrorSite using a \p usdStage and an \p
    /// objectPath.
    ///
    /// An option \p layer can also be provided to provide information about a
    /// specific layer the erroring \p objectPath is found in the property
    /// stack.
    ///
    /// Object Path here could be a prim path or a property path.
    /// Note that to identify stage's root layer metadata, objectPath can be set
    /// as the pseudoRoot.
    USDVALIDATION_API
    UsdValidationErrorSite(const UsdStagePtr &usdStage,
                           const SdfPath &objectPath,
                           const SdfLayerHandle &layer = SdfLayerHandle());

    /// Returns true if UsdValidationErrorSite instance can either point to a
    /// prim or property spec in a layer or a prim or property on a stage.
    bool IsValid() const
    {
        return IsValidSpecInLayer() || IsPrim() || IsProperty();
    }

    /// Returns true if the objectPath and layer represent a spec in the layer;
    /// false otherwise.
    bool IsValidSpecInLayer() const
    {
        if (!_layer || _objectPath.IsEmpty()) {
            return false;
        }
        return _layer->HasSpec(_objectPath);
    }

    /// Returns true if UsdValidationErrorSite represents a prim on a stage,
    /// false otherwise.
    bool IsPrim() const
    {
        return GetPrim().IsValid();
    }

    /// Returns true if UsdValidationErrorSite represents a property on a stage,
    /// false otherwise.
    bool IsProperty() const
    {
        return GetProperty().IsValid();
    }

    /// Returns the SdfPropertySpecHandle associated with this
    /// ValidationErrorSite's layer and objectPath.
    ///
    /// Returns an invalid SdfPropertySpecHandle if no valid property spec is
    /// found, or when UsdValidationErrorSite instance doesn't have a
    /// layer.
    const SdfPropertySpecHandle GetPropertySpec() const
    {
        if (!_layer) {
            return SdfPropertySpecHandle();
        }
        return _layer->GetPropertyAtPath(_objectPath);
    }

    /// Returns the SdfPrimSpecHandle associated with this ValidationErrorSite's
    /// layer and objectPath.
    ///
    /// Returns an invalid SdfPrimSpecHandle if no valid prim spec is found, or
    /// when UsdValidationErrorSite instance doesn't have a layer.
    const SdfPrimSpecHandle GetPrimSpec() const
    {
        if (!_layer) {
            return SdfPrimSpecHandle();
        }
        return _layer->GetPrimAtPath(_objectPath);
    }

    /// Returns the SdfLayerHandle associated with this
    /// UsdValidationValidatorErrorSite
    const SdfLayerHandle &GetLayer() const
    {
        return _layer;
    }

    /// Returns the UsdStage associated with this UsdValidationErrorSite;
    /// nullptr othewrise.
    const UsdStagePtr &GetStage() const
    {
        return _usdStage;
    }

    /// Returns UsdPrim associated with this UsdValidationErrorSite, that is
    /// when UsdStage is present and objectPath represents a prim path on this
    /// stage; if not, an invalid prim is returned.
    UsdPrim GetPrim() const
    {
        if (_usdStage) {
            return _usdStage->GetPrimAtPath(_objectPath);
        }
        return UsdPrim();
    }

    /// Returns UsdProperty associated with this UsdValidationErrorSite, that is
    /// when UsdStage is present and objectPath represents a property path on
    /// this stage; if not, an invalid property is returned.
    UsdProperty GetProperty() const
    {
        if (_usdStage) {
            return _usdStage->GetPropertyAtPath(_objectPath);
        }
        return UsdProperty();
    }

    /// Returns true if \p other UsdValidationErrorSite has same valued members
    /// as this UsdValidationErrorSite, false otherwise.
    bool operator==(const UsdValidationErrorSite &other) const
    {
        return (_layer == other._layer) && (_usdStage == other._usdStage)
            && (_objectPath == other._objectPath);
    }

    /// Returns false if \p other UsdValidationErrorSite has same valued members
    /// as this UsdValidationErrorSite, true otherwise.
    bool operator!=(const UsdValidationErrorSite &other) const
    {
        return !(*this == other);
    }

private:
    UsdStagePtr _usdStage;
    SdfLayerHandle _layer;
    SdfPath _objectPath;

}; // UsdValidationErrorSite

using UsdValidationErrorSites = std::vector<UsdValidationErrorSite>;

/// \class UsdValidationError
///
/// UsdValidationError is an entity returned by a validation task, which is
/// associated with a UsdValidationValidator.
///
/// A UsdValidationError instance contains important information, like:
///
/// - Name - A name the validator writer provided for the error. This is then
///        used to construct an identifier for the error.
///
/// - UsdValidationErrorType - severity of an error,
///
/// - UsdValidationErrorSites - on what sites validationError was reported by a
///                          validation task,
///
/// - Message - Message providing more information associated with the error.
///           Such a message is provided by the validator writer, when providing
///           implementation for the validation task function.
///
/// UsdValidationError instances will be stored in the UsdValidationContext
/// responsible for executing a set of UsdValidationValidators.
///
class UsdValidationError
{
public:
    /// A default constructed UsdValidationError signifies no error.
    USDVALIDATION_API
    UsdValidationError();

    /// Instantiate a ValidationError by providing its \p name, \p errorType,
    /// \p errorSites and an \p errorMsg.
    USDVALIDATION_API
    UsdValidationError(const TfToken &name,
                       const UsdValidationErrorType &errorType,
                       const UsdValidationErrorSites &errorSites,
                       const std::string &errorMsg);

    bool operator==(const UsdValidationError &other) const
    {
        return (_name == other._name) && (_errorType == other._errorType)
            && (_errorSites == other._errorSites)
            && (_errorMsg == other._errorMsg)
            && (_validator == other._validator);
    }

    bool operator!=(const UsdValidationError &other) const
    {
        return !(*this == other);
    }

    /// Returns the name token of the UsdValidationError
    const TfToken &GetName() const &
    {
        return _name;
    }

    /// Returns the name token of the UsdValidationError by-value
    TfToken GetName() &&
    {
        return std::move(_name);
    }

    /// Returns the UsdValidationErrorType associated with this
    /// UsdValidationError
    UsdValidationErrorType GetType() const
    {
        return _errorType;
    }

    /// Returns the UsdValidationErrorSite associated with this
    /// UsdValidationError
    const UsdValidationErrorSites &GetSites() const &
    {
        return _errorSites;
    }

    /// Returns the UsdValidationErrorSite associated with this
    /// UsdValidationError by-value
    UsdValidationErrorSites GetSites() &&
    {
        return std::move(_errorSites);
    }

    /// Returns the UsdValidationValidator that reported this error.
    ///
    /// This will return nullptr if there is no UsdValidationValidator
    /// associated with this error. This will never be nullptr for validation
    /// errors returned from calls to UsdValidationValidator::Validate.
    const UsdValidationValidator *GetValidator() const
    {
        return _validator;
    }

    /// Returns the message associated with this UsdValidationError
    const std::string &GetMessage() const
    {
        return _errorMsg;
    }

    /// An identifier for the error constructed from the validator name this
    /// error was generated from and its name.
    ///
    /// Since a validator may result in multiple distinct errors, the identifier
    /// helps in distinguishing and categorizing the errors. The identifier
    /// returned will be in the following form:
    /// For a plugin validator: "plugName":"validatorName"."ErrorName"
    /// For a non-plugin validator: "validatorName"."ErrorName"
    ///
    /// For an error that was generated without a name, the identifier will be
    /// same as the validator name which generated the error.
    ///
    /// For an error which is created directly and not via
    /// UsdValidationValidator::Validate() call, we throw a coding error, as its
    /// an improper use of the API.
    USDVALIDATION_API
    TfToken GetIdentifier() const;

    /// Returns UsdValidationErrorType and ErrorMessage concatenated as a string
    USDVALIDATION_API
    std::string GetErrorAsString() const;

    /// Returns true if UsdValidationErrorType is UsdValidationErrorType::None,
    /// false otherwise
    bool HasNoError() const
    {
        return _errorType == UsdValidationErrorType::None;
    }

private:
    // UsdValidationValidatorError holds a pointer to the UsdValidationValidator
    // that generated it, so we need to provide friend access to allow the
    // necessary mutation.
    friend class UsdValidationValidator;

    // Used by UsdValidationValidator::Validate methods to embed itself to the
    // reported errors.
    void _SetValidator(const UsdValidationValidator *validator);

    // _validator is set when ValidationError is generated via a
    // UsdValidationValidator::Validate() call.
    const UsdValidationValidator *_validator;

    // These data members should not be modified other than during
    // initialization by the validate task functions.
    TfToken _name;
    UsdValidationErrorType _errorType;
    UsdValidationErrorSites _errorSites;
    std::string _errorMsg;

    // TODO:(Subsequent iterations)
    // - VtValue of a random value the error wants to propagate down to the
    // fixer

}; // UsdValidationError

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_VALIDATION_USD_VALIDATION_ERROR_H
