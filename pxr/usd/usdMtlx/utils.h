//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_USDMTLX_UTILS_H
#define PXR_USD_USDMTLX_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usdMtlx/api.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/value.h"
#include <MaterialXCore/Document.h>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Return the MaterialX standard library paths.  All standard library
/// files (and only standard library files) should be found on these
/// paths.
USDMTLX_API
const SdrStringVec&
UsdMtlxStandardLibraryPaths();

/// Return the paths to directories containing custom MaterialX files, set in 
/// the environment variable 'PXR_MTLX_PLUGIN_SEARCH_PATHS'
USDMTLX_API
const SdrStringVec&
UsdMtlxCustomSearchPaths();

/// Return the MaterialX search paths. In order, this includes:
/// - directories containing custom MaterialX files set in the env var
///   'PXR_MTLX_PLUGIN_SEARCH_PATHS'
/// - standard library paths set in the env var 'PXR_MTLX_STDLIB_SEARCH_PATHS'
/// - path to the MaterialX standard library discovered at build time.
USDMTLX_API
const SdrStringVec&
UsdMtlxSearchPaths();

/// Return the MaterialX standard file extensions.
USDMTLX_API
SdrStringVec
UsdMtlxStandardFileExtensions();

/// Return the MaterialX document at \p resolvedPath.  Return null if the
/// document could not be read and report a runtime error.
///
/// Unlike UsdMtlxGetDocument, this function does not implement any
/// caching or special behavior for MaterialX standard library documents.
USDMTLX_API
MaterialX::DocumentPtr
UsdMtlxReadDocument(const std::string& resolvedPath);

/// Return the (possibly cached) MaterialX document at \p resolvedUri.
/// Return null if the document could not be read and report a
/// warning (once per uri).  \p resolvedUri may be empty to indicate
/// the MaterialX standard library documents all rolled into one.
USDMTLX_API
MaterialX::ConstDocumentPtr
UsdMtlxGetDocument(const std::string& resolvedUri);

/// Returns the (possibly cached) MaterialX document created from the given 
/// string containing the source MaterialX XML.
USDMTLX_API
MaterialX::ConstDocumentPtr 
UsdMtlxGetDocumentFromString(const std::string &mtlxXml);

// Return the version of the mtlx element.  If the version cannot be
// found then return an invalid default version.  If implicitDefault
// isn't null then we do to two things:  we set implicitDefault to
// false iff the isdefaultversion attribute exists and isn't empty,
// otherwise we set it to true;  and we return the version as a
// default if isdefaultversion exists and is set to "true".
USDMTLX_API
SdrVersion
UsdMtlxGetVersion(const MaterialX::ConstInterfaceElementPtr& mtlx,
                  bool* implicitDefault = nullptr);

/// Return the source URI for a MaterialX element.  If the element
/// doesn't have a non-empty URI then return the source URI of the
/// closest element up the element hierarchy that does have one.
/// Return the empty string if no element has a source URI.
USDMTLX_API
const std::string&
UsdMtlxGetSourceURI(const MaterialX::ConstElementPtr& element);

/// Result of \c UsdMtlxGetUsdType().
struct UsdMtlxUsdTypeInfo {
    UsdMtlxUsdTypeInfo(
        SdfValueTypeName valueTypeName,
        bool valueTypeNameIsExact,
        TfToken shaderPropertyType,
        int arraySize=0)
        : valueTypeName(valueTypeName)
        , shaderPropertyType(shaderPropertyType)
        , arraySize(arraySize)
        , valueTypeNameIsExact(valueTypeNameIsExact)
    { }

    /// The value type name that most closely matches the MaterialX type.
    /// If the type isn't recognized this is the invalid value type name.
    /// Clients can check for array types by calling \c IsArray() on this.
    SdfValueTypeName valueTypeName;

    /// The exact \c SdrShaderProperty type name.  If there is no exact
    /// match this is empty.
    TfToken shaderPropertyType;

    /// If the value type is a fixed-size array/tuple, this will be greater
    /// then zero.  For "dynamic arrays" this will be zero.
    int arraySize;

    /// \c true iff the value type name is an exact match to the
    /// MaterialX type.
    bool valueTypeNameIsExact;
};

/// Convert a (standard) MaterialX type name.
USDMTLX_API
UsdMtlxUsdTypeInfo
UsdMtlxGetUsdType(const std::string& mtlxTypeName);

/// Return the value in \p mtlx as a \c VtValue.  Returns an
/// empty VtValue and reports an error if the conversion cannot be
/// applied.  If \p getDefaultValue is \c true then converts the
/// default value.  It is not an error if the value doesn't exist;
/// that silently returns an empty VtValue.
USDMTLX_API
VtValue
UsdMtlxGetUsdValue(const MaterialX::ConstElementPtr& mtlx,
                   bool getDefaultValue = false);

/// Return the MaterialX values in \p values assuming it contains an
/// array of values of MaterialX type \p type as a vector of VtValue.
USDMTLX_API
std::vector<VtValue>
UsdMtlxGetPackedUsdValues(const std::string& values, const std::string& type);

/// Split a MaterialX string array into a vector of strings.
///
/// The MaterialX specification says:
///
/// > Individual string values within stringarrays may not contain
/// > commas or semicolons, and any leading and trailing whitespace
/// > characters in them is ignored.
///
/// These restrictions do not apply to the string type.
USDMTLX_API
std::vector<std::string>
UsdMtlxSplitStringArray(const std::string& s);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USDMTLX_UTILS_H
