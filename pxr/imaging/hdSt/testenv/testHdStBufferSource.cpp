//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2i.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3i.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

template <typename T>
void
BasicTest(size_t numComponents)
{
    std::cout << TfType::Find<T>().GetTypeName();
    std::cout << "------------------------------------------------------\n";
    T value(1);
    std::cout << value << std::endl;

    VtValue v(value);
    HdVtBufferSource b(HdTokens->points, v);
    std::cout << b << std::endl;

    TF_VERIFY(b.GetTupleType() == HdGetValueTupleType(v));
    TF_VERIFY(HdGetComponentCount(b.GetTupleType().type) == numComponents);
    TF_VERIFY(HdDataSizeOfTupleType(b.GetTupleType()) == sizeof(T));
    TF_VERIFY(b.GetNumElements() == 1);

    TF_VERIFY(*(T*)b.GetData() == value);
    std::cout << "\n";
}

template <typename DBL, typename FLT>
void
BasicDoubleTest(size_t numComponents)
{
    std::cout << TfType::Find<DBL>().GetTypeName();
    std::cout << "------------------------------------------------------\n";
    DBL valueDbl(1);
    FLT valueFlt(1);
    std::cout << valueDbl << std::endl;

    VtValue vDbl(valueDbl);
    VtValue vFlt(valueFlt);

    // Doubles can be used for the buffer source
    {
        bool const allowDoubles = true;
        HdVtBufferSource b(HdTokens->points, vDbl, 1, allowDoubles);
        std::cout << "Double -> Double\n" << b << std::endl;

        TF_VERIFY(b.GetTupleType() == HdGetValueTupleType(vDbl));
        TF_VERIFY(HdGetComponentCount(b.GetTupleType().type) == numComponents);
        TF_VERIFY(HdDataSizeOfTupleType(b.GetTupleType()) == sizeof(DBL));
        TF_VERIFY(b.GetNumElements() == 1);

        TF_VERIFY(*(DBL*)b.GetData() == valueDbl);
        std::cout << "\n";
    }

    // Doubles must be converted to floats for the buffer source
    {
        bool const allowDoubles = false;
        HdVtBufferSource b(HdTokens->points, vDbl, 1, allowDoubles);
        std::cout << "Double -> Float\n" << b << std::endl;

        TF_VERIFY(b.GetTupleType() == HdGetValueTupleType(vFlt));
        TF_VERIFY(HdGetComponentCount(b.GetTupleType().type) == numComponents);
        TF_VERIFY(HdDataSizeOfTupleType(b.GetTupleType()) == sizeof(FLT));
        TF_VERIFY(b.GetNumElements() == 1);

        TF_VERIFY(*(FLT*)b.GetData() == valueFlt);
        std::cout << "\n";
    }
}

template <typename ELT>
void
BasicTest(size_t length, size_t numComponents)
{
    std::cout << "[ " << TfType::Find<ELT>().GetTypeName() << " ]";
    std::cout << "------------------------------------------------------\n";
    VtArray<ELT> vtArray(length);
    for (size_t i = 0; i < length; i++) {
        vtArray[i] = ELT(i);
    }
    std::cout << vtArray << std::endl;
    std::cout << "Source bytes: " << vtArray.size() * sizeof(ELT) << std::endl;

    VtValue v(vtArray);

    // Non-array case (array of 1 value per element)
    {
        HdVtBufferSource b(HdTokens->points, v);
        std::cout << b << std::endl;

        TF_VERIFY(b.GetTupleType().type == HdGetValueTupleType(v).type);
        TF_VERIFY(b.GetTupleType().count == 1);
        TF_VERIFY(b.GetNumElements() == vtArray.size());
        TF_VERIFY(HdGetComponentCount(b.GetTupleType().type)
                  == numComponents);
        TF_VERIFY(HdDataSizeOfType(b.GetTupleType().type) == sizeof(ELT));

        for (size_t i = 0; i < length; i++) {
            TF_VERIFY(vtArray[i] == 
                      ((ELT*)b.GetData())[i]);
        }
        std::cout << "\n";
    }

    // Array case (array of N values per element)
    {
        HdVtBufferSource b(HdTokens->points, v, length);
        std::cout << b << std::endl;

        TF_VERIFY(b.GetTupleType() == HdGetValueTupleType(v));
        TF_VERIFY(b.GetNumElements() == 1);
        TF_VERIFY(HdGetComponentCount(b.GetTupleType().type)
                  == numComponents);
        TF_VERIFY(HdDataSizeOfType(b.GetTupleType().type) == sizeof(ELT));

        for (size_t i = 0; i < length; i++) {
            TF_VERIFY(vtArray[i] == 
                      ((ELT*)b.GetData())[i]);
        }
        std::cout << "\n";
    }
}

