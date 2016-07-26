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

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MColor.h>
#include <maya/MColorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>

class MFnDependencyNode;

namespace PxrUsdMayaUtil
{

struct cmpDag
{
    bool operator()( const MDagPath& lhs, const MDagPath& rhs ) const
    {
            std::string name1(lhs.fullPathName().asChar());
            std::string name2(rhs.fullPathName().asChar());
            return (name1.compare(name2) < 0);
     }
};
typedef std::set< MDagPath, cmpDag > ShapeSet;

// can't have a templated typedef.  this is the best we can do.
template <typename V>
struct MDagPathMap
{
    typedef std::map<MDagPath, V, cmpDag> Type;
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

// seconds per frame
double spf();

bool isAncestorDescendentRelationship(const MDagPath & path1,
    const MDagPath & path2);

// returns 0 if static, 1 if sampled, and 2 if a curve
int getSampledType(const MPlug& iPlug, bool includeConnectedChildren);

// 0 dont write, 1 write static 0, 2 write anim 0, 3 write anim 1
int getVisibilityType(const MPlug & iPlug);

// determines what order we do the rotation in, returns false if iOrder is
// kInvalid or kLast
bool getRotOrder(MTransformationMatrix::RotationOrder iOrder,
    unsigned int & oXAxis, unsigned int & oYAxis, unsigned int & oZAxis);

// determine if a Maya Object is animated or not
// copy from mayapit code (MayaPit.h .cpp)
bool isAnimated(MObject & object, bool checkParent = false);

// determine if a Maya Object is intermediate
bool isIntermediate(const MObject & object);

// returns true for visible and lod invisible and not templated objects
bool isRenderable(const MObject & object);

// strip iDepth namespaces from the node name, go from taco:foo:bar to bar
// for iDepth > 1
MString stripNamespaces(const MString & iNodeName, unsigned int iDepth);

std::string SanitizeName(const std::string& name);

// This to allow various pipeline to sanitize the colorset name for output
std::string SanitizeColorSetName(const std::string& name);

// Get the basecolor from the bound shader.  Returned colors will be in linear
// color space.
bool GetLinearShaderColor(
        const MFnDagNode& node,
        const int numFaces, 
        VtArray<GfVec3f> *RGBData, TfToken *RGBInterp, 
        VtArray<float> *AlphaData, TfToken *AlphaInterp);


MPlug GetConnected(const MPlug& plug);

void Connect(
        const MPlug& srcPlug,
        const MPlug& dstPlug,
        bool clearDstPlug);

/// For \p dagPath, returns a UsdPath corresponding to it.  
/// If \p mergeTransformAndShape and the dagPath is a shapeNode, it will return
/// the same value as MDagPathToUsdPath(transformPath) where transformPath is
/// the MDagPath for \p dagPath's transform node.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace ':' with '_'.
SdfPath MDagPathToUsdPath(const MDagPath& dagPath, bool mergeTransformAndShape);

/// Conveniency function to retreive custom data
bool GetBoolCustomData(UsdAttribute obj, TfToken key, bool defaultValue);

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

/// Given an \p usdAttr , extract the value at the default timecode and write
/// it on \p attrPlug.
/// This will make sure that color values (which are linear in usd) get
/// gamma corrected (display in maya).
/// Returns true if the value was set on the plug successfully, false otherwise.
bool setPlugValue(
        const UsdAttribute& attr,
        MPlug& attrPlug);

/// Given an \p usdAttr , extract the value at timecode \p time and write it
/// on \p attrPlug.
/// This will make sure that color values (which are linear in usd) get
/// gamma corrected (display in maya).
/// Returns true if the value was set on the plug successfully, false otherwise.
bool setPlugValue(
        const UsdAttribute& attr,
        UsdTimeCode time,
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

} // namespace PxrUsdMayaUtil

#endif // PXRUSDMAYA_UTIL_H
