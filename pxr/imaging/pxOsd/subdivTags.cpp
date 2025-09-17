//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file pxOsd/subdivTags.h
///

#include "pxr/imaging/pxOsd/subdivTags.h"
#include "pxr/base/arch/hash.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE


PxOsdSubdivTags::ID
PxOsdSubdivTags::ComputeHash() const {

    ID hash = 0;

    hash = ArchHash64((const char*)&_vtxInterpolationRule,
                      sizeof(_vtxInterpolationRule), hash);

    hash = ArchHash64((const char*)&_fvarInterpolationRule,
                      sizeof(_fvarInterpolationRule), hash);

    hash = ArchHash64((const char*)&_creaseMethod,
                      sizeof(_creaseMethod), hash);

    hash = ArchHash64((const char*)&_trianglesSubdivision,
                      sizeof(_trianglesSubdivision), hash);

    hash = ArchHash64((const char*)_cornerIndices.cdata(),
                      _cornerIndices.size() * sizeof(int), hash);

    hash = ArchHash64((const char*)_cornerWeights.cdata(),
                      _cornerWeights.size() * sizeof(float), hash);

    hash = ArchHash64((const char*)_creaseIndices.cdata(),
                      _creaseIndices.size() * sizeof(int), hash);

    hash = ArchHash64((const char*)_creaseLengths.cdata(),
                      _creaseLengths.size() * sizeof(int), hash);

    hash = ArchHash64((const char*)_creaseWeights.cdata(),
                      _creaseWeights.size() * sizeof(float), hash);

    return hash;
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
        << st.GetCornerWeights() << "))";
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
        && lhs.GetCornerWeights() == rhs.GetCornerWeights();
}

bool 
operator!=(const PxOsdSubdivTags& lhs, const PxOsdSubdivTags& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