template <typename DELT, typename FELT>
void
BasicDoubleTest(size_t length, size_t numComponents)
{
    std::cout << "[ " << TfType::Find<DELT>().GetTypeName() << " ]";
    std::cout << "------------------------------------------------------\n";
    VtArray<DELT> vtArrayDbl(length);
    VtArray<FELT> vtArrayFlt(length);
    for (size_t i = 0; i < length; i++) {
        vtArrayDbl[i] = DELT(i);
        vtArrayFlt[i] = FELT(i);
    }
    std::cout << vtArrayDbl << std::endl;
    std::cout << "Source bytes: " << vtArrayDbl.size() * sizeof(DELT)
              << std::endl;

    VtValue vDbl(vtArrayDbl);
    VtValue vFlt(vtArrayFlt);

    // Non-array case (array of 1 value per element)
    {
        // Doubles can be used for the buffer source
        {
            bool const allowDoubles = true;
            HdVtBufferSource b(HdTokens->points, vDbl, 1, allowDoubles);
            std::cout << "Double -> Double\n" << b << std::endl;

            TF_VERIFY(b.GetTupleType().type == HdGetValueTupleType(vDbl).type);
            TF_VERIFY(b.GetTupleType().count == 1);
            TF_VERIFY(b.GetNumElements() == vtArrayDbl.size());
            TF_VERIFY(HdGetComponentCount(b.GetTupleType().type)
                    == numComponents);
            TF_VERIFY(HdDataSizeOfType(b.GetTupleType().type) == sizeof(DELT));

            for (size_t i = 0; i < length; i++) {
                TF_VERIFY(vtArrayDbl[i] == 
                        ((DELT*)b.GetData())[i]);
            }
            std::cout << "\n";
        }

        // Doubles must be converted to floats for the buffer source
        {
            bool const allowDoubles = false;
            HdVtBufferSource b(HdTokens->points, vDbl, 1, allowDoubles);
            std::cout << "Double -> Float\n" << b << std::endl;

            TF_VERIFY(b.GetTupleType().type == HdGetValueTupleType(vFlt).type);
            TF_VERIFY(b.GetTupleType().count == 1);
            TF_VERIFY(b.GetNumElements() == vtArrayFlt.size());
            TF_VERIFY(HdGetComponentCount(b.GetTupleType().type)
                    == numComponents);
            TF_VERIFY(HdDataSizeOfType(b.GetTupleType().type) == sizeof(FELT));

            for (size_t i = 0; i < length; i++) {
                TF_VERIFY(vtArrayFlt[i] == 
                        ((FELT*)b.GetData())[i]);
            }
            std::cout << "\n";
        }
    }

    // Array case (array of N values per element)
    {
        // Doubles can be used for the buffer source
        {
            bool const allowDoubles = true;
            HdVtBufferSource b(HdTokens->points, vDbl, length, allowDoubles);
            std::cout << "Double -> Double\n" << b << std::endl;

            TF_VERIFY(b.GetTupleType() == HdGetValueTupleType(vDbl));
            TF_VERIFY(b.GetNumElements() == 1);
            TF_VERIFY(HdGetComponentCount(b.GetTupleType().type)
                    == numComponents);
            TF_VERIFY(HdDataSizeOfType(b.GetTupleType().type) == sizeof(DELT));

            for (size_t i = 0; i < length; i++) {
                TF_VERIFY(vtArrayDbl[i] == 
                        ((DELT*)b.GetData())[i]);
            }
            std::cout << "\n";
        }

        // Doubles must be converted to floats for the buffer source
        {
            bool const allowDoubles = false;
            HdVtBufferSource b(HdTokens->points, vDbl, length, allowDoubles);
            std::cout << "Double -> Float\n" << b << std::endl;

            TF_VERIFY(b.GetTupleType() == HdGetValueTupleType(vFlt));
            TF_VERIFY(b.GetNumElements() == 1);
            TF_VERIFY(HdGetComponentCount(b.GetTupleType().type)
                    == numComponents);
            TF_VERIFY(HdDataSizeOfType(b.GetTupleType().type) == sizeof(FELT));

            for (size_t i = 0; i < length; i++) {
                TF_VERIFY(vtArrayFlt[i] == 
                        ((FELT*)b.GetData())[i]);
            }
            std::cout << "\n";
        }
    }
}

