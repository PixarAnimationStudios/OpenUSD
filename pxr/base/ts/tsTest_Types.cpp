//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_Types.h"

PXR_NAMESPACE_OPEN_SCOPE

TsTest_Sample::TsTest_Sample() = default;

TsTest_Sample::TsTest_Sample(
    double timeIn, double valueIn)
    : time(timeIn), value(valueIn) {}

TsTest_Sample::TsTest_Sample(
    const TsTest_Sample &other) = default;

TsTest_Sample&
TsTest_Sample::operator=(
    const TsTest_Sample &other) = default;

PXR_NAMESPACE_CLOSE_SCOPE
