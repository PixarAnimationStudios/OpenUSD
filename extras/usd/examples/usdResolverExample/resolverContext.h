//
// Copyright 2021 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
