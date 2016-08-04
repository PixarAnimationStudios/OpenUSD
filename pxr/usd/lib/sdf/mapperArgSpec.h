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
/// \file sdf/mapperArgSpec.h

#ifndef SDF_MAPPERARGSPEC_H
#define SDF_MAPPERARGSPEC_H

#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/base/vt/value.h"
#include <string>

SDF_DECLARE_HANDLES(SdfMapperArgSpec);
SDF_DECLARE_HANDLES(SdfMapperSpec);

///
/// \class SdfMapperArgSpec 
/// \brief Represents an argument to a specific mapper.
///
class SdfMapperArgSpec : public SdfSpec 
{
    SDF_DECLARE_SPEC(SdfSchema, SdfSpecTypeMapperArg,
                     SdfMapperArgSpec, SdfSpec);

public:
    typedef SdfMapperArgSpec This;
    typedef SdfSpec Parent;

    ///
    /// \name Spec creation
    /// @{

    /// \brief Create a mapper arg spec.
    ///
    /// Creates and returns a new mapper arg owned by mapper \a owner with
    /// the name \p name and value \p value.
    ///
    /// Mapper args must be created in the context of an existing mapper.
    SDF_API
    static SdfMapperArgSpecHandle New(const SdfMapperSpecHandle& owner, 
                                      const std::string& name, 
                                      const VtValue& value);

    /// \name Namespace hierarchy
    /// @{

    /// Returns the mapper that owns this arg.
    SDF_API
    SdfMapperSpecHandle GetMapper() const;

    /// Returns the name for the mapper arg.
    SDF_API
    const std::string& GetName() const;

    /// Returns the name for the mapper arg.
    SDF_API
    const TfToken& GetNameToken() const;

    /// Sets the name of this mapper arg.
    SDF_API
    void SetName(const std::string& name);

    /// @}
    /// \name Value
    /// @{

    /// Returns the value of the mapper arg.
    SDF_API
    VtValue GetValue() const;

    /// Sets the value of the mapper arg.
    SDF_API
    void SetValue(const VtValue& value);

    /// @}
};

#endif /* SDF_MAPPERARGSPEC_H */
