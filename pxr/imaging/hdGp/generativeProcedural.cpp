//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "generativeProcedural.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdGpGenerativeProceduralTokens,
    HDGPGENERATIVEPROCEDURAL_TOKENS);

HdGpGenerativeProcedural::HdGpGenerativeProcedural(
    const SdfPath &proceduralPrimPath)
: _proceduralPrimPath(proceduralPrimPath)
{
}

HdGpGenerativeProcedural::~HdGpGenerativeProcedural() = default;

const SdfPath &
HdGpGenerativeProcedural::_GetProceduralPrimPath()
{
    return _proceduralPrimPath;
}


/*static*/

const HdDataSourceLocator &
HdGpGenerativeProcedural::GetChildNamesDependencyKey()
{
    static const HdDataSourceLocator loc(TfToken("__childNames"));
    return loc;
}

bool
HdGpGenerativeProcedural::AsyncBegin(bool asyncEnabled)
{
    return false;
}

HdGpGenerativeProcedural::AsyncState
HdGpGenerativeProcedural::AsyncUpdate(
    const ChildPrimTypeMap &previousResult,
    ChildPrimTypeMap *outputPrimTypes,
    HdSceneIndexObserver::DirtiedPrimEntries *outputDirtiedPrims)
{
    return Finished;
}

PXR_NAMESPACE_CLOSE_SCOPE