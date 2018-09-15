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
#ifndef SDF_LAYER_BASE_H
#define SDF_LAYER_BASE_H

/// \file sdf/layerBase.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerBase);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfFileFormat);

class SdfSchemaBase;

/// \class SdfLayerBase
///
/// Base class for all layer implementations. Holds a pointer to the file
/// format for the layer.
///
class SdfLayerBase : public TfRefBase, public TfWeakBase
{
public:
    // Disallow copies
    SdfLayerBase(const SdfLayerBase&) = delete;
    SdfLayerBase& operator=(const SdfLayerBase&) = delete;

    /// Returns the file format used by this layer.
    SDF_API SdfFileFormatConstPtr GetFileFormat() const;

    /// Type for specifying additional file format-specific arguments to
    /// layer API.
    typedef std::map<std::string, std::string> FileFormatArguments;

    /// Returns the file format-specific arguments used during the construction
    /// of this layer.
    SDF_API const FileFormatArguments& GetFileFormatArguments() const;

    /// Returns the schema this layer adheres to. This schema provides details
    /// about the scene description that may be authored in this layer.
    SDF_API virtual const SdfSchemaBase& GetSchema() const = 0;

protected:
    SDF_API
    SdfLayerBase(const SdfFileFormatConstPtr& fileFormat,
                 const FileFormatArguments& args = FileFormatArguments());

    SDF_API
    virtual ~SdfLayerBase();

private:
    SdfFileFormatConstPtr _fileFormat;
    FileFormatArguments _fileFormatArgs;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_LAYER_BASE_H
