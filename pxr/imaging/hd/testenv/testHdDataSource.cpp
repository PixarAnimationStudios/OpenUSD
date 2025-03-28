//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <iostream>
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialInterfaceMappingSchema.h"

#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

//-----------------------------------------------------------------------------

bool TestRetainedDataSource()
{
    float inputValue = 5.0f;
    HdRetainedTypedSampledDataSource<float> taco(inputValue);
    std::cout << taco.GetTypedValue(inputValue) << std::endl;

    if (taco.GetTypedValue(inputValue) != inputValue) {
        std::cerr << "input doesn't match output." << std::endl;
        return false;
    }

    return true;
}


#define COMPARECONTAINERSIZE(T, C, S) \
    std::cout << T << ": "; \
    if (auto c = HdRetainedContainerDataSource::Cast(C)) { \
        size_t s = c->GetNames().size(); \
        if (s == S) { \
            std::cout << " matches " << s << std::endl; \
        } else { \
            std::cout << " doesn't match." << std::endl; \
            std::cout << "expected: " << S  << " but got " << s << std::endl; \
            return false; \
        } \
    } else { \
        std::cout << "not a container." << std::endl; \
        return false; \
    }


bool TestRetainedContainerDataSource()
{
    // test 1-6 sized constructors and static Get (via locator)

    auto leaf = HdRetainedTypedSampledDataSource<int>::New(0);
    auto root = HdRetainedContainerDataSource::New( // 1
        TfToken("a"), HdRetainedContainerDataSource::New( // 2
            TfToken("b"), HdRetainedContainerDataSource::New( // 3
                TfToken("c"), HdRetainedContainerDataSource::New( // 4
                    TfToken("d"), HdRetainedContainerDataSource::New( // 5
                        TfToken("e"), HdRetainedContainerDataSource::New( // 6
                            TfToken("y"), leaf,
                            TfToken("x"), leaf,
                            TfToken("w"), leaf,
                            TfToken("v"), leaf,
                            TfToken("u"), leaf,
                            TfToken("t"), leaf
                        ),
                        TfToken("y"), leaf,
                        TfToken("x"), leaf,
                        TfToken("w"), leaf,
                        TfToken("v"), leaf
                    ),
                    TfToken("y"), leaf,
                    TfToken("x"), leaf,
                    TfToken("w"), leaf
                ),
                TfToken("y"), leaf,
                TfToken("x"), leaf
            ),
            TfToken("z"), leaf
        )
    );

    HdDataSourceLocator locator;

    COMPARECONTAINERSIZE("<empty>",
            HdContainerDataSource::Get(root, locator), 1);

    COMPARECONTAINERSIZE("a",
            HdContainerDataSource::Get(root,
                    locator = locator.Append(TfToken("a"))), 2);

    COMPARECONTAINERSIZE("a.b",
            HdContainerDataSource::Get(root,
                    locator = locator.Append(TfToken("b"))), 3);

    COMPARECONTAINERSIZE("a.b.c",
            HdContainerDataSource::Get(root,
                    locator = locator.Append(TfToken("c"))), 4);

    COMPARECONTAINERSIZE("a.b.c.d",
            HdContainerDataSource::Get(root,
                    locator = locator.Append(TfToken("d"))), 5);

    COMPARECONTAINERSIZE("a.b.c.d.e",
            HdContainerDataSource::Get(root,
                    locator = locator.Append(TfToken("e"))), 6);

    return true;
}

