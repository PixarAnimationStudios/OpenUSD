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
#ifndef PXRUSDMAYA_ATTRIBUTECONVERTERREGISTRY_H
#define PXRUSDMAYA_ATTRIBUTECONVERTERREGISTRY_H

/// \file AttributeConverterRegistry.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class AttributeConverter;

/// \brief A registry of all the converters used to import and export
/// USD-specific information stored in Maya attributes (e.g. "USD_hidden").
struct AttributeConverterRegistry {
    /// \brief Registers the given attribute converter.
    /// Ownership of the converter \p converter transfers to
    /// AttributeConverterRegistry.
    PXRUSDMAYA_API
    static void Register(AttributeConverter* converter);
    
    /// \brief Gets a copy of the list of all registered converters.
    PXRUSDMAYA_API
    static std::vector<const AttributeConverter*> GetAllConverters();
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_ATTRIBUTECONVERTERREGISTRY_H
