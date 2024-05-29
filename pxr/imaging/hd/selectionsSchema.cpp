//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/selectionsSchema.h"

#include "pxr/imaging/hd/selectionSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdSelectionsSchemaTokens,
    HDSELECTIONS_SCHEMA_TOKENS);

/*static*/
HdSelectionsSchema
HdSelectionsSchema::GetFromParent(
        const HdContainerDataSourceHandle &fromParentContainer)
{
    return HdSelectionsSchema(
        fromParentContainer
        ? HdVectorDataSource::Cast(fromParentContainer->Get(
                HdSelectionsSchemaTokens->selections))
        : nullptr);
}

/*static*/
const TfToken &
HdSelectionsSchema::GetSchemaToken()
{
    return HdSelectionsSchemaTokens->selections;
}

/*static*/
const HdDataSourceLocator &
HdSelectionsSchema::GetDefaultLocator()
{
    static const HdDataSourceLocator locator(
        HdSelectionsSchemaTokens->selections
    );
    return locator;
} 
PXR_NAMESPACE_CLOSE_SCOPE