HdContainerDataSourceHandle
_GetMeshPrimDataSource()
{
    return HdRetainedContainerDataSource::New(
        HdPrimvarsSchemaTokens->primvars,
        HdRetainedContainerDataSource::New(

            HdPrimvarsSchemaTokens->points,
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                        VtVec3fArray{
                            {0.5f, -0.5f, -0.5f},
                            {0.5f, -0.5f, 0.5f},
                            {-0.5f, -0.5f, 0.5f},
                            {-0.5f, 0.5f, -0.5f},
                            {0.5f, 0.5f, -0.5f},
                            {0.5f, 0.5f, 0.5f},
                            {-0.5f, 0.5f, 0.5f},
                        }))
                .SetInterpolation(
                    HdPrimvarSchema::BuildInterpolationDataSource(
                        HdPrimvarSchemaTokens->varying))
                .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                        HdPrimvarSchemaTokens->point))
                .Build(),
            HdTokens->displayColor,
            HdPrimvarSchema::Builder()
                .SetIndexedPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtVec3fArray>::New(
                        VtVec3fArray{
                            {1.0f, 0.0f, 0.0f},
                            {0.0f, 1.0f, 0.0f},
                            {0.0f, 0.0f, 1.0f},
                            {1.0f, 1.0f, 1.0f}
                        }))
                .SetIndices(
                    HdRetainedTypedSampledDataSource<VtIntArray>::New(
                        VtIntArray{
                            3, 3, 3, 3, 0, 1, 2, 3, 0, 1, 2, 3,
                            0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3
                        }
                    )
                )
                .SetInterpolation(
                    HdPrimvarSchema::BuildInterpolationDataSource(
                        HdPrimvarSchemaTokens->faceVarying))
                .SetRole(
                    HdPrimvarSchema::BuildRoleDataSource(
                        HdPrimvarSchemaTokens->color))
                .Build(),

            HdTokens->displayOpacity,
            HdPrimvarSchema::Builder()
                .SetPrimvarValue(
                    HdRetainedTypedSampledDataSource<VtFloatArray>::New(
                        VtFloatArray{
                            0.6f, 0.6f, 0.6f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f,
                            1.0f, 1.0f, 1.0f, 1.0f, 0.6f, 0.6f, 0.6f, 0.6f,
                            1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f
                        }))
                .SetInterpolation(
                    HdPrimvarSchema::BuildInterpolationDataSource(
                        HdPrimvarSchemaTokens->faceVarying))
                .Build()
        ),

        HdMeshSchemaTokens->mesh,
        HdMeshSchema::Builder()
            .SetTopology(HdMeshTopologySchema::Builder()
                .SetFaceVertexCounts(
                    HdRetainedTypedSampledDataSource<VtIntArray>::New(
                        VtIntArray{4, 4, 4, 4, 4, 4}))
                .SetFaceVertexIndices(
                    HdRetainedTypedSampledDataSource<VtIntArray>::New(
                        VtIntArray{1, 5, 4, 0, 2, 6, 5, 1, 3, 7, 6, 2, 0, 4, 7,
                            3, 2, 1, 0, 3, 5, 6, 7, 4}))
                .SetOrientation(
                    HdMeshTopologySchema::BuildOrientationDataSource(
                        HdMeshTopologySchemaTokens->leftHanded))
                .Build())
            .Build(),

        HdXformSchemaTokens->xform,
        HdXformSchema::Builder()
            .SetMatrix(HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                GfMatrix4d(1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                10.0, 20.0, 30.0, 1.0
                )))
            .Build()
    );

}


bool TestMeshTopologySchema()
{
    HdMeshSchema hdMesh(
        HdMeshSchema::GetFromParent(_GetMeshPrimDataSource()));

    if (!hdMesh.IsDefined()) {
        std::cerr << "couldn't get mesh" << std::endl;
        return false;
    }

    HdMeshTopologySchema hdMeshTopology(
        HdMeshTopologySchema::GetFromParent(hdMesh.GetContainer()));

    if (!hdMeshTopology.IsDefined()) {
        std::cerr << "couldn't get meshTopology" << std::endl;
        return false;
    }

    if (HdIntArrayDataSourceHandle faceVertexCounts =
            hdMeshTopology.GetFaceVertexCounts()) {
        std::cout << "faceVertexCounts: ";
        std::cout << faceVertexCounts->GetTypedValue(0.0f) << std::endl;
    } else {
        std::cerr << "couldn't get faceVertexCounts" << std::endl;
        return false;
    }

    if (HdIntArrayDataSourceHandle faceVertexIndices =
            hdMeshTopology.GetFaceVertexIndices()) {
        std::cout << "faceVertexIndices: ";
        std::cout << faceVertexIndices->GetTypedValue(0.0f) << std::endl;
    } else {
        std::cerr << "couldn't get faceVertexIndices" << std::endl;
        return false;
    }

    return true;
}

