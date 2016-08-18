#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/tokens.h"
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

template <typename T>
void
BasicTest(short numComponents)
{
    std::cout << TfType::Find<T>().GetTypeName();
    std::cout << "------------------------------------------------------\n";
    T value(1);
    std::cout << value << std::endl;

    VtValue v(value);
    HdVtBufferSource b(HdTokens->points, v);
    std::cout << b << std::endl;

    TF_VERIFY(b.GetSize() == sizeof(T));
    TF_VERIFY(b.GetNumComponents() == numComponents);
    TF_VERIFY(b.GetElementSize() == sizeof(T));
    TF_VERIFY(b.GetComponentSize()*b.GetNumComponents() == sizeof(T));
    TF_VERIFY(b.GetNumElements() == 1);

    TF_VERIFY(*(T*)b.GetData() == value);
    std::cout << "\n";
}

template <typename ELT>
void
BasicTest(size_t length, short numComponents)
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
    HdVtBufferSource b(HdTokens->points, v);
    std::cout << b << std::endl;

    TF_VERIFY(b.GetSize() == vtArray.size() * sizeof(ELT));
    TF_VERIFY(b.GetNumComponents() == numComponents);
    TF_VERIFY(b.GetElementSize() == sizeof(ELT));
    TF_VERIFY(b.GetComponentSize()*b.GetNumComponents() == sizeof(ELT));
    TF_VERIFY(b.GetNumElements() == vtArray.size());

    for (size_t i = 0; i < length; i++) {
        TF_VERIFY(vtArray[i] == 
                  ((ELT*)b.GetData())[i]);
    }
    std::cout << "\n";
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

    TF_VERIFY(b.GetSize() == sizeof(GfMatrix4f));
    TF_VERIFY(b.GetNumComponents() == 16);
    TF_VERIFY(b.GetElementSize() == sizeof(GfMatrix4f));
    TF_VERIFY(b.GetComponentSize()*b.GetNumComponents() == sizeof(GfMatrix4f));
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

    TF_VERIFY(b.GetSize() == sizeof(GfMatrix4f)*length);
    TF_VERIFY(b.GetNumComponents() == 16);
    TF_VERIFY(b.GetElementSize() == sizeof(GfMatrix4f));
    TF_VERIFY(b.GetComponentSize()*b.GetNumComponents() == sizeof(GfMatrix4f));
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
    BasicTest<GfVec2d>(2);
    BasicTest<GfVec3d>(3);
    BasicTest<GfVec4d>(4);
    BasicTest<GfMatrix4f>(16);
    BasicTest<GfMatrix4d>(16);

    // array
    BasicTest<int>(10, 1);
    BasicTest<float>(10, 1);
    BasicTest<double>(10, 1);

    BasicTest<GfVec2i>(10, 2);
    BasicTest<GfVec3i>(10, 3);
    BasicTest<GfVec4i>(10, 4);

    BasicTest<GfVec2f>(10, 2);
    BasicTest<GfVec3f>(10, 3);
    BasicTest<GfVec4f>(10, 4);

    BasicTest<GfVec2d>(10, 2);
    BasicTest<GfVec3d>(10, 3);
    BasicTest<GfVec4d>(10, 4);

    BasicTest<GfMatrix4f>(10, 16);
    BasicTest<GfMatrix4d>(10, 16);

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

