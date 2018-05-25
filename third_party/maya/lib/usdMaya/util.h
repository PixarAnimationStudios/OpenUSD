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
#ifndef PXRUSDMAYA_UTIL_H
#define PXRUSDMAYA_UTIL_H

/// \file util.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MDataHandle.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNumericData.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <map>
#include <set>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace PxrUsdMayaUtil
{

struct cmpDag
{
    bool operator()( const MDagPath& lhs, const MDagPath& rhs ) const
    {
        return strcmp(lhs.fullPathName().asChar(), rhs.fullPathName().asChar()) < 0;
    }
};
typedef std::set< MDagPath, cmpDag > ShapeSet;

// can't have a templated typedef.  this is the best we can do.
template <typename V>
struct MDagPathMap
{
    typedef std::map<MDagPath, V, cmpDag> Type;
};

/// RAII-style helper for destructing an MDataHandle obtained from a plug
/// once it goes out of scope.
class MDataHandleHolder : public TfRefBase {
    MPlug _plug;
    MDataHandle _dataHandle;

public:
    static TfRefPtr<MDataHandleHolder> New(const MPlug& plug);
    MDataHandle GetDataHandle() { return _dataHandle; }

private:
    MDataHandleHolder(const MPlug& plug, MDataHandle dataHandle);
    ~MDataHandleHolder();
};

inline MStatus isFloat(MString str, const MString & usage)
{
    MStatus status = MS::kSuccess;

    if (!str.isFloat())
    {
        MGlobal::displayInfo(usage);
        status = MS::kFailure;
    }

    return status;
}

inline MStatus isUnsigned(MString str, const MString & usage)
{
    MStatus status = MS::kSuccess;

    if (!str.isUnsigned())
    {
        MGlobal::displayInfo(usage);
        status = MS::kFailure;
    }

    return status;
}

// safely inverse a scale component
inline double inverseScale(double scale)
{
    const double kScaleEpsilon = 1.0e-12;

    if (scale < kScaleEpsilon && scale >= 0.0)
        return 1.0 / kScaleEpsilon;
    else if (scale > -kScaleEpsilon && scale < 0.0)
        return 1.0 / -kScaleEpsilon;
    else
        return 1.0 / scale;
}

const double MillimetersPerInch = 25.4;

/// Converts the given value \p mm in millimeters to the equivalent value
/// in inches.
inline
double
ConvertMMToInches(double mm) {
    return mm / MillimetersPerInch;
}

/// Converts the given value \p inches in inches to the equivalent value
/// in millimeters.
inline
double
ConvertInchesToMM(double inches) {
    return inches * MillimetersPerInch;
}

const double MillimetersPerCentimeter = 10.0;

/// Converts the given value \p mm in millimeters to the equivalent value
/// in centimeters.
inline
double
ConvertMMToCM(double mm) {
    return mm / MillimetersPerCentimeter;
}

/// Converts the given value \p cm in centimeters to the equivalent value
/// in millimeters.
inline
double
ConvertCMToMM(double cm) {
    return cm * MillimetersPerCentimeter;
}

// seconds per frame
PXRUSDMAYA_API
double spf();

/// Gets the Maya MObject for the node named \p nodeName.
PXRUSDMAYA_API
MStatus GetMObjectByName(const std::string& nodeName, MObject& mObj);

/// Gets the Maya MDagPath for the node named \p nodeName.
PXRUSDMAYA_API
MStatus GetDagPathByName(const std::string& nodeName, MDagPath& dagPath);

/// Gets the Maya MPlug for the given \p attrPath.
/// The attribute path should be specified as "nodeName.attrName" (the format
/// used by MEL).
PXRUSDMAYA_API
MStatus GetPlugByName(const std::string& attrPath, MPlug& plug);

/// Get the MPlug for the output time attribute of Maya's global time object
///
/// The Maya API does not appear to provide any facilities for getting a handle
/// to the global time object (e.g. "time1"). We need to find this object in
/// order to make connections between its "outTime" attribute and the input
/// "time" attributes on assembly nodes when their "Playback" representation is
/// activated.
///
/// This function makes a best effort attempt to find "time1" by looking through
/// all MFn::kTime function set objects in the scene and returning the one whose
/// outTime attribute matches the current time. If no such object can be found,
/// an invalid plug is returned.
PXRUSDMAYA_API
MPlug
GetMayaTimePlug();

PXRUSDMAYA_API
bool isAncestorDescendentRelationship(const MDagPath & path1,
    const MDagPath & path2);

// returns 0 if static, 1 if sampled, and 2 if a curve
PXRUSDMAYA_API
int getSampledType(const MPlug& iPlug, bool includeConnectedChildren);

// 0 dont write, 1 write static 0, 2 write anim 0, 3 write anim 1
PXRUSDMAYA_API
int getVisibilityType(const MPlug & iPlug);

// determines what order we do the rotation in, returns false if iOrder is
// kInvalid or kLast
PXRUSDMAYA_API
bool getRotOrder(MTransformationMatrix::RotationOrder iOrder,
    unsigned int & oXAxis, unsigned int & oYAxis, unsigned int & oZAxis);

// determine if a Maya Object is animated or not
// copy from mayapit code (MayaPit.h .cpp)
PXRUSDMAYA_API
bool isAnimated(MObject & object, bool checkParent = false);

// Determine if a specific Maya plug is animated or not.
PXRUSDMAYA_API
bool isPlugAnimated(const MPlug& plug);

// determine if a Maya Object is intermediate
PXRUSDMAYA_API
bool isIntermediate(const MObject & object);

// returns true for visible and lod invisible and not templated objects
PXRUSDMAYA_API
bool isRenderable(const MObject & object);

// strip iDepth namespaces from the node name, go from taco:foo:bar to bar
// for iDepth > 1
PXRUSDMAYA_API
MString stripNamespaces(const MString & iNodeName, unsigned int iDepth);

PXRUSDMAYA_API
std::string SanitizeName(const std::string& name);

// This to allow various pipeline to sanitize the colorset name for output
PXRUSDMAYA_API
std::string SanitizeColorSetName(const std::string& name);

/// Get the base colors and opacities from the shader(s) bound to \p node.
/// Returned colors will be in linear color space.
///
/// A single value for each of color and alpha will be returned,
/// interpolation will be constant, and assignmentIndices will be empty.
///
PXRUSDMAYA_API
bool GetLinearShaderColor(
        const MFnDagNode& node,
        PXR_NS::VtArray<PXR_NS::GfVec3f> *RGBData,
        PXR_NS::VtArray<float> *AlphaData,
        PXR_NS::TfToken *interpolation,
        PXR_NS::VtArray<int> *assignmentIndices);

/// Get the base colors and opacities from the shader(s) bound to \p mesh.
/// Returned colors will be in linear color space.
///
/// If the entire mesh has a single shader assignment, a single value for each
/// of color and alpha will be returned, interpolation will be constant, and
/// assignmentIndices will be empty.
///
/// Otherwise, a color and alpha value will be returned for each shader
/// assigned to any face of the mesh. \p assignmentIndices will be the length
/// of the number of faces with values indexing into the color and alpha arrays
/// representing per-face assignments. Faces with no assigned shader will have
/// a value of -1 in \p assignmentIndices. \p interpolation will be uniform.
///
PXRUSDMAYA_API
bool GetLinearShaderColor(
        const MFnMesh& mesh,
        PXR_NS::VtArray<PXR_NS::GfVec3f> *RGBData,
        PXR_NS::VtArray<float> *AlphaData,
        PXR_NS::TfToken *interpolation,
        PXR_NS::VtArray<int> *assignmentIndices);

/// Combine distinct indices that point to the same values to all point to the
/// same index for that value. This will potentially shrink the data array.
PXRUSDMAYA_API
void MergeEquivalentIndexedValues(
        PXR_NS::VtArray<float>* valueData,
        PXR_NS::VtArray<int>* assignmentIndices);

/// Combine distinct indices that point to the same values to all point to the
/// same index for that value. This will potentially shrink the data array.
PXRUSDMAYA_API
void MergeEquivalentIndexedValues(
        PXR_NS::VtArray<PXR_NS::GfVec2f>* valueData,
        PXR_NS::VtArray<int>* assignmentIndices);

/// Combine distinct indices that point to the same values to all point to the
/// same index for that value. This will potentially shrink the data array.
PXRUSDMAYA_API
void MergeEquivalentIndexedValues(
        PXR_NS::VtArray<PXR_NS::GfVec3f>* valueData,
        PXR_NS::VtArray<int>* assignmentIndices);

/// Combine distinct indices that point to the same values to all point to the
/// same index for that value. This will potentially shrink the data array.
PXRUSDMAYA_API
void MergeEquivalentIndexedValues(
        PXR_NS::VtArray<PXR_NS::GfVec4f>* valueData,
        PXR_NS::VtArray<int>* assignmentIndices);

/// Attempt to compress faceVarying primvar indices to uniform, vertex, or
/// constant interpolation if possible. This will potentially shrink the
/// indices array and will update the interpolation if any compression was
/// possible.
PXRUSDMAYA_API
void CompressFaceVaryingPrimvarIndices(
        const MFnMesh& mesh,
        PXR_NS::TfToken *interpolation,
        PXR_NS::VtArray<int>* assignmentIndices);

/// If any components in \p assignmentIndices are unassigned (-1), the given
/// default value will be added to uvData and all of those components will be
/// assigned that index, which is returned in \p unassignedValueIndex.
/// Returns true if unassigned values were added and indices were updated, or
/// false otherwise.
PXRUSDMAYA_API
bool AddUnassignedUVIfNeeded(
        PXR_NS::VtArray<PXR_NS::GfVec2f>* uvData,
        PXR_NS::VtArray<int>* assignmentIndices,
        int* unassignedValueIndex,
        const PXR_NS::GfVec2f& defaultUV);

/// If any components in \p assignmentIndices are unassigned (-1), the given
/// default values will be added to RGBData and AlphaData and all of those
/// components will be assigned that index, which is returned in
/// \p unassignedValueIndex.
/// Returns true if unassigned values were added and indices were updated, or
/// false otherwise.
PXRUSDMAYA_API
bool AddUnassignedColorAndAlphaIfNeeded(
        PXR_NS::VtArray<PXR_NS::GfVec3f>* RGBData,
        PXR_NS::VtArray<float>* AlphaData,
        PXR_NS::VtArray<int>* assignmentIndices,
        int* unassignedValueIndex,
        const PXR_NS::GfVec3f& defaultRGB,
        const float defaultAlpha);

/// Get whether \p plug is authored in the Maya scene.
///
/// A plug is considered authored if its value has been changed from the
/// default (or since being brought in from a reference for plugs on nodes from
/// referenced files), or if the plug has a connection. Otherwise, it is
/// considered unauthored.
///
/// Note that MPlug::getSetAttrCmds() is currently not declared const, so
/// IsAuthored() here must take a non-const MPlug.
PXRUSDMAYA_API
bool IsAuthored(MPlug& plug);

PXRUSDMAYA_API
MPlug GetConnected(const MPlug& plug);

PXRUSDMAYA_API
void Connect(
        const MPlug& srcPlug,
        const MPlug& dstPlug,
        bool clearDstPlug);

/// Get a named child plug of \p plug by name.
PXRUSDMAYA_API
MPlug FindChildPlugByName(const MPlug& plug, const MString& name);

/// For \p dagPath, returns a UsdPath corresponding to it.  
/// If \p mergeTransformAndShape and the dagPath is a shapeNode, it will return
/// the same value as MDagPathToUsdPath(transformPath) where transformPath is
/// the MDagPath for \p dagPath's transform node.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace ':' with '_'.
PXRUSDMAYA_API
PXR_NS::SdfPath MDagPathToUsdPath(const MDagPath& dagPath, bool mergeTransformAndShape);

/// Convenience function to retrieve custom data
PXRUSDMAYA_API
bool GetBoolCustomData(PXR_NS::UsdAttribute obj, PXR_NS::TfToken key, bool defaultValue);

// Compute the value of \p attr, returning true upon success.
//
// Only valid for T's bool, short, int, float, double, and MObject.  No
// need to boost:mpl verify it, though, since getValue() will fail to compile
// with any other types
template <typename T>
bool getPlugValue(MFnDependencyNode const &depNode, 
                  MString const &attr, 
                  T *val,
                  bool *isAnimated = NULL)
{
    MPlug plg = depNode.findPlug( attr, /* findNetworked = */ true );
    if ( !plg.isNull() ) {
        if (isAnimated)
            *isAnimated = plg.isDestination();
        return plg.getValue(*val);
    }

    return false;
}

/// Convert a Gf matrix to an MMatrix.
PXRUSDMAYA_API
MMatrix GfMatrixToMMatrix(const GfMatrix4d& mx);

// Like getPlugValue, but gets the matrix stored inside the MFnMatrixData on a
// plug.
// Returns true upon success, placing the matrix in the outVal parameter.
PXRUSDMAYA_API
bool getPlugMatrix(const MFnDependencyNode& depNode,
                   const MString& attr,
                   MMatrix* outVal);

/// Set a matrix value on plug name \p attr, of \p depNode.
/// Returns true if the value was set on the plug successfully, false otherwise.
PXRUSDMAYA_API
bool setPlugMatrix(const MFnDependencyNode& depNode,
                   const MString& attr,
                   const GfMatrix4d& mx);

PXRUSDMAYA_API
bool setPlugMatrix(const GfMatrix4d& mx, MPlug& plug);

/// Given an \p usdAttr , extract the value at the default timecode and write
/// it on \p attrPlug.
/// This will make sure that color values (which are linear in usd) get
/// gamma corrected (display in maya).
/// Returns true if the value was set on the plug successfully, false otherwise.
PXRUSDMAYA_API
bool setPlugValue(
        const PXR_NS::UsdAttribute& attr,
        MPlug& attrPlug);

/// Given an \p usdAttr , extract the value at timecode \p time and write it
/// on \p attrPlug.
/// This will make sure that color values (which are linear in usd) get
/// gamma corrected (display in maya).
/// Returns true if the value was set on the plug successfully, false otherwise.
PXRUSDMAYA_API
bool setPlugValue(
        const PXR_NS::UsdAttribute& attr,
        PXR_NS::UsdTimeCode time,
        MPlug& attrPlug);

/// \brief sets \p attr to have value \p val, assuming it exists on \p
/// depNode.  Returns true if successful.
template <typename T>
bool setPlugValue(MFnDependencyNode const &depNode, 
                  MString const &attr, 
                  T val)
{
    MPlug plg = depNode.findPlug( attr, /* findNetworked = */ false );
    if ( !plg.isNull() ) {
        return plg.setValue(val);
    }

    return false;
}

/// Obtains an RAII helper object for accessing the MDataHandle stored on the
/// plug. When the helper object goes out of scope, the data handle will be
/// destructed.
/// If the plug's data handle could not be obtained, returns nullptr.
PXRUSDMAYA_API
TfRefPtr<MDataHandleHolder> GetPlugDataHandle(const MPlug& plug);

PXRUSDMAYA_API
bool createStringAttribute(
        MFnDependencyNode& depNode,
        const MString& attr);

PXRUSDMAYA_API
bool createNumericAttribute(
        MFnDependencyNode& depNode,
        const MString& attr,
        MFnNumericData::Type type);

/// Reads values from the given \p argData into a VtDictionary, using the
/// \p guideDict to figure out which keys and what type of values should be read
/// from \p argData.
/// Mainly useful for parsing arguments in commands all at once.
PXRUSDMAYA_API
VtDictionary GetDictionaryFromArgDatabase(
    const MArgDatabase& argData,
    const VtDictionary& guideDict);

/// Parses \p value based on the type of \p key in \p guideDict, returning the
/// parsed value wrapped in a VtValue.
/// Raises a coding error if \p key doesn't exist in \p guideDict.
/// Mainly useful for parsing arguments one-by-one in translators' option
/// strings. If you have an MArgList/MArgParser/MArgDatabase, it's going to be
/// way simpler to use GetDictionaryFromArgDatabase() instead.
PXRUSDMAYA_API
VtValue ParseArgumentValue(
    const std::string& key,
    const std::string& value,
    const VtDictionary& guideDict);

} // namespace PxrUsdMayaUtil

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_UTIL_H
