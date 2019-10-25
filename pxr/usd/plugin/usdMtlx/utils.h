//
// Copyright 2018 Pixar
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

#ifndef USDMTLX_UTILS_H
#define USDMTLX_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usdMtlx/api.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/base/vt/value.h"
#include <MaterialXCore/Document.h>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Return the contents of a search path environment variable named
/// \p name as a vector of strings.  The path is split on the platform's
/// native path list separator.
USDMTLX_LOCAL
NdrStringVec
UsdMtlxGetSearchPathsFromEnvVar(const char* name);

/// Combines two search path lists.
USDMTLX_LOCAL
NdrStringVec
UsdMtlxMergeSearchPaths(const NdrStringVec& stronger,
                        const NdrStringVec& weaker);

/// Return the MaterialX standard library paths.  All standard library
/// files (and only standard library files) should be found on these
/// paths.
USDMTLX_LOCAL
NdrStringVec
UsdMtlxStandardLibraryPaths();

/// Return the MaterialX standard file extensions.
USDMTLX_LOCAL
NdrStringVec
UsdMtlxStandardFileExtensions();

/// Return the (possibly cached) MaterialX document at \p resolvedUri.
/// Return null if the document could not be read and report a
/// warning (once per uri).  \p resolvedUri may be empty to indicate
/// the MaterialX standard library documents all rolled into one.
USDMTLX_LOCAL
MaterialX::ConstDocumentPtr
UsdMtlxGetDocument(const std::string& resolvedUri);

/// Returns the (possibly cached) MaterialX document created from the given 
/// string containing the source MaterialX XML.
USDMTLX_LOCAL
MaterialX::ConstDocumentPtr 
UsdMtlxGetDocumentFromString(const std::string &mtlxXml);

// Return the version of the mtlx element.  If the version cannot be
// found then return an invalid default version.  If implicitDefault
// isn't null then we do to two things:  we set implicitDefault to
// false iff the isdefaultversion attribute exists and isn't empty,
// otherwise we set it to true;  and we return the version as a
// default if isdefaultversion exists and is set to "true".
USDMTLX_LOCAL
NdrVersion
UsdMtlxGetVersion(const MaterialX::ConstElementPtr& mtlx,
                  bool* implicitDefault = nullptr);

/// Return the source URI for a MaterialX element.  If the element
/// doesn't have a non-empty URI then return the source URI of the
/// closest element up the element hierarchy that does have one.
/// Return the empty string if no element has a source URI.
USDMTLX_LOCAL
const std::string&
UsdMtlxGetSourceURI(const MaterialX::ConstElementPtr& element);

/// Result of \c UsdMtlxGetUsdType().
struct UsdMtlxUsdTypeInfo {
    UsdMtlxUsdTypeInfo(
        SdfValueTypeName valueTypeName,
        bool valueTypeNameIsExact,
        TfToken shaderPropertyType)
        : valueTypeName(valueTypeName)
        , shaderPropertyType(shaderPropertyType)
        , valueTypeNameIsExact(valueTypeNameIsExact)
    { }

    /// The value type name that most closely matches the MaterialX type.
    /// If the type isn't recognized this is the invalid value type name.
    /// Clients can check for array types by calling \c IsArray() on this.
    SdfValueTypeName valueTypeName;

    /// The exact \c SdrShaderProperty type name.  If there is no exact
    /// match this is empty.
    TfToken shaderPropertyType;

    /// \c true iff the value type name is an exact match to the
    /// MaterialX type.
    bool valueTypeNameIsExact;
};

/// Convert a (standard) MaterialX type name.
USDMTLX_LOCAL
UsdMtlxUsdTypeInfo
UsdMtlxGetUsdType(const std::string& mtlxTypeName);

/// Return the value in \p mtlx as a \c VtValue.  Returns an
/// empty VtValue and reports an error if the conversion cannot be
/// applied.  If \p getDefaultValue is \c true then converts the
/// default value.  It is not an error if the value doesn't exist;
/// that silently returns an empty VtValue.
USDMTLX_LOCAL
VtValue
UsdMtlxGetUsdValue(const MaterialX::ConstElementPtr& mtlx,
                   bool getDefaultValue = false);

/// Return the MaterialX values in \p values assuming it contains an
/// array of values of MaterialX type \p type as a vector of VtValue.
USDMTLX_LOCAL
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
USDMTLX_LOCAL
std::vector<std::string>
UsdMtlxSplitStringArray(const std::string& s);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDMTLX_UTILS_H
