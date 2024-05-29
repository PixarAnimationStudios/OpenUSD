//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/pathUtils.h"

#include "resolverContext.h"

PXR_NAMESPACE_USING_DIRECTIVE

UsdResolverExampleResolverContext::UsdResolverExampleResolverContext()
    = default;

UsdResolverExampleResolverContext::UsdResolverExampleResolverContext(
    const UsdResolverExampleResolverContext&) = default;

UsdResolverExampleResolverContext::UsdResolverExampleResolverContext(
    const std::string& mappingFile)
    : _mappingFile(TfAbsPath(mappingFile))
{
}

bool
UsdResolverExampleResolverContext::operator<(
    const UsdResolverExampleResolverContext& rhs) const
{
    return _mappingFile < rhs._mappingFile;
}

bool
UsdResolverExampleResolverContext::operator==(
    const UsdResolverExampleResolverContext& rhs) const
{
    return _mappingFile == rhs._mappingFile;
}
    
size_t hash_value(const UsdResolverExampleResolverContext& ctx)
{
    return TfHash()(ctx._mappingFile);
}

const std::string& 
UsdResolverExampleResolverContext::GetMappingFile() const
{
    return _mappingFile;
}


