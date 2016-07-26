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
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/ostreamHelpers.h"

#include "pxr/base/tf/type.h"


TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<GfBBox3d>();
}

void
GfBBox3d::_SetMatrices(const GfMatrix4d &matrix)
{
    const double PRECISION_LIMIT = 1.0e-13;
    double det;

    _isDegenerate = false;
    _matrix = matrix; 
    _inverse = matrix.GetInverse(&det, PRECISION_LIMIT);

    // Check for degenerate matrix:
    if (GfAbs(det) <= PRECISION_LIMIT) {
        _isDegenerate = true;
        _inverse.SetIdentity();
    }
}

double
GfBBox3d::GetVolume() const
{
    if (_box.IsEmpty())
        return 0.0;

    // The volume of a transformed box is just its untransformed
    // volume times the determinant of the upper-left 3x3 of the xform
    // matrix. Pretty cool, indeed.
    GfVec3d size = _box.GetSize();
    return fabs(_matrix.GetDeterminant3() * size[0] * size[1] * size[2]);
}

GfRange3d
GfBBox3d::ComputeAlignedRange() const
{
    if (_box.IsEmpty())
	return _box;
    
    // Method: James Arvo, Graphics Gems I, pp 548-550

    // Translate the origin and use the result as the min and max.
    GfVec3d trans(_matrix[3][0], _matrix[3][1], _matrix[3][2]);
    GfVec3d alignedMin = trans;
    GfVec3d alignedMax = trans;

    const GfVec3d &min = _box.GetMin();
    const GfVec3d &max = _box.GetMax();

    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
            double a = min[i] * _matrix[i][j];
            double b = max[i] * _matrix[i][j];
            if (a < b) {
                alignedMin[j] += a;
                alignedMax[j] += b;
            }
            else {
                alignedMin[j] += b;
                alignedMax[j] += a;
            }
        }
    }

    return GfRange3d(alignedMin, alignedMax);
}

GfBBox3d
GfBBox3d::Combine(const GfBBox3d &b1, const GfBBox3d &b2)
{
    GfBBox3d result;

    // If either box is empty, use the other as is
    if (b1.GetRange().IsEmpty())
        result = b2;
    else if (b2.GetRange().IsEmpty())
        result = b1;

    // If both boxes are degenerate, combine their projected
    // boxes. Otherwise, transform the degenerate box into the space
    // of the other box and combine the results in that space.
    else if (b1._isDegenerate) {
        if (b2._isDegenerate)
            result = GfBBox3d(GfRange3d::GetUnion(b1.ComputeAlignedRange(),
                                                  b2.ComputeAlignedRange()));
        else
            result = _CombineInOrder(b2, b1);
    }
    else if (b2._isDegenerate)
        result = _CombineInOrder(b1, b2);

    // Non-degenerate case: Neither box is empty and they are in
    // different spaces. To get the best results, we'll perform the
    // merge of the two boxes in each of the two spaces. Whichever
    // merge ends up being smaller (by volume) is the one we'll use.
    // Note that we don't use ComputeAlignedRange() as part of the test.
    // This is because projecting almost always adds a little extra
    // space and it gives an unfair advantage to the box that is more
    // closely aligned to the coordinate axes.
    else {
        GfBBox3d result1 = _CombineInOrder(b1, b2);
        GfBBox3d result2 = _CombineInOrder(b2, b1);

        // Test within a tolerance (based on volume) to make this
        // reasonably deterministic.
        double v1 = result1.GetVolume();
        double v2 = result2.GetVolume();
        double tolerance = GfMax(1e-10, 1e-6 * GfAbs(GfMax(v1, v2)));

        result = (GfAbs(v1 - v2) <= tolerance ? result1 :
                  (v1 < v2 ? result1 : result2));
    }

    // The _hasZeroAreaPrimitives is set to true if either of the
    // input boxes has it set to true.
    result.SetHasZeroAreaPrimitives(b1.HasZeroAreaPrimitives() ||
                                    b2.HasZeroAreaPrimitives());

    return result;
}

GfBBox3d
GfBBox3d::_CombineInOrder(const GfBBox3d &b1, const GfBBox3d &b2)
{
    // Transform b2 into b1's space to get b2t
    GfBBox3d b2t;
    b2t._box = b2._box;
    b2t._matrix  = b2._matrix * b1._inverse;
    b2t._inverse = b1._matrix * b2._inverse;

    // Compute the projection of this box into b1's space.
    GfRange3d proj = b2t.ComputeAlignedRange();

    // Extend b1 by this box to get the result.
    GfBBox3d result = b1;
    result._box.UnionWith(proj);
    return result;
}

GfVec3d
GfBBox3d::ComputeCentroid() const
{
    GfVec3d a = GetRange().GetMax();
    GfVec3d b = GetRange().GetMin(); 

    return GetMatrix().Transform( .5 * (a + b) );
}

std::ostream &
operator<<(std::ostream& out, const GfBBox3d& b)
{
    return out
        << "[("
        << Gf_OstreamHelperP(b.GetRange()) << ") (" 
        << Gf_OstreamHelperP(b.GetMatrix()) << ") "
        << (b.HasZeroAreaPrimitives() ? "true" : "false")
        << ']';
}
