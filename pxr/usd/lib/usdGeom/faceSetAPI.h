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
#ifndef USDGEOM_FACE_SET_API_H
#define USDGEOM_FACE_SET_API_H

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <string>

// -------------------------------------------------------------------------- //
// FACESET API                                                                //
// -------------------------------------------------------------------------- //

/// \class UsdGeomFaceSetAPI
/// 
/// This is a general purpose API schema used to describe many different 
/// organizations and modifications of a prim's faces' behavior. It's an add-on 
/// schema that can be applied many times to a prim with different face-set 
/// names. All the properties authored by the schema are namespaced under 
/// "faceSet:". The given name of the face-set provides additional namespacing
/// for the various per-face-set properties, which include the following:
/// 
/// \li<b>bool isPartition</b> - must the sets of enumerated faces be mutually 
/// exclusive?  I.e. can the same index appear more than once in faceIndices?
/// \li<b>int[] faceCounts</b> - length of faceCounts is the number of distinct 
/// groups of faces in this FaceSet.  Element i gives the number of faces in the 
/// i'th group.  The membership of each set can be variable over time, but the 
/// number of groups must be uniform over time, and the schema will enforce 
/// this.
/// \li<b>int[] faceIndices</b> - flattened list of all the faces in the 
/// face-set, with the faces of each group laid out sequentially
/// \li<b>rel binding</b> - (optional) if authored, possesses as many targets 
/// as there are groups of faces
/// 
/// The \b binding property elevates the schema beyond being purely a set; it 
/// allows us to effectively customize/specialize each group of faces in the 
/// set by establishing a relationship per-face-group to another prim that 
/// contains properties that consumers can consider to be overrides to whatever
/// values are provided by the prim that contains the faceSets, with the 
/// following caveats:
/// 
/// \li This schema makes no specific statement or behavioral constraint on what
/// properties should be considered overridable by the target of a 
/// per-face-group binding.  Specific prim schemas that define face-sets 
/// meaningful to them should declare what the expected behavior should be, with
/// respect to their core properties.
/// \li The targets of a single relationship themselves form a unique set. This
/// means that you cannot have two different face-groups in a face-set that have 
/// the same \b binding target.  You must merge the groups into one.
/// 
/// Note that we have not referred anywhere to meshes or polygons in this class.  
/// We use the term "face" generically, as this schema could be used equally 
/// well to partition curves within a UsdGeomCurves-derived schema.  Renderer
/// support for variation among curves in batched curves primitives is spotty, 
/// as of 2015, however.
/// 
/// Here's some sample code to create a face-set on a mesh prim and bind the 
/// different face groups in the face-set to different targets:
/// 
/// \code
/// UsdGeomMesh mesh(stage.GetPrimAtPath("/path/to/mesh"));
/// 
/// // Create a face-set named "look" on the mesh prim.
/// UsdGeomFaceSetAPI lookFaceSet = UsdGeomFaceSetAPI::Create(mesh, "look")
/// 
/// VtIntArray faceGroup1, faceGroup2;
/// // ... populate faceGroup1 and faceGroup2 with the desired set of face 
/// // ... indices.
/// 
/// SdfPath look1Path("/path/to/look1");
/// SdfPath look2Path("/path/to/look2");
/// 
/// // Now, bind faceGroup1 to look1 and faceGroup2 to look2.
/// lookFaceSet.AppendFaceGroup(faceGroup1, look1Path);
/// lookFaceSet.AppendFaceGroup(faceGroup2, look2Path);
/// \endcode
///
/// An alternate way to author face groups belonging to the face-set would be to
/// set the face-set properties directly using the API provided in 
/// \ref UsdGeomFaceSetAPI_PropertyValueAPI.
/// 
/// \code
/// VtIntArray faceCounts(2);
/// faceCounts[0] = faceGroup1.size();
/// faceCounts[1] = faceGroup2.size();
/// lookFaceSet.SetFaceCounts(faceCounts);
///
/// VtIntArray faceIndices;
/// // ... insert indices from faceGroup1 and faceGroup2 into faceIndices here.
/// lookFaceSet.SetFaceIndices(faceIndices);
/// 
/// lookFaceSet.SetBindingTargets([look1Path, look2Path])
/// \endcode
/// 
class UsdGeomFaceSetAPI: public UsdSchemaBase
{
public:
    /// Construct a UsdGeomFaceSetAPI with the given \p setName on 
    /// the UsdPrim \p prim .
    /// 
    explicit UsdGeomFaceSetAPI(const UsdPrim& prim=UsdPrim(), 
                               const TfToken &setName=TfToken())
        : UsdSchemaBase(prim),
          _setName(setName)
    {
    }

