//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_VALIDATION_USD_VALIDATION_FIXER_H
#define PXR_USD_VALIDATION_USD_VALIDATION_FIXER_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usdValidation/usdValidation/api.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class UsdValidationError;
class UsdEditTarget;

using FixerImplFn = 
    std::function<bool(
        const UsdValidationError& e, const UsdEditTarget& editTarget,
        const UsdTimeCode& timeCode)>;

using FixerCanApplyFn = 
    std::function<bool(
        const UsdValidationError& e, const UsdEditTarget& editTarget,
        const UsdTimeCode& timeCode)>;

/// \class UsdValidationFixer
///
/// A UsdValidationFixer represents a fix that can be applied to fix a specific
/// validation error.
///
/// A fixer is associated with a specific validator, and can
/// be associated with a specific error name, or can be generic to any
/// error associated with the corresponding validator.
///
/// A fixer has a name, description, and a set of keywords associated with it.
/// The name must be unique among all fixers associated with a specific
/// validator. The keywords can be used to group fixers by department, show,
/// etc.
///
/// A fixer has two functions associated with it:
/// - The \p FixerImplFn is the function that will be called to apply the
/// fix for a given error.
/// - The \p FixerCanApplyFn is the function that will be called to determine
/// if the fixer can be applied to a given error.
///
/// Note that the Validation framework will not apply any fixers automatically.
/// It's the responsibility of the client to pick and apply a fix for a given
/// error, by specifically calling the ApplyFix() method on an appropriate
/// UsdEditTarget.
///
/// Appropriate fixers can be retrieved from UsdValidationValidator or 
/// UsdValidationError itself via the GetFixers(), GetFixersWithKeyword(),
/// GetFixerByName(), GetFixersByErrorName(), etc.
///
class UsdValidationFixer
{
public:
    UsdValidationFixer(const UsdValidationFixer&) = default;
    UsdValidationFixer& operator=(const UsdValidationFixer&) = default;

    UsdValidationFixer(UsdValidationFixer&&) noexcept = default;
    UsdValidationFixer& operator=(UsdValidationFixer&&) noexcept = default;
    
    /// Construct a UsdValidationFixer with the given \p name and \p description.
    ///
    /// The \p fixerImplFn is the function that will be called to apply the fix
    /// for a given error. The \p canApplyFn is the function that will be
    /// called to determine if the fixer can be applied to a given error.
    /// The \p errorName, if provided, is the error name that this fixer
    /// can fix. If not provided, the fixer can be applied to any error 
    /// associated with the corresponding validator. The \p keywords are the
    /// keywords associated with this fixer. Clients when instantiating a fixer
    /// for a validator, can provide keywords such as studio, department, or
    /// show. Clients can later see which keywords are associated with the fixer
    /// or use the GetFixersByKeywords method to return fixers by the keywords
    /// they are associated with.  
    USDVALIDATION_API
    UsdValidationFixer(
        const TfToken &name, const std::string &description, 
        const FixerImplFn &fixerImplFn, const FixerCanApplyFn &canApplyFn,
        const TfTokenVector &keywords = TfTokenVector(),
        const TfToken &errorName = TfToken());

    ~UsdValidationFixer() = default;

    /// Returns the name of this fixer.
    const TfToken &GetName() const &
    { 
        return _name; 
    }

    /// Return the name of this fixer by-value.
    TfToken GetName() &&
    {
        return std::move(_name);
    }

    /// Returns the description of this fixer.
    const std::string &GetDescription() const &
    { 
        return _description; 
    }

    /// Return the description of this fixer by-value.
    std::string GetDescription() &&
    {
        return std::move(_description);
    }

    /// Returns the error name that this fixer can fix, if any.
    const TfToken &GetErrorName() const &
    { 
        return _errorName; 
    }

    /// Returns the error name that this fixer can fix, if any, by-value.
    TfToken GetErrorName() && 
    { 
        return std::move(_errorName); 
    }

    /// Returns true if this fixer is associated with the given error name.
    USDVALIDATION_API
    bool IsAssociatedWithErrorName(const TfToken &errorName) const;

    /// Returns the keywords associated with this fixer.
    const TfTokenVector &GetKeywords() const &
    { 
        return _keywords; 
    }

    /// Return the keywords associated with this fixer by-value.
    TfTokenVector GetKeywords() && 
    { 
        return std::move(_keywords); 
    }

    /// Returns true if this fixer has the given keyword.
    USDVALIDATION_API
    bool HasKeyword(const TfToken &keyword) const;

    /// Returns true if this fixer can be applied to the given error and edit
    /// target; false otherwise.
    ///
    /// A fixer can be associated with a specific error name, if so, it can
    /// only be applied to errors with the same name, otherwise, it can be
    /// applied to any error generated by the corresponding validator.
    ///
    /// Additionally, the \p editTarget can be inspected to determine if the
    /// fix can be applied to the given target, etc.
    ///
    /// \sa ApplyFix
    USDVALIDATION_API
    bool CanApplyFix(
        const UsdValidationError &error, const UsdEditTarget &editTarget,
        const UsdTimeCode &timeCode = UsdTimeCode::Default()) const;

    /// Applies the fix for the given error at the given time code and edit
    /// target. Returns true if the fix was applied successfully.
    ///
    /// After the fix is applied, the edit target's layer is saved, and false
    /// is returned if saving fails. Note that in this case the fix has still
    /// been authored to the layer. Anonymous layers are not saved, since they
    /// have no backing asset; the fix remains an unsaved edit on the layer.
    ///
    /// \sa CanApplyFix
    USDVALIDATION_API
    bool ApplyFix(
        const UsdValidationError &error, const UsdEditTarget &editTarget, 
        const UsdTimeCode &timeCode = UsdTimeCode::Default()) const;

private:
    TfToken _name;
    std::string _description;
    FixerImplFn _fixerImplFn;
    FixerCanApplyFn _canApplyFn;
    TfTokenVector _keywords;
    TfToken _errorName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_VALIDATION_USD_VALIDATION_FIXER_H
