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
/// \file sdf/mapperSpec.h

#ifndef SDF_MAPPERSPEC_H
#define SDF_MAPPERSPEC_H

#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/usd/sdf/proxyTypes.h"
#include "pxr/usd/sdf/types.h"
#include <string>

///
/// \class SdfMapperSpec 
/// \brief Represents the mapper to be used for values coming from 
/// a particular connection path of an attribute.
///
/// When instantiated on a stage, the appropriate subclass of MfMapper
/// will be chosen based on the mapper spec's type name.
class SdfMapperSpec : public SdfSpec 
{
    SDF_DECLARE_SPEC(SdfSchema, SdfSpecTypeMapper, SdfMapperSpec, SdfSpec);

public:
    typedef SdfMapperSpec This;
    typedef SdfSpec Parent;

    ///
    /// \name Spec creation
    /// @{

    /// \brief Create a mapper spec.
    ///
    /// Creates and returns a new mapper owned by the attribute \p owner
    /// with the type name \p typeName.
    ///
    /// Mappers must be created in the context of an existing attribute.
    static SdfMapperSpecHandle New(const SdfAttributeSpecHandle& owner,
                                   const SdfPath& connPath,
                                   const std::string& typeName);

    /// @}

    /// \name Namespace hierarchy
    /// @{

    /// Returns the attribute that owns this mapper.
    SdfAttributeSpecHandle GetAttribute() const;

    ///
    /// Returns the connection path this mapper is associated with.
    SdfPath GetConnectionTargetPath() const;

    /// @}
    /// \name Type
    /// @{

    /// Returns the type name for the mapper.
    std::string GetTypeName() const;
    
    /// Sets the type name for the mapper.
    void SetTypeName(const std::string& typeName);

    /// @}
    /// \name Args
    /// @{

    /// \brief Returns the mapper's args.
    ///
    /// The returned object is a proxy through which the args can be accessed
    /// or deleted.  It is not allowed to create new arguments using the list;
    /// Construct an \c SdfMapperArgSpec directly to do that.
    SdfMapperArgsProxy GetArgs() const;

    /// @}
    /// \name Symmetry args
    /// @{

    /// \brief Returns the mapper's symmetry args.
    SdfDictionaryProxy GetSymmetryArgs() const;
    
    /// \brief Sets the mapper's symmetry args
    void SetSymmetryArgs(const VtDictionary& args);

    /// @}
};

#endif /* SDF_MAPPERSPEC_H */
