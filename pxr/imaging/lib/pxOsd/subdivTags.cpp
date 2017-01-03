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
///
/// \file pxOsd/subdivTags.h
///

#include "pxr/imaging/pxOsd/subdivTags.h"

PxOsdSubdivTags::ID
PxOsdSubdivTags::ComputeHash() const {

    uint32_t hash = 0;

    hash = ArchHash((const char*)&_vtxInterpolationRule,
                    sizeof(_vtxInterpolationRule), hash);

    hash = ArchHash((const char*)&_fvarInterpolationRule,
                    sizeof(_fvarInterpolationRule), hash);

    hash = ArchHash((const char*)&_creaseMethod,
                    sizeof(_creaseMethod), hash);

    hash = ArchHash((const char*)&_trianglesSubdivision,
                    sizeof(_trianglesSubdivision), hash);

    hash = ArchHash((const char*)_holeIndices.cdata(),
                    _holeIndices.size() * sizeof(int), hash);

    hash = ArchHash((const char*)_cornerIndices.cdata(),
                    _cornerIndices.size() * sizeof(int), hash);

    hash = ArchHash((const char*)_cornerWeights.cdata(),
                    _cornerWeights.size() * sizeof(float), hash);

    hash = ArchHash((const char*)_creaseIndices.cdata(),
                    _creaseIndices.size() * sizeof(int), hash);

    hash = ArchHash((const char*)_creaseLengths.cdata(),
                    _creaseLengths.size() * sizeof(int), hash);

    hash = ArchHash((const char*)_creaseWeights.cdata(),
                    _creaseWeights.size() * sizeof(float), hash);

    // promote to size_t
    return (ID)hash;
}

std::ostream& 
operator << (std::ostream &out, PxOsdSubdivTags const &st)
{
    out << "(" << st.GetVertexInterpolationRule() << ", "
        << st.GetFaceVaryingInterpolationRule() << ", "
        << st.GetCreaseMethod() << ", "
        << st.GetTriangleSubdivision() << ", ("
        << st.GetCreaseIndices() << "), ("
        << st.GetCreaseLengths() << "), ("
        << st.GetCreaseWeights() << "), ("
        << st.GetCornerIndices() << "), ("
        << st.GetCornerWeights() << "), ("
        << st.GetHoleIndices()   << "))";
    return out;
}

bool 
operator==(const PxOsdSubdivTags& lhs, const PxOsdSubdivTags& rhs)
{
    return  lhs.GetVertexInterpolationRule() == rhs.GetVertexInterpolationRule() 
        && lhs.GetFaceVaryingInterpolationRule() == rhs.GetFaceVaryingInterpolationRule()
        && lhs.GetCreaseMethod() == rhs.GetCreaseMethod()
        && lhs.GetTriangleSubdivision() == rhs.GetTriangleSubdivision()
        && lhs.GetCreaseIndices() == rhs.GetCreaseIndices()
        && lhs.GetCreaseLengths() == rhs.GetCreaseLengths()
        && lhs.GetCreaseWeights() == rhs.GetCreaseWeights()
        && lhs.GetCornerIndices() == rhs.GetCornerIndices()
        && lhs.GetCornerWeights() == rhs.GetCornerWeights()
        && lhs.GetHoleIndices() == rhs.GetHoleIndices();
}

bool 
operator!=(const PxOsdSubdivTags& lhs, const PxOsdSubdivTags& rhs)
{
    return !(lhs == rhs);
}