bool TestXformSchema()
{
    HdXformSchema hdXform(HdXformSchema::GetFromParent(
        _GetMeshPrimDataSource()));

    if (HdMatrixDataSourceHandle matrixSource = hdXform.GetMatrix()) {

        std::cout << matrixSource->GetTypedValue(0.0f) << std::endl;

    } else {
        std::cerr << "couldn't retrieve matrix data source" << std::endl;
        return false;
    }


    return true;
}

bool TestRetainedSmallVectorDataSource()
{
    static HdDataSourceBaseHandle values[] = {
        HdRetainedTypedSampledDataSource<int>::New(1),
        HdRetainedTypedSampledDataSource<int>::New(10),
        HdRetainedTypedSampledDataSource<int>::New(20)
    };

    HdRetainedSmallVectorDataSource::Handle v =
        HdRetainedSmallVectorDataSource::New(3, values);
    if (v->GetNumElements() != 3) {
        std::cerr << "vector size doesn't match." << std::endl;
        return false;
    }

    HdDataSourceBaseHandle v0 = v->GetElement(0);
    HdDataSourceBaseHandle v1 = v->GetElement(1);
    HdDataSourceBaseHandle v2 = v->GetElement(2);
    HdSampledDataSource::Time t = 0.0;
    if (HdRetainedTypedSampledDataSource<int>::Cast(v0)->GetTypedValue(t) !=1 ||
        HdRetainedTypedSampledDataSource<int>::Cast(v1)->GetTypedValue(t) !=10||
        HdRetainedTypedSampledDataSource<int>::Cast(v2)->GetTypedValue(t) !=20
        ) {
        std::cerr << "vector values don't match." << std::endl;
        return false;
    }

    HdDataSourceBaseHandle v3 = v->GetElement(3);
    HdRetainedTypedSampledDataSource<int>::Handle i3 = 
        HdRetainedTypedSampledDataSource<int>::Cast(v3);
    if (i3) {
        std::cerr << "vector behavior doesn't match" << std::endl;
        return false;
    }

    return true;
}

