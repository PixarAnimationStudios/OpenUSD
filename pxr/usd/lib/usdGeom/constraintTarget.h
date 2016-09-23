//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef USDGEOM_CONSTRAINT_TARGET_H
#define USDGEOM_CONSTRAINT_TARGET_H

#include "pxr/usd/usd/attribute.h"

#include <string>

class GfMatrix4d;
class UsdGeomXformCache;

/// \class UsdGeomConstraintTarget
///
/// Schema wrapper for UsdAttribute for authoring and introspecting
/// attributes that are constraint targets.
/// 
/// Constraint targets correspond roughly to what some DCC's call locators.
/// They are coordinate frames, represented as (animated or static) GfMatrix4d
/// values.  We represent them as attributes in USD rather than transformable
/// prims because generally we require no other coordinated information about 
/// a constraint target other than its name and its matrix value, and because
/// attributes are more concise than prims.
///
/// Because consumer clients often care only about the identity and value of
/// constraint targets and may be able to usefully consume them without caring
/// about the actual geometry with which they may logically correspond,
/// UsdGeom aggregates all constraint targets onto a model's root prim,
/// assuming that an exporter will use property namespacing within the 
/// constraint target attribute's name to indicate a path to a prim within 
/// the model with which the constraint target may correspond.
///
/// To facilitate instancing, and also position-tweaking of baked assets, we
/// stipulate that constraint target values always be recorded in
/// <b>model-relative transformation space</b>.  In other words, to get the
/// world-space value of a constraint target, transform it by the
/// local-to-world transformation of the prim on which it is recorded.  
/// ComputeInWorldSpace() will perform this calculation.
/// 
/// \todo Provide API for extracting prim or property path from a target's
/// namespaced name.
///
class UsdGeomConstraintTarget
{
    typedef const UsdAttribute UsdGeomConstraintTarget::*_UnspecifiedBoolType;

 public:
  
    // Default constructor returns an invalid ConstraintTarget.  Exists for 
    // container classes
    UsdGeomConstraintTarget()
    {
        /* NOTHING */
    }
    
    /// Speculative constructor that will produce a valid
    /// UsdGeomConstraintTarget when \p attr already represents an attribute
    /// that is a UsdGeomConstraintTarget, and produces an \em invalid
    /// UsdGeomConstraintTarget otherwise (i.e.  
    /// \ref UsdGeomConstraintTarget_bool_type "unspecified-bool-type()"
    /// will return false).
    ///
    /// Calling \c UsdGeomConstraintTarget::IsValid(attr) will return the
    /// same truth value as the object returned by this constructor, but if
    /// you plan to subsequently use the ConstraintTarget anyways, just construct
    /// the object and bool-evaluate it before proceeding.
    explicit UsdGeomConstraintTarget(const UsdAttribute &attr);

    /// Test whether a given UsdAttribute represents valid ConstraintTarget, which
    /// implies that creating a UsdGeomConstraintTarget from the attribute will succeed.
    ///
    /// Success implies that \c attr.IsDefined() is true.
    static bool IsValid(const UsdAttribute &attr);

    // ---------------------------------------------------------------
    /// \name UsdAttribute API
    // ---------------------------------------------------------------
    
    /// Allow UsdGeomConstraintTarget to auto-convert to UsdAttribute, so you can
    /// pass a UsdGeomConstraintTarget to any function that accepts a UsdAttribute or
    /// const-ref thereto.
    operator UsdAttribute const& () const { return _attr; }

    /// Explicit UsdAttribute extractor
    UsdAttribute const &GetAttr() const { return _attr; }
    
    /// Return true if the wrapped UsdAttribute::IsDefined(), and in
    /// addition the attribute is identified as a ConstraintTarget.
    bool IsDefined() const { return IsValid(_attr); }

    /// \anchor UsdGeomConstraintTarget_bool_type
    /// Return true if this ConstraintTarget is valid for querying and authoring
    /// values and metadata, which is identically equivalent to IsDefined().
#ifdef doxygen
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return IsDefined() ? &UsdGeomConstraintTarget::_attr : 0;
    }
#endif // doxygen

    /// Get the attribute value of the ConstraintTarget at \p time 
    bool Get(GfMatrix4d* value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Set the attribute value of the ConstraintTarget at \p time 
    bool Set(const GfMatrix4d& value, UsdTimeCode time = UsdTimeCode::Default()) const;

    /// Get the stored identifier unique to the enclosing model's namespace for
    /// this constraint target.
    /// \sa SetIdentifier()
    TfToken GetIdentifier() const;

    /// Explicitly sets the stored identifier to the given string. Clients are
    /// responsible for ensuring the uniqueness of this identifier within the
    /// enclosing model's namespace.
    void SetIdentifier(const TfToken &identifier);

    /// Returns the fully namespaced constraint attribute name, given the 
    /// constraint name.
    static TfToken GetConstraintAttrName(const std::string &constraintName);

    /// Computes the value of the constraint target in world space.
    /// 
    /// If a valid UsdGeomXformCache is provided in the argument \p xfCache, 
    /// it is used to evaluate the CTM of the model to which the constraint
    /// target belongs. 
    /// 
    /// To get the constraint value in model-space (or local space), simply 
    /// use UsdGeomConstraintTarget::Get(), since the authored values must 
    /// already be in model-space.
    /// 
    GfMatrix4d ComputeInWorldSpace(UsdTimeCode time=UsdTimeCode::Default(),
                                   UsdGeomXformCache *xfCache=NULL) const;

private:

    UsdAttribute _attr;
};

#endif // USD_CONSTRAINT_TARGET_H
