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
#ifndef USD_PSHADER_SHD_UTILS_H
#define USD_PSHADER_SHD_UTILS_H

// This file is a port of selected parts of ext/global/Util/Shd/Utils to Usd.
//
// The differences are:
// - this deals with Usd scenegraph objects instead of Mf/Shd ones
// - this only covers reading functionality, not authoring
// - for ease of comparison, function names are the same,
//   just prepended with Usd
// - in places where Usd does not provide the same type hierarchy or
//   base types (Object, Property), we use overloads instead
// - we have injected logic into the accessors that helps transition from
//   the Presto "ri" shading model to a more generalized one (see XXX comment
//   in usd/tokens.h)

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdShade/pShader.h"
#include "pxr/usd/usd/relationship.h"

/// Return whether the given relationship represents a shader binding.
/// This is true if it's called "shaders" or the isShaderBinding metadata 
/// is true.
bool UsdShdIsShaderBinding(const UsdRelationship &rel);

/// Return the sloArgName metadata value from the specified property, or an
/// empty string if none exists.
//
std::string UsdShdGetSloArgName(const UsdAttribute &prop);
std::string UsdShdGetSloArgName(const UsdRelationship &prop);

/// Return the ribAttributeName metadata value from the specified property, or
/// an empty string if none exists.
///
std::string UsdShdGetRibAttributeName(const UsdAttribute &prop);
std::string UsdShdGetRibAttributeName(const UsdRelationship &prop);

/// Return the sloPath attribute value from the specified Shader prim.
///
//RpPath UsdShdGetSloPath(const ShdShaderConstHandle &shader);

/// Return whether the given property represents a coshader instance.
///
bool UsdShdIsCoshaderInstance(const UsdAttribute &prop);
bool UsdShdIsCoshaderInstance(const UsdRelationship &prop);

/// Return whether isExplicitRib metadata exists and has a true value for the
/// specified property.
///
bool UsdShdIsExplicitRib(const UsdAttribute &prop);
bool UsdShdIsExplicitRib(const UsdRelationship &prop);

/// Return whether isCoshaderArray metadata exists and has a true value for the
/// specified property.
///
bool UsdShdIsCoshaderArray(const UsdAttribute &prop);
bool UsdShdIsCoshaderArray(const UsdRelationship &prop);

/// Return the shaderType attribute value from the specified Shader prim.
///
//ShdShaderType UsdShdGetShaderType(const ShdShaderConstHandle &shader);

/// Return the coshaderHandle metadata value from the specified property, or
/// an empty string if none exists.
///
std::string UsdShdGetCoshaderHandle(const UsdAttribute &prop);
std::string UsdShdGetCoshaderHandle(const UsdRelationship &prop);

/// Return a RIB-safe name for the given object that is guaranteed to be
/// unique.  Used to, e.g., give unique names to shader instances.
std::string UsdShdGetPath(const UsdPrim &obj);

/// Return the handle that should be used for the given shader.
/// This is the path unless an explicit riName was specified.
std::string UsdShdGetShaderHandle(const UsdPrim &obj);

/// Given a rib attribute name, split on the first ':' and return the two parts
/// as attrName and argName.
/// If no ':' occurs in the string, then split on the first '_' instead.
/// Return false if neither ':' nor '_' occur in the string.
bool UsdShdSplitRibAttributeName(const std::string &name,
                                 std::string *attrName, std::string *argName);

#endif
