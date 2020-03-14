//
// Copyright 2016 Pixar
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
#ifndef PXR_USD_USD_USDA_FILE_FORMAT_H
#define PXR_USD_USD_USDA_FILE_FORMAT_H
 
#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/textFileFormat.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


#define USD_USDA_FILE_FORMAT_TOKENS \
    ((Id,      "usda"))             \
    ((Version, "1.0"))

TF_DECLARE_PUBLIC_TOKENS(UsdUsdaFileFormatTokens, USD_API, USD_USDA_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdUsdaFileFormat);

/// \class UsdUsdaFileFormat
///
/// File format used by textual USD files.
///
class UsdUsdaFileFormat : public SdfTextFileFormat
{
private:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    UsdUsdaFileFormat();

    virtual ~UsdUsdaFileFormat();
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDA_FILE_FORMAT_H