    /// Construct a USdGeomFaceSetAPI with the given \p setName on the 
    /// prim held by \p schemaObj .
    /// 
    explicit UsdGeomFaceSetAPI(const UsdSchemaBase& schemaObj, 
                               const TfToken &setName)
        : UsdSchemaBase(schemaObj.GetPrim()),
          _setName(setName)
    {
    }

    /// Destructor
    virtual ~UsdGeomFaceSetAPI();

private:
    // Returns true if the face-set contains the isPartition attribute. Note 
    // that this does not check the validity of the face-set attribute values. 
    // To check the validity, invoke \ref Validate().
    virtual bool _IsCompatible(const UsdPrim &prim) const;

public:

    /// \anchor UsdGeomFaceSetAPI_PropertyValueAPI
    /// \name FaceSet Property Value Getters and Setters
    /// 
    /// Convenience API for getting and setting values of the various face-set
    /// properties.
    /// 
    /// @{
    
    /// Returns the name of the face-set.
    /// 
    TfToken GetFaceSetName() const {
        return _setName;
    }

    /// Set whether the set of enumerated faces must be mutually exclusive.
    /// If \b isPartition is true, then any given face index can appear only 
    /// once in the \b faceIndices attribute value belonging to the face-set.
    /// 
    bool SetIsPartition(bool isPartition) const; 

    /// Returns whether the set of enumerated faces must be mutually exclusive.
    /// If this returns true, then any given face index can appear only 
    /// once in the \b faceIndices attribute value belonging to the face-set.
    /// 
    bool GetIsPartition() const;

