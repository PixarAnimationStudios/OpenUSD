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
#ifndef USDGEOM_COLLECTION_API_H
#define USDGEOM_COLLECTION_API_H

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdLightSplineAPI
/// 
/// This is a general purpose API schema, used to describe a named
/// 1D spline stored as a set of knot attributes on a prim. It's
/// an add-on schema that can be applied many times to a prim with
/// different spline names. All the properties authored by the schema
/// are namespaced under "$NAME:spline:", with the given name of the
/// spline providing a namespace for the attributes.
/// 
class UsdLightSplineAPI: public UsdSchemaBase
{
public:
    /// Construct a UsdLightSplineAPI with the given \p name on 
    /// the UsdPrim \p prim .
    /// 
    explicit UsdLightSplineAPI(const UsdPrim& prim=UsdPrim(), 
                               const TfToken &name=TfToken())
        : UsdSchemaBase(prim),
          _name(name)
    {
    }

    /// Construct a UsdLightSplineAPI with the given \p name on the 
    /// prim held by \p schemaObj .
    /// 
    explicit UsdLightSplineAPI(const UsdSchemaBase& schemaObj, 
                               const TfToken &name)
        : UsdSchemaBase(schemaObj.GetPrim()),
          _name(name)
    {
    }

    /// Destructor
    USDGEOM_API
    virtual ~UsdLightSplineAPI();

public:

    /// \anchor UsdLightSplineAPI_PropertyValueAPI
    /// \name Spline Property Value Getters and Setters
    /// 
    /// Convenience API for getting and setting values of the various 
    /// collection properties.
    /// 
    /// @{
    
    /// Returns the name of the collection.
    /// 
    TfToken GetSplineName() const {
        return _name;
    }

    /// Returns true if the collection has no targets.
    /// 
    USDGEOM_API
    bool IsEmpty() const;

    /// Sets the paths to target objects that belong to the collection.
    /// 
    USDGEOM_API
    bool SetTargets(const SdfPathVector &targets) const;

    /// Returns the <b>unresolved paths</b> to target objects belonging to the 
    /// collection.
    /// 
    /// Since a collection can include a relationship, no relationship 
    /// forwarding is performed by the method. i.e., if the collection targets 
    /// a relationship, the target relationship is returned (and not the 
    /// ultimate targets of the target relationship).
    /// 
    /// By default, any relationship targets that point to a child prim or
    /// a property of a child prim beneath an instanceable prim will be
    /// forwarded to the corresponding object in the instance's master.
    /// This can be disabled by setting \p forwardToObjectsInMasters
    /// to false.
    ///
    /// \sa UsdRelationship::GetTargets
    /// 
    USDGEOM_API
    bool GetTargets(SdfPathVector *targets, 
                    bool forwardToObjectsInMasters=true) const;

