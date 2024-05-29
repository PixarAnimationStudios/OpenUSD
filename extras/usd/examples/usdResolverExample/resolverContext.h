//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "api.h"

#include "pxr/usd/ar/defineResolverContext.h"
#include "pxr/usd/ar/resolverContext.h"

#include <string>

/// \class UsdResolverExampleResolverContext
///
/// Context object for the UsdResolverExampleResolver. This object
/// allows the client to specify a version mapping file to use for
/// {$VERSION} substitutions during asset resolution. See overview
/// for more details.
class UsdResolverExampleResolverContext
{
public:
    /// Create a context that specifies that the version mappings in
    /// \p mappingFile should be used when resolving asset paths with
    /// this context bound. \p mappingFile may be an absolute or
    /// relative file path; if relative, it will be anchored to the
    /// current working directory.
    USDRESOLVEREXAMPLE_API
    explicit UsdResolverExampleResolverContext(
        const std::string& mappingFile);

    USDRESOLVEREXAMPLE_API
    UsdResolverExampleResolverContext();

    USDRESOLVEREXAMPLE_API
    UsdResolverExampleResolverContext(
        const UsdResolverExampleResolverContext& rhs);

    USDRESOLVEREXAMPLE_API
    bool operator<(const UsdResolverExampleResolverContext& rhs) const;

    USDRESOLVEREXAMPLE_API
    bool operator==(const UsdResolverExampleResolverContext& rhs) const;
    
    USDRESOLVEREXAMPLE_API
    friend size_t hash_value(const UsdResolverExampleResolverContext& ctx);

    USDRESOLVEREXAMPLE_API
    const std::string& GetMappingFile() const;

private:
    std::string _mappingFile;
};

PXR_NAMESPACE_OPEN_SCOPE
AR_DECLARE_RESOLVER_CONTEXT(UsdResolverExampleResolverContext);
PXR_NAMESPACE_CLOSE_SCOPE