    /// Sets the lengths of various groups of faces belonging to this face-set
    ///  at UsdTimeCode \p time.
    /// 
    /// Length of faceCounts is the number of distinct groups of faces in this 
    /// face-set. Element i gives the number of faces in the i'th group. The 
    /// membership of each set can be variable over time, but the number of 
    /// groups must be uniform over time, and this schema will enforce this.
    /// 
    /// \sa UsdGeomFaceSetAPI::GetFaceCounts()
    bool SetFaceCounts(const VtIntArray &faceCounts,
                       const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// Returns the lengths of various groups of faces belonging to this 
    /// face-set at UsdTimeCode \p time.
    /// 
    /// Length of faceCounts is the number of distinct groups of faces in this 
    /// face-set. Element i gives the number of faces in the i'th group. The 
    /// membership of each set can be variable over time, but the number of 
    /// groups must be uniform over time, and this schema will enforce this.
    /// 
    /// \sa UsdGeomFaceSetAPI::GetFaceIndices()
    /// \sa UsdGeomFaceSetAPI::SetFaceCounts()
    bool GetFaceCounts(VtIntArray *faceCounts, 
                       const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// Sets the flattened list of all the faces in the face-set, with the 
    /// faces of each group laid out sequentially at UsdTimeCode, \p time.
    /// 
    /// \sa UsdGeomFaceSetAPI::GetFaceCounts()
    /// \sa UsdGeomFaceSetAPI::SetFaceIndices()
    bool SetFaceIndices(const VtIntArray &faceIndices, 
                        const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// Returns the flattened list of all the faces in the face-set, with the 
    /// faces of each group laid out sequentially at UsdTimeCode, \p time.
    /// 
    /// \sa UsdGeomFaceSetAPI::GetFaceCounts()
    /// \sa UsdGeomFaceSetAPI::SetFaceIndices()
    bool GetFaceIndices(VtIntArray *faceIndices, 
                        const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// Sets the paths to target prims that the different groups of faces in 
    /// this face-set are bound to.
    /// 
    /// The number of \p bindings should match the number of groups of faces.
    /// 
    bool SetBindingTargets(const SdfPathVector &bindings) const;

    /// Returns the <b>resolved paths</b> to target prims that the different 
    /// groups of faces in this face-set are bound to.
    /// 
    /// In a valid face-set, the number of \p bindings always matches the number 
    /// of groups of faces.
    /// 
    bool GetBindingTargets(SdfPathVector *bindings) const;

    /// Appends a new face group containing the given \p faceIndices to an 
    /// existing face-set at the specified time ordinate, \p time and binds it 
    /// to the given \p bindingTarget. Returns true only upon success.
    /// 
    /// Here are a few things worth noting about this method:
    /// 
    /// \li The faceCount is gleaned from the length of the \p faceIndices 
    /// argument.
    /// \li If \p bindingTarget is empty, but there is already a face-group 
    /// authored with a non-empty bindingTarget, a warning is issued and no
    /// edits are performed.
    /// \li Similarly, if bindingTarget is not empty, but there is already a 
    /// face-group authored without binding targets, a warning is issued and 
    /// no edits are performed.
    /// \li If the face groups belonging to the face-set are animated, then 
    /// a new face group is \b appended if there are existing face groups 
    /// authored at the given time ordinate. 
    /// \li If there are no existing face groups authored at the given time 
    /// ordinate, then the face group being added will be the only one at 
    /// \p time.
    ///
    bool AppendFaceGroup(const VtIntArray &faceIndices,
                         const SdfPath &bindingTarget=SdfPath(),
                         const UsdTimeCode &time=UsdTimeCode::Default()) const;

    /// @}

    /// \anchor UsdGeomFaceSetAPI_RawProperties
    /// \name FaceSet Property API
    /// 
    /// API for getting and creating the "raw" properties associated with a 
    /// face-set. 
    /// 
    /// @{

    /// Returns the "isPartition" attribute associated with the face-set.
    /// This attribute deternines whether the set of enumerated faces must be
    /// mutually exclusive.
    /// 
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityUniform
    ///
    /// \ref GetIsPartition()
    ///
    UsdAttribute GetIsPartitionAttr() const;

    /// Creates the "isPartition" attribute associated with the face-set.
    /// 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    /// 
    UsdAttribute CreateIsPartitionAttr(const VtValue &defaultValue=VtValue(),
                                       bool writeSparsely=false) const;

    /// Returns the "faceCounts" attribute associated with the face-set.
    /// 
    /// \n  C++ Type: VtIntArray
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    ///
    /// \ref GetFaceCounts()
    ///
    UsdAttribute GetFaceCountsAttr() const;

    /// Creates the "faceCounts" attribute associated with the face-set.
    /// 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    ///
    /// \ref GetFaceCountsAttr()
    ///
    UsdAttribute CreateFaceCountsAttr(const VtValue &defaultValue=VtValue(),
                                      bool writeSparsely=false) const;

    /// Returns the "faceIndices" attribute associated with the face-set.
    /// 
    /// \n  C++ Type: VtIntArray
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// 
    /// \ref GetFaceIndices()
    ///
    UsdAttribute GetFaceIndicesAttr() const;

    /// Creates the "faceIndices" attribute associated with the face-set.
    ///
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    ///
    /// \ref GetFaceIndicesAttr()
    ///
    UsdAttribute CreateFaceIndicesAttr(const VtValue &defaultValue=VtValue(),
                                       bool writeSparsely=false) const;

    /// Returns the "bindingTargets" relationship associated with the face-set.
    /// 
    /// \ref GetBindingTargets()
    /// 
    UsdRelationship GetBindingTargetsRel() const;

    /// Creates the "bindingTargets" relationship associated with the face-set 
    /// and returns it.
    /// 
    /// \ref GetBindingTargetsRel()
    ///
    UsdRelationship CreateBindingTargetsRel() const;

    /// @}

    /// \anchor UsdGeomFaceSetAPI_StaticHelpers
    /// \name Static API
    /// 
    /// Convenience API for creating a face set and for getting the 
    /// existing face-sets on a prim.
    /// 
    /// @{

    /// Creates a new face-set on the given \p prim with the given \p setName.
    /// 
    /// The existence of a face-set on a prim is identified by the presence 
    /// of the associated isPartition attribute. Hence, this function also 
    /// creates the isPartition attribute and sets it to \p isPartition.
    /// 
    static UsdGeomFaceSetAPI Create(const UsdPrim &prim, 
                                    const TfToken &setName,
                                    bool isPartition=true);

    /// Creates a new face-set with the given \p setName on the prim held in 
    /// the given \p schemaObj.
    /// 
    /// The existence of a face-set on a prim is identified by the presence 
    /// of the associated isPartition attribute. Hence, this function also 
    /// creates the isPartition attribute and sets it to \p isPartition.
    /// 
    static UsdGeomFaceSetAPI Create(const UsdSchemaBase &schemaObj, 
                                    const TfToken &setName,
                                    bool isPartition=true);

    /// Returns the list of all face-sets on the given prim, \p prim.
    /// 
    /// A face-set will be included in the list only if the corresponding
    /// isPartition attribute is present on the prim.
    /// 
    static std::vector<UsdGeomFaceSetAPI> GetFaceSets(const UsdPrim &prim);

    /// Returns the list of all face-sets on the prim held by \p schemaObj.
    static std::vector<UsdGeomFaceSetAPI> GetFaceSets(
        const UsdSchemaBase &schemaObj);

    /// @}

    /// \anchor UsdGeomFaceSetAPI_Validation
    /// \name Face-Set Validation API
    /// 
    /// API for validating the properties of a face-set.
    /// 
    /// @{

    /// Validates the attribute values belonging to the face-set. Returns true 
    /// if the face-set has all valid attribute values. Returns false and 
    /// populates the \p reason output argument if the face-set has invalid 
    /// attribute values.
    /// 
    /// Here's the list of validations performed by this method:
    /// \li If the faceSet is a partition, the face indices must be 
    /// mutually exclusive.
    /// \li The size of faceIndices array should matche the sum of values in 
    /// the faceCounts array.
    /// \li The number of elements in faceCounts must not vary over time.
    /// \li If binding targets exist, their number should match the length of 
    /// the faceCounts array.
    /// 
    bool Validate(std::string *reason) const;

    /// @}

private:
    
    // Returns the faceSet:<setName>:isPartition attribute.
    UsdAttribute _GetIsPartitionAttr(bool create=false) const;

    // Returns the faceSet:<setName>:faceCounts attribute.
    UsdAttribute _GetFaceCountsAttr(bool create=false) const;

    // Returns the faceSet:<setName>:faceIndices attribute.
    UsdAttribute _GetFaceIndicesAttr(bool create=false) const;

    // Returns the faceSet:<setName>:binding relationship.
    UsdRelationship _GetBindingTargetsRel(bool create=false) const;

    // Returns the name of the attribute belonging to this FaceSet, given the 
    // base name of the attribute. Eg, if baseName is 'isPartition', this 
    // returns 'faceSet:<setName>:isPartition'.
    TfToken _GetFaceSetPropertyName(const TfToken &baseName) const;

    // The name of the FaceSet.
    TfToken _setName;
};

#endif