    /// Sets the targetFaceCounts property of the collection at the given 
    /// \p time. Returns true if the value was authored successfully, false 
    /// otherwise.
    /// 
    /// If the collection restricts to a face-set for any of its targets, then
    /// "targetFaceCounts" specifies the number of faces included in the 
    /// various targets. The number of entries in \p targetFaceCounts should 
    /// always match the number of targets. If a target does not have 
    /// a face restriction, then it's count is set to 0, to indicate that 
    /// the entire prim is included in the collection. 
    ///
    /// \sa UsdLightSplineAPI::GetTargetFaceCounts()
    ///
    USDGEOM_API
    bool SetTargetFaceCounts(const VtIntArray &targetFaceCounts, 
                             const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// Retrieves the targetFaceCounts property value at the given \p time.
    /// Returns false if no value is authored or if the "targetFaceCounts" 
    /// property does not exist.
    /// 
    /// \sa UsdLightSplineAPI::SetTargetFaceCounts()
    /// \sa UsdLightSplineAPI::GetFaceIndices()
    ///
    USDGEOM_API
    bool GetTargetFaceCounts(VtIntArray *targetFaceCounts, 
                             const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// Sets the list of face indices belonging to the targets of the collection 
    /// that have a face-restriction. The total number of face indices should 
    /// be equal to the sum of all entires in the associated "targetFaceCounts" 
    /// value.
    /// 
    /// Returns true if the value was authored successfully, false 
    /// otherwise.
    /// 
    /// \sa UsdLightSplineAPI::GetTargetFaceIndices()
    /// \sa UsdLightSplineAPI::SetTargetFaceCounts()
    USDGEOM_API
    bool SetTargetFaceIndices(const VtIntArray &targetFaceIndices, 
                              const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// Retrieves the targetFaceCounts property value at the given \p time.
    /// Returns false if no value is authored or if the "targetFaceIndices" 
    /// property does not exist.
    /// 
    /// \sa UsdLightSplineAPI::SetTargetFaceIndices()
    /// 
    USDGEOM_API
    bool GetTargetFaceIndices(VtIntArray *targetFaceIndices, 
                              const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// Appends a new target, \p target to the collection. The list of 
    /// face indices in the array, \p faceIndices is used to specify a 
    /// face-restriction on the target at the given time, \p time. 
    /// 
    /// Returns true only upon success.
    /// 
    /// Here are a few things worth noting about this method:
    /// 
    /// \li The target face-count is gleaned from the length of the 
    /// \p faceIndices array.
    /// 
    /// \li If \p faceIndices is empty and there is an existing value for 
    /// "targetFaceCounts", then 0 is appended to the list of target face-counts
    /// to indicate that the entire target is included in the collection.
    /// 
    /// \li If \p faceIndices is empty and the collection does have not a value 
    /// for the "targetFaceCounts" property, then only the target is appended.
    /// targetFaceCounts and targetFaceIndices are not authored (or even created)
    /// in this case.
    /// 
    USDGEOM_API
    bool AppendTarget(const SdfPath &target, 
                      const VtIntArray &faceIndices=VtIntArray(),
                      const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// @}

    /// \anchor UsdLightSplineAPI_RawProperties
    /// \name Spline Property API
    /// 
    /// API for getting and creating the "raw" properties associated with a 
    /// collection. 
    /// 
    /// @{

    /// Returns the "targetFaceCounts" attribute associated with the collection.
    /// 
    /// \n  C++ Type: VtIntArray
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    ///
    /// \sa GetTargetFaceCounts()
    ///
    USDGEOM_API
    UsdAttribute GetTargetFaceCountsAttr() const;

    /// Creates the "targetFaceCounts" attribute associated with the collection.
    /// 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    ///
    /// \sa GetTargetFaceCountsAttr()
    ///
    USDGEOM_API
    UsdAttribute CreateTargetFaceCountsAttr(const VtValue &defaultValue=VtValue(),
                                            bool writeSparsely=false) const;

    /// Returns the "targetFaceIndices" attribute associated with the collection.
    /// 
    /// \n  C++ Type: VtIntArray
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// 
    /// \sa GetTargetFaceIndices()
    ///
    USDGEOM_API
    UsdAttribute GetTargetFaceIndicesAttr() const;

    /// Creates the "targetFaceIndices" attribute associated with the collection.
    ///
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    ///
    /// \sa GetFaceIndicesAttr()
    ///
    USDGEOM_API
    UsdAttribute CreateTargetFaceIndicesAttr(const VtValue &defaultValue=VtValue(),
                                             bool writeSparsely=false) const;

    /// Returns the relationship that targets the prims included in the 
    /// collection.
    /// 
    /// \sa GetTargets()
    /// 
    USDGEOM_API
    UsdRelationship GetTargetsRel() const;

    /// Creates the relationship that targets the prims included in the 
    /// collection.
    /// 
    /// \sa GetTargetsRel()
    /// 
    USDGEOM_API
    UsdRelationship CreateTargetsRel() const;

    /// @}

    /// \anchor UsdLightSplineAPI_StaticHelpers
    /// \name Static API
    /// 
    /// Convenience API for creating a collection on a prim and for retrieving 
    /// all the collections on a prim.
    /// 
    /// @{

    /// Creates a new collection on the given \p prim with the given \p name.
    /// 
    /// If \p targets, \p targetFaceCounts and \p targetFaceIndices are set 
    /// if specified. No validation is performed on the values passed in. 
    /// 
    /// If a collection already exists with the given name, it's targets 
    /// are reset to the specified set of targets, if \p targets is non-empty.
    /// 
    USDGEOM_API
    static UsdLightSplineAPI Create(
        const UsdPrim &prim, 
        const TfToken &name,
        const SdfPathVector &targets=SdfPathVector(),
        const VtIntArray &targetFaceCounts=VtIntArray(),
        const VtIntArray &targetFaceIndices=VtIntArray());

    /// \overload
    /// Creates a new collection with the given \p name on the prim held in 
    /// the given \p schemaObj.
    /// 
    /// If \p targets, \p targetFaceCounts and \p targetFaceIndices are set 
    /// if specified. No validation is performed on the values passed in. 
    /// 
    /// If a collection already exists with the given name, it's targets 
    /// are reset to the specified set of targets, if \p targets is non-empty.
    /// 
    USDGEOM_API
    static UsdLightSplineAPI Create(
        const UsdSchemaBase &schemaObj, 
        const TfToken &name,
        const SdfPathVector &targets=SdfPathVector(),
        const VtIntArray &targetFaceCounts=VtIntArray(),
        const VtIntArray &targetFaceIndices=VtIntArray());

    /// Returns the list of all collections on the given prim, \p prim.
    /// 
    /// This will return both empty and non-empty collections. 
    /// 
    USDGEOM_API
    static std::vector<UsdLightSplineAPI> GetSplines(const UsdPrim &prim);

    /// Returns the list of all face-sets on the prim held by \p schemaObj.
    USDGEOM_API
    static std::vector<UsdLightSplineAPI> GetSplines(
        const UsdSchemaBase &schemaObj);

    /// @}

    /// \anchor UsdLightSplineAPI_Validation
    /// \name Spline Validation API
    /// 
    /// API for validating the properties of a collection.
    /// 
    /// @{

    /// Validates the properties belonging to the collection. Returns true 
    /// if the collection has all valid properties. Returns false and 
    /// populates the \p reason output argument if the collection is invalid.
    /// 
    /// Here's the list of validations performed by this method:
    /// \li A collection is considered to be invalid if it has no data authored.
    /// i.e. when the collection relationship does not exist.
    /// \li The number of entries in "targetFaceCounts" should match the number 
    /// of targets in the collection over all timeSamples.
    /// \li The sum all values in the "targetFaceCounts" array should be equal 
    /// to the length of the "targetFaceIndices" array over all timeSamples.
    /// 
    USDGEOM_API
    bool Validate(std::string *reason) const;

    /// @}

private:
    
    // Returns the collection:<name relationship.
    UsdRelationship _GetTargetsRel(bool create=false) const;

    // Returns the collection:<name>:targetFaceCounts attribute.
    UsdAttribute _GetTargetFaceCountsAttr(bool create=false) const;

    // Returns the collection:<name>:targetFaceIndices attribute.
    UsdAttribute _GetTargetFaceIndicesAttr(bool create=false) const;

    // Returns the name of the property belonging to this collection, given the 
    // base name of the attribute. Eg, if baseName is 'targetFaceCounts', this 
    // returns 'collection:<name>:targetFaceCounts'.
    TfToken _GetSplinePropertyName(const TfToken &baseName=TfToken()) const;

    // The name of the collection.
    TfToken _name;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