bool TestPrimvarSchema()
{
    HdPrimvarsSchema primvars(
        HdPrimvarsSchema::GetFromParent(_GetMeshPrimDataSource()));

    std::cout << "Primvars contained: ";
    TfTokenVector primvarNames = primvars.GetPrimvarNames();
    for (size_t i = 0; i < primvarNames.size(); ++i) {
        std::cout << primvarNames[i].GetText() << std::endl;
    }

    HdPrimvarSchema displayOpacity = 
        primvars.GetPrimvar(HdTokens->displayOpacity);
        
    if (displayOpacity.IsDefined()) {
        
        if (displayOpacity.IsIndexed()) {
            std::cerr << "displayOpacity primvar should not be indexed" 
                << std::endl;
            return false;
        }

        VtFloatArray expectedPvValue = {
            0.6f, 0.6f, 0.6f, 0.6f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f,
            0.6f, 0.6f, 0.6f, 0.6f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f };
        VtValue pvValue;

        if (HdSampledDataSourceHandle primvarValue = 
                displayOpacity.GetPrimvarValue()) {
            pvValue = primvarValue->GetValue(0.0f);
            if (pvValue != expectedPvValue) {
                std::cerr << "Unexpected displayOpacity primvar value" 
                    << std::endl;
                return false;
            }
        } else {
            std::cerr << "Couldn't get displayOpacity primvar value" 
                << std::endl;
            return false;
        }

        // Would normally probably not want to call GetIndexedPrimvarValue() 
        // since IsIndexed() returned false.
        if (HdSampledDataSourceHandle primvarValue = 
                displayOpacity.GetIndexedPrimvarValue()) {
            VtValue indexedPvValue = primvarValue->GetValue(0.0f);
            if (indexedPvValue != expectedPvValue) {
                std::cerr << "Unexpected displayOpacity indexed primvar value" 
                    << std::endl;
                return false;
            }

        } else {
            std::cerr << "Couldn't get displayOpacity indexed primvar value" 
                << std::endl;
            return false;
        }

    } else {
        std::cerr << "Couldn't get displayOpacity primvar schema" << std::endl;
        return false;
    }

    
    HdPrimvarSchema displayColor = primvars.GetPrimvar(HdTokens->displayColor);
    if (displayColor.IsDefined()) {
        if (!displayColor.IsIndexed()) {
            std::cerr << "displayColor primvar should be indexed" << std::endl;
            return false;
        }

        if (HdSampledDataSourceHandle primvarValue = 
                displayColor.GetIndexedPrimvarValue()) {
            VtValue indexedPvValue = primvarValue->GetValue(0.0f);
            VtVec3fArray expectedPvValue = { GfVec3f(1, 0, 0),
                GfVec3f(0, 1, 0), GfVec3f(0, 0, 1), GfVec3f(1, 1, 1) };
            if (indexedPvValue != expectedPvValue) {
                std::cerr << "Unexpected displayColor primvar value" 
                    << std::endl;
                return false;
            }
        } else {
            std::cerr << "Couldn't get displayColor indexed primvar value" 
                << std::endl;
            return false;
        }

        if (HdTypedSampledDataSource<VtIntArray>::Handle primvarIndices = 
                displayColor.GetIndices()) {
            VtIntArray indices = primvarIndices->GetTypedValue(0.0f);
            VtIntArray expectedIndices = { 3, 3, 3, 3, 0, 1, 2, 3, 0, 1, 2, 3,
                0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3 };
            if (indices != expectedIndices) {
                std::cerr << "Unexpcted displayColor indices" << std::endl;
                return false;
            }
        } else {
            std::cerr << "Couldn't get displayColor indices" << std::endl;
            return false;
        }

        VtValue pvValue;
        if (HdSampledDataSourceHandle primvarValue = 
                displayColor.GetPrimvarValue()) {
            VtValue pvValue = primvarValue->GetValue(0.0f);
            VtVec3fArray expectedFlattenedValue = { GfVec3f(1, 1, 1), 
                GfVec3f(1, 1, 1), GfVec3f(1, 1, 1), GfVec3f(1, 1, 1), 
                GfVec3f(1, 0, 0), GfVec3f(0, 1, 0), GfVec3f(0, 0, 1), 
                GfVec3f(1, 1, 1), GfVec3f(1, 0, 0), GfVec3f(0, 1, 0), 
                GfVec3f(0, 0, 1), GfVec3f(1, 1, 1), GfVec3f(1, 0, 0),
                GfVec3f(0, 1, 0), GfVec3f(0, 0, 1), GfVec3f(1, 1, 1),
                GfVec3f(1, 0, 0), GfVec3f(0, 1, 0), GfVec3f(0, 0, 1), 
                GfVec3f(1, 1, 1), GfVec3f(1, 0, 0), GfVec3f(0, 1, 0), 
                GfVec3f(0, 0, 1), GfVec3f(1, 1, 1) };
            if (pvValue != expectedFlattenedValue) {
                std::cerr << "Unexpected displayColor flattened primvar value" 
                    << std::endl;
                return false;
            }
        } else {
            std::cerr << "Couldn't get displayColor primvar value" << std::endl;
            return false;
        }

    } else {
        std::cerr << "Couldn't get displayColor primvar schema" << std::endl;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

// HdRetainedTypedSampledDataSource<bool>::New is specialized to return only
// shared instances of static true or false values
bool TestSpecializedNew()
{
    HdBoolDataSourceHandle t2 =
        HdRetainedTypedSampledDataSource<bool>::New(true);
    HdBoolDataSourceHandle t1 =
        HdRetainedTypedSampledDataSource<bool>::New(true);

    HdBoolDataSourceHandle f1 =
        HdRetainedTypedSampledDataSource<bool>::New(false);
    HdBoolDataSourceHandle f2 =
        HdRetainedTypedSampledDataSource<bool>::New(false);


    if (t1.get() == t2.get() && f1.get() == f2.get()) {
        return true;
    }

    std::cerr << "static bool pointers don't match" << std::endl;
    return false;
}

//-----------------------------------------------------------------------------

bool TestContainerSchemas()
{
    HdContainerDataSourceHandle c1 = HdRetainedContainerDataSource::New(
        TfToken("a"), HdRetainedTypedSampledDataSource<int>::New(1),
        TfToken("b"), HdRetainedTypedSampledDataSource<int>::New(2),
        TfToken("c"), HdRetainedTypedSampledDataSource<float>::New(3.0f));

    HdTypedContainerSchema<HdIntDataSource> s1(c1);

    if (!s1.Get(TfToken("a")) || !s1.Get(TfToken("b"))) {
        std::cerr << "expected int data source result" << std::endl;
        return false;
    }

    if (s1.Get(TfToken("c"))) {
        std::cerr << "unexpected data source result from float" << std::endl;
        return false;
    }

    auto _TDS = [](const char *v) {
        return HdRetainedTypedSampledDataSource<TfToken>::New(TfToken(v));
    };

    HdDataSourceBaseHandle h1[] = {
        HdMaterialInterfaceMappingSchema::Builder()
            .SetNodePath(_TDS("A"))
            .SetInputName(_TDS("x"))
            .Build(),
        HdMaterialInterfaceMappingSchema::Builder()
            .SetNodePath(_TDS("B"))
            .SetInputName(_TDS("y"))
            .Build(),
    };

    HdContainerDataSourceHandle c2 = HdRetainedContainerDataSource::New(
        TfToken("Q"), HdRetainedSmallVectorDataSource::New(2, h1));

    HdMaterialInterfaceMappingsContainerSchema mappings(c2);
    HdTokenDataSourceHandle t =
        mappings.Get(TfToken("Q")).GetElement(1).GetNodePath();

    if (!t) {
        std::cerr << "expected token data source for mapping node path"
            << std::endl;
        return false;
    }

    if (t->GetTypedValue(0.0f) != TfToken("B")) {
        std::cerr << "unexpected value for mapping node path" << std::endl;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

#define xstr(s) str(s)
#define str(s) #s
#define TEST(X) std::cout << (++i) << ") " <<  str(X) << "..." << std::endl; \
if (!X()) { std::cout << "FAILED" << std::endl; return -1; } \
else std::cout << "...SUCCEEDED" << std::endl;


int main(int argc, char**argv)
{
    std::cout << "STARTING testHdValue" << std::endl;
    // ------------------------------------------------------------------------

    int i = 0;
    TEST(TestRetainedDataSource);
    TEST(TestRetainedContainerDataSource);
    TEST(TestRetainedSmallVectorDataSource);
    TEST(TestMeshTopologySchema);
    TEST(TestXformSchema);
    TEST(TestPrimvarSchema);
    TEST(TestSpecializedNew);
    TEST(TestContainerSchemas);

    // ------------------------------------------------------------------------
    std::cout << "DONE testHdValue: SUCCESS" << std::endl;
    return 0;
}