template <typename T>
void
MatrixTest()
{
    std::cout << TfType::Find<T>().GetTypeName();
    std::cout << " to float matrix ---------------------------------------\n";

    T value(1);
    std::cout << value << std::endl;

    HdVtBufferSource b(HdTokens->points, value);
    std::cout << b << std::endl;

    TF_VERIFY(b.GetTupleType().type == HdTypeFloatMat4);
    TF_VERIFY(HdGetComponentCount(b.GetTupleType().type) == 16);
    TF_VERIFY(HdGetComponentType(b.GetTupleType().type) == HdTypeFloat);
    TF_VERIFY(HdDataSizeOfTupleType(b.GetTupleType()) == sizeof(GfMatrix4f));
    TF_VERIFY(b.GetNumElements() == 1);

    std::cout << "\n";
}

template <typename ELT>
void
MatrixArrayTest(size_t length)
{
    std::cout << "[ " << TfType::Find<ELT>().GetTypeName() << " ]";
    std::cout << " to float matrix ---------------------------------------\n";
    VtArray<ELT> vtArray(length);
    for (size_t i = 0; i < length; i++) {
        vtArray[i] = ELT(i);
    }

    std::cout << vtArray << std::endl;

    HdVtBufferSource b(HdTokens->points, vtArray);
    std::cout << b << std::endl;

    TF_VERIFY(b.GetTupleType().type == HdTypeFloatMat4);
    TF_VERIFY(HdGetComponentCount(b.GetTupleType().type) == 16);
    TF_VERIFY(HdGetComponentType(b.GetTupleType().type) == HdTypeFloat);
    TF_VERIFY(HdDataSizeOfType(b.GetTupleType().type) == sizeof(GfMatrix4f));
    TF_VERIFY(HdDataSizeOfTupleType(b.GetTupleType()) == sizeof(GfMatrix4f));
    TF_VERIFY(b.GetNumElements() == length);

    std::cout << "\n";
}

int main()
{
    TfErrorMark mark;

    // non-array
    BasicTest<GfVec2f>(2);
    BasicTest<GfVec3f>(3);
    BasicTest<GfVec4f>(4);
    BasicDoubleTest<GfVec2d, GfVec2f>(2);
    BasicDoubleTest<GfVec3d, GfVec3f>(3);
    BasicDoubleTest<GfVec4d, GfVec4f>(4);
    BasicTest<GfMatrix4f>(16);
    BasicDoubleTest<GfMatrix4d, GfMatrix4f>(16);

    // array
    BasicTest<int>(10, 1);
    BasicTest<float>(10, 1);
    BasicDoubleTest<double, float>(10, 1);

    BasicTest<GfVec2i>(10, 2);
    BasicTest<GfVec3i>(10, 3);
    BasicTest<GfVec4i>(10, 4);

    BasicTest<GfVec2f>(10, 2);
    BasicTest<GfVec3f>(10, 3);
    BasicTest<GfVec4f>(10, 4);

    BasicDoubleTest<GfVec2d, GfVec2f>(10, 2);
    BasicDoubleTest<GfVec3d, GfVec3f>(10, 3);
    BasicDoubleTest<GfVec4d, GfVec4f>(10, 4);

    BasicTest<GfMatrix4f>(10, 16);
    BasicDoubleTest<GfMatrix4d, GfMatrix4f>(10, 16);

    // double to float matrix type conversion
    MatrixTest<GfMatrix4d>();
    MatrixArrayTest<GfMatrix4d>(10);

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

