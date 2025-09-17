//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_USING_DIRECTIVE

class MockData final : public SdfData 
{
public:
    bool
    GetPreviousTimeSampleForPath(
        const SdfPath& path, double time, double* tPrevious) const override
    {
        return this->SdfAbstractData::GetPreviousTimeSampleForPath(
            path, time, tPrevious);
    }

    MockData() {
        CreateSpec(SdfPath("/Prim.attr"), SdfSpecTypeAttribute);
        SetTimeSample(SdfPath("/Prim.attr"), 1.0, VtValue(1));
        SetTimeSample(SdfPath("/Prim.attr"), 2.0, VtValue(2));
        SetTimeSample(SdfPath("/Prim.attr"), 3.0, VtValue(3));
    }
};

int 
main(int argc, char** argv)
{
    MockData mockData;
    // No previous time sample before first time sample.
    double tPrevious = 0.0;
    TF_AXIOM(!mockData.GetPreviousTimeSampleForPath(
        SdfPath("/Prim.attr"), 0.5, &tPrevious));
    // No previous time sample at first time sample.
    TF_AXIOM(!mockData.GetPreviousTimeSampleForPath(
        SdfPath("/Prim.attr"), 1.0, &tPrevious));
    // Previous time sample between 1st and 2nd time samples is 1st.
    tPrevious = 0.0;
    TF_AXIOM(mockData.GetPreviousTimeSampleForPath(
        SdfPath("/Prim.attr"), 1.5, &tPrevious));
    TF_AXIOM(tPrevious == 1.0);
    // Previous time sample at 2nd time sample is 1st.
    tPrevious = 0.0;
    TF_AXIOM(mockData.GetPreviousTimeSampleForPath(
        SdfPath("/Prim.attr"), 2.0, &tPrevious));
    TF_AXIOM(tPrevious == 1.0);
    // Previous time sample past the last time sample is the last time sample.
    tPrevious = 0.0;
    TF_AXIOM(mockData.GetPreviousTimeSampleForPath(
        SdfPath("/Prim.attr"), 10.0, &tPrevious));
    TF_AXIOM(tPrevious == 3.0);
    printf(">>> Test PASSED\n");
    return 0;
}
