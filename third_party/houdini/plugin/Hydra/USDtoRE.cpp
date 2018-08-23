// DreamWorks Animation LLC Confidential Information.
// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.

#include "USDtoRE.h"

#include <RE/RE_ElementArray.h>

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/tokens.h>

namespace {

// Houdini vec3f is actually the same as pixar's, so hack an assignment:
inline void assign(UT_Vector3F& out, const pxr::GfVec3f& in) { *(pxr::GfVec3f*)(&out) = in; }

enum AddResult { NONE, BAD, GOOD };

const UT_Vector3F defaultColor(0.6);

struct Line {
    unsigned a,b;
    void fix() { if (a > b) std::swap(a,b); }
    bool operator<(const Line& x) const { return a<x.a || (a == x.a && b < x.b); }
    bool operator==(const Line& x) const { return a == x.a && b == x.b; }
};

struct Collector {
    std::vector<UT_Vector3F> P;
    std::vector<UT_Vector3F> N;
    std::vector<UT_Vector3F> Cd;
    std::vector<unsigned> I;
    std::vector<Line> WI;
    bool getAll = true;
    bool hasCd = false;
    // size_t is not used to make it easier to use Houdini api
    unsigned size() const { return (unsigned)P.size(); }
    void resize(unsigned n) {
        P.resize(n);
        if (getAll) {
            N.resize(n);
            Cd.resize(n, defaultColor);
        }
    }
    unsigned sizeI() const { return (unsigned)I.size(); }
    void resizeI(unsigned n) { I.resize(n); }
    unsigned sizeWI() const { return (unsigned)WI.size(); }
    void resizeWI(unsigned n) { WI.resize(n); }
    void reserve(unsigned n, unsigned ni, unsigned nwi) {
        P.reserve(n);
        I.reserve(ni);
        if (getAll) {
            N.reserve(n);
            Cd.reserve(n);
            WI.reserve(nwi);
        }
    }
    bool empty() const { return I.empty(); }
    // remove duplicate lines from i0 to end
    void fixLines(unsigned i0)
    {
        auto start = WI.begin()+i0;
        auto end = WI.end();
        for (auto i = start; i < end; ++i) i->fix();
        std::sort(start, end);
        WI.resize(std::unique(start, end) - WI.begin());
    }
};

// Draws an object with the same silloette as an octohedron inscribed in bbox.
AddResult
addBoundable(const pxr::UsdGeomBoundable& prim,
             const pxr::UsdTimeCode& frame, const pxr::GfMatrix4d& xform,
             Collector& v)
{
    if (not prim) return NONE;

    pxr::UsdAttribute extentAttr = prim.GetExtentAttr();
    if (not extentAttr) return NONE;
    pxr::VtVec3fArray extent; extentAttr.Get(&extent, frame);
    if (extent.size() < 2) return NONE;

    const float x0 = extent[0][0];
    const float y0 = extent[0][1];
    const float z0 = extent[0][2];
    const float x1 = extent[1][0];
    const float y1 = extent[1][1];
    const float z1 = extent[1][2];
    if (x0 > x1) return NONE;

    // six points in the center of each face
    unsigned p0(v.size());
    v.resize(p0 + 6);
    UT_Vector3F* p = &v.P[p0];
    const float cx = (x0 + x1)/2;
    const float cy = (y0 + y1)/2;
    const float cz = (z0 + z1)/2;
    assign(*p++, xform.TransformAffine(pxr::GfVec3f(x0,cy,cz)));
    assign(*p++, xform.TransformAffine(pxr::GfVec3f(x1,cy,cz)));
    assign(*p++, xform.TransformAffine(pxr::GfVec3f(cx,y0,cz)));
    assign(*p++, xform.TransformAffine(pxr::GfVec3f(cx,y1,cz)));
    assign(*p++, xform.TransformAffine(pxr::GfVec3f(cx,cy,z0)));
    assign(*p++, xform.TransformAffine(pxr::GfVec3f(cx,cy,z1)));

    // 6 triangles making 3 diamond shapes along each plane through bounding box
    unsigned i0(v.sizeI());
    v.resizeI(i0 + 3 * 6);
    unsigned int* i = &v.I[i0];
    // xy 2 triangles
    *i++ = p0+0;
    *i++ = p0+1;
    *i++ = p0+2;
    *i++ = p0+0;
    *i++ = p0+1;
    *i++ = p0+3;
    // yz 2 triangles
    *i++ = p0+2;
    *i++ = p0+3;
    *i++ = p0+4;
    *i++ = p0+2;
    *i++ = p0+3;
    *i++ = p0+5;
    // xz 2 triangles
    *i++ = p0+0;
    *i++ = p0+1;
    *i++ = p0+4;
    *i++ = p0+0;
    *i++ = p0+1;
    *i++ = p0+5;

    return BAD;
}

// Only vertex attributes are supported, fake all others by putting one of the several
// values for a point on that point. Returns true if not constant.
bool
distribute(const pxr::VtVec3fArray& val, const pxr::TfToken& interpolation,
           UT_Vector3F* out, size_t n, const pxr::VtIntArray& indexes, const pxr::VtIntArray& counts)
{
    if (interpolation == pxr::UsdGeomTokens->vertex || interpolation == pxr::UsdGeomTokens->varying) {
        if (val.size() >= n) {
            for (size_t i = 0; i < n; ++i)
                assign(out[i], val[i]);
            return true;
        }
    } else if (interpolation == pxr::UsdGeomTokens->faceVarying) {
        if (val.size() >= indexes.size()) {
            for (size_t i = 0; i < indexes.size(); ++i)
                assign(out[indexes[i]], val[i]);
            return true;
        }
    } else if (interpolation == pxr::UsdGeomTokens->uniform) {
        if (val.size() >= counts.size()) {
            unsigned j = 0;
            for (size_t i = 0; i < counts.size(); ++i) {
                int count = counts[i];
                for (int k = 0; k < count; ++k)
                    assign(out[indexes[j+k]], val[i]);
                j += count;
            }
            return true;
        }
    } else { // assume constant
        if (not val.empty())
            for (size_t i = 0; i < n; ++i) assign(out[i], val[0]);
    }
    return false;
}

AddResult
addMesh(const pxr::UsdGeomMesh& mesh,
        const pxr::UsdTimeCode& frame, const pxr::GfMatrix4d& xform,
        Collector& v)
{
    if (not mesh) return NONE;

    pxr::UsdAttribute countsAttr = mesh.GetFaceVertexCountsAttr();
    if (not countsAttr) return NONE;
    pxr::VtIntArray counts; countsAttr.Get(&counts, frame);
    int numTri = 0;
    int numIndexes = 0;
    for (int count : counts) {
        numIndexes += count;
        if (count >= 3) numTri += count-2;
    }
    if (not numTri) return NONE;

    pxr::UsdAttribute indexAttr = mesh.GetFaceVertexIndicesAttr();
    if (not indexAttr) return NONE;
    pxr::VtIntArray indexes; indexAttr.Get(&indexes, frame);
    if (indexes.size() < (size_t)numIndexes) return NONE;

    pxr::UsdAttribute pointsAttr = mesh.GetPointsAttr();
    if (not pointsAttr) return NONE;
    pxr::VtVec3fArray points; pointsAttr.Get(&points, frame);
    const unsigned numPoints(points.size());
    if (numPoints < 3) return NONE;

    const unsigned p0(v.size());
    v.resize(p0 + numPoints);
    UT_Vector3F* p = &v.P[p0];
    for (auto& point : points)
        assign(*p++, xform.TransformAffine(point));

    bool leftHanded;
    {pxr::TfToken t; leftHanded = (mesh.GetOrientationAttr().Get(&t, frame) && t != pxr::UsdGeomTokens->rightHanded);}
    if (xform.IsLeftHanded()) leftHanded = !leftHanded;

    if (v.getAll) {
        bool hasN = false;
        pxr::UsdAttribute normalsAttr = mesh.GetNormalsAttr();
        if (normalsAttr) {
            pxr::VtVec3fArray val; normalsAttr.Get(&val, frame);
            // fixme: need to transform normals by transpose(xform).inverse
            hasN = distribute(val, mesh.GetNormalsInterpolation(), &v.N[p0], numPoints, indexes, counts);
        }
        if (not hasN) { // calculate normals as average of adjoining polygons
            unsigned j = 0;
            for (size_t i = 0; i < counts.size(); ++i) {
                const int count = counts[i];
                const UT_Vector3F& A(v.P[p0+indexes[j]]);
                const UT_Vector3F& B(v.P[p0+indexes[j+1]]);
                const UT_Vector3F& C(v.P[p0+indexes[j+count-1]]);
                UT_Vector3F n(cross(B-A, C-A));
                //n.normalize(); // it seems better to weigh them by polygon area
                for (int k = 0; k < count; ++k)
                    v.N[p0+indexes[j+k]] += n;
                j += count;
            }
            if (leftHanded)
                for (unsigned i = p0; i < p0+numPoints; ++i) v.N[i] *= -1;
            // for (unsigned i = p0; i < p0+numPoints; ++i) v.N[i].normalize(); // not necessary for beauty shader
        }
        pxr::UsdGeomPrimvar primvar = mesh.GetPrimvar(pxr::UsdGeomTokens->primvarsDisplayColor);
        if (primvar) {
            pxr::VtVec3fArray val;
            primvar.ComputeFlattened(&val, frame);
            if (distribute(val, primvar.GetInterpolation(), &v.Cd[p0], numPoints, indexes, counts))
                v.hasCd = true;
            else if (p0 && v.Cd[p0] != v.Cd[0])
                v.hasCd = true;
        }
    }

    unsigned int i0 = v.sizeI();
    v.resizeI(i0 + 3 * numTri);
    unsigned int* i = &v.I[i0];
    unsigned j = 0;
    if (leftHanded) {
        for (int count : counts) {
            for (int k = 2; k < count; ++k) {
                *i++ = p0+indexes[j];
                *i++ = p0+indexes[j+k-1];
                *i++ = p0+indexes[j+k];
            }
            j += count;
        }
    } else {
        for (int count : counts) {
            for (int k = 2; k < count; ++k) {
                *i++ = p0+indexes[j];
                *i++ = p0+indexes[j+k];
                *i++ = p0+indexes[j+k-1];
            }
            j += count;
        }
    }

    if (v.getAll) {
        // add wire loops
        i0 = v.sizeWI();
        v.resizeWI(i0 + numIndexes);
        j = 0;
        Line* i = &v.WI[i0];
        for (int count : counts) {
            i->a = p0+indexes[j];
            for (int k = 1; k < count; ++k) {
                i->b = (i+1)->a = p0+indexes[j+k];
                ++i;
            }
            i->b = p0+indexes[j];
            i++;
            j += count;
        }
        v.fixLines(i0);
    }

    return GOOD;
}

enum PurposeState { TOP, UNKNOWN, MATCH };

// Append the vertices of prim to v. Recursively do the children.
// Result indicates if it was a "good" piece of geometry (ie accurately converted)
AddResult addPrim(const pxr::UsdPrim& prim,
                  const pxr::UsdTimeCode& frame, const pxr::GfMatrix4d& xform, const pxr::TfTokenVector& purposes,
                  Collector& v,
                  PurposeState purposeState = TOP)
{
    // based on code in UsdGeomImagable, the highest node with a non-default purpose
    // applies. So this remembers if a purpose was found already and uses it. For
    // the top prim the normal ComputePurpose is done in order to check parents,
    // but when recursing we only need to check the local attribute.
    if (purposeState != MATCH) {
        pxr::UsdGeomImageable img(prim);
        if (img) {
            pxr::TfToken purpose;
            if (purposeState == TOP)
                purpose = img.ComputePurpose();
            else
                img.GetPurposeAttr().Get(&purpose);
            if (purpose == pxr::UsdGeomTokens->default_) {
                purposeState = UNKNOWN;
            } else {
                bool match = false;
                for (auto&& i : purposes) if (i == purpose) {match = true; break;}
                if (not match) return NONE;
                purposeState = MATCH;
            }
        } else {
            purposeState = UNKNOWN;
        }
    }

    // Convert the master if this is an instance (note this will completely undo
    // the reuse of data by instancing)
    if (pxr::UsdPrim master = prim.GetMaster())
        return addPrim(master, frame, xform, purposes, v, purposeState);
    // Draw an instance proxy (this should only happen if USD Import selects a child
    // of an instance)
    if (pxr::UsdPrim master = prim.GetPrimInMaster())
        return addPrim(master, frame, xform, purposes, v, purposeState);

    // Try classes that we know how to convert
    if (auto result = addMesh(pxr::UsdGeomMesh(prim), frame, xform, v)) return result;

    // try all the children
    if (not pxr::UsdGeomPointInstancer(prim)) { // don't draw the Prototype in PointInstancer
        AddResult result = NONE;
        for (const pxr::UsdPrim& child : prim.GetChildren()) {
            // ignore invisible children
            if (auto imageable = pxr::UsdGeomImageable(child)) {
                pxr::TfToken t; imageable.GetVisibilityAttr().Get(&t, frame);
                if (t == pxr::UsdGeomTokens->invisible)
                    continue;
            }
            // Apply the xform of this child
            const pxr::GfMatrix4d* xformToUse = &xform;
            pxr::GfMatrix4d localXform;
            if (auto xformable = pxr::UsdGeomXformable(child)) {
                bool reset; xformable.GetLocalTransformation(&localXform, &reset, frame);
                if (not reset) localXform *= xform;
                xformToUse = &localXform;
            }
            switch (addPrim(child, frame, *xformToUse, purposes, v, purposeState)) {
            case NONE: break;
            case BAD: if (not result) result = BAD; break;
            case GOOD: result = GOOD; break;
            }
        }
        if (result) return result;
    }

    // try drawing this object's bounding box as an approximation
    return addBoundable(pxr::UsdGeomBoundable(prim), frame, xform, v);
}

}

