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
#ifndef USDOBJ_FILE_FORMAT_H
#define USDOBJ_FILE_FORMAT_H

#include "Python.h" 
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"
#include <iosfwd>
#include <string>

#define USDOBJ_FILE_FORMAT_TOKENS       \
    ((Id,      "obj"))                  \
    ((Version, "1.0"))                  \
    ((Target,  "usd"))

TF_DECLARE_PUBLIC_TOKENS(UsdObjFileFormatTokens, USDOBJ_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdObjFileFormat);
TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayerBase);

/// \class UsdObjFileFormat
///
/// This is an example tutorial file format plugin for Usd.  It is not meant to
/// be a full-featured OBJ importer.  Rather, it's intentionally just barely
/// functional so as not to obscure the fundamental plugin structure.  It could
/// serve as a starting point for a more full-featured OBJ importer, or an
/// importer for another format.  For a much more fully-featured example, see
/// the usdAbc alembic plugin.
///
class UsdObjFileFormat : public SdfFileFormat {
public:

    // SdfFileFormat overrides.
    virtual bool CanRead(const std::string &file) const;
    virtual bool Read(const SdfLayerBasePtr& layerBase,
                      const std::string& resolvedPath,
                      bool metadataOnly) const;
    virtual bool ReadFromString(const SdfLayerBasePtr& layerBase,
                                const std::string& str) const;

    // We override Write methods so SdfLayer::ExportToString() etc, work.  We
    // don't support writing general Usd data back to OBJ files.  So
    // SdfLayer::Save() doesn't work, for example.
    virtual bool WriteToString(const SdfLayerBase* layerBase,
                               std::string* str,
                               const std::string& comment=std::string()) const;
    virtual bool WriteToStream(const SdfSpecHandle &spec,
                               std::ostream& out,
                               size_t indent) const;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    virtual ~UsdObjFileFormat();
    UsdObjFileFormat();

private:
    bool _ReadFromStream(const SdfLayerBasePtr &layerBase,
                         std::istream &input,
                         bool metadataOnly,
                         std::string *outErr) const;

    virtual bool _IsStreamingLayer(const SdfLayerBase& layer) const {
        return false;
    }
};

#endif // USDOBJ_FILE_FORMAT_H