/// Convert a UsdPrim to an RE_Geometry, creating or updating *geo (it is apparently
/// faster to re-use an existing RE_Geometry, which is why this odd api is used).
/// It may set geo to null if there is nothing visible.
/// If prim is an xformable, it's transform is already in xform (!). This matches how
/// USD Import stores prim transforms.
///
/// If "getAll" is false then the result is only used for hit detection. "getAll" adds
/// colors, normals, and wireframe information.
///
/// This returns true if the RE_Geometry is considered "good" (in that drawing it will
/// look acceptable to the user).
bool
USDtoRE(const pxr::UsdPrim& prim,
        double frame, const pxr::GfMatrix4d& xform, const pxr::TfTokenVector& purposes,
        RE_Render* r, std::unique_ptr<RE_Geometry>& geo,
        unsigned* numPrims, bool getAll)
{
    Collector v;
    v.getAll = getAll;
    if (geo) { // assume new geometry is same size as previous one
        size_t n = geo->getNumPoints();
        v.reserve(n, n*6, n*6); // I can't figure out how to extract index array size from RE_Geometry
    }

    bool result = prim && addPrim(prim, pxr::UsdTimeCode(frame), xform, purposes, v) == GOOD;

    if (v.empty()) { // houdini does not like empty RE_Geometry
        geo.reset();
    } else {
        if (not geo)
            geo.reset(new RE_Geometry(v.size(), false));
        else
            geo->setNumPoints(v.size());
        geo->createAttribute(r, "P", RE_GPU_FLOAT32, 3, v.P[0].data());
        if (getAll) {
            geo->createAttribute(r, "N", RE_GPU_FLOAT32, 3, v.N[0].data());
            // color+alpha are needed or it does not draw:
            if (v.hasCd) {
                geo->createAttribute(r, "Cd", RE_GPU_FLOAT32, 3, v.Cd[0].data());
            } else {
                geo->createConstAttribute(r, "Cd", RE_GPU_FLOAT32, 3, v.Cd[0].data());
            }
            // alpha other than 1.0 does not work, most likely because I am using the
            // Houdini shaders incorrectly. If this is fixed using displayOpacity would be nice.
            fpreal32 alpha  = 1.0;
            geo->createConstAttribute(r, "Alpha", RE_GPU_FLOAT32, 1, &alpha);
        }
        geo->connectIndexedPrims(r, RE_GEO_SHADED_IDX, RE_PRIM_TRIANGLES, v.sizeI(), &v.I[0], nullptr, true);
        if (numPrims) *numPrims = v.sizeI()/3;
        if (getAll && v.sizeWI())
            geo->connectIndexedPrims(r, RE_GEO_WIRE_IDX, RE_PRIM_LINES, v.WI.size()*2, &v.WI[0].a, nullptr, true);
    }

    return result;
}

// TM and (c) 2017 DreamWorks Animation LLC.  All Rights Reserved.
// Reproduction in whole or in part without prior written permission of a
// duly authorized representative is prohibited.
