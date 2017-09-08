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
///
/// \file Sdf/textFileFormat.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/textFileFormat.h"
#include "pxr/usd/sdf/fileIO.h"
#include "pxr/usd/sdf/fileIO_Common.h"
#include "pxr/usd/sdf/layer.h"

#include "pxr/base/tracelite/trace.h"
#include "pxr/base/tf/atomicOfstreamWrapper.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/arch/fileSystem.h"

#include <boost/assign.hpp>
#include <ostream>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(SdfTextFileFormatTokens, SDF_TEXT_FILE_FORMAT_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

// Our interface to the YACC menva parser for parsing to SdfData.
extern bool Sdf_ParseMenva(
    const string& context, 
    FILE *f,
    const string& token,
    const string& version,
    bool metadataOnly,
    PXR_NS::SdfDataRefPtr data);

extern bool Sdf_ParseMenvaFromString(
    const std::string & menvaString,
    const string& token,
    const string& version,
    PXR_NS::SdfDataRefPtr data);

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(SdfTextFileFormat, SdfFileFormat);
}

SdfTextFileFormat::SdfTextFileFormat()
    : SdfFileFormat(
        SdfTextFileFormatTokens->Id,
        SdfTextFileFormatTokens->Version,
        SdfTextFileFormatTokens->Target,
        SdfTextFileFormatTokens->Id.GetString())
{
    // Do Nothing.
}

SdfTextFileFormat::SdfTextFileFormat(
    const TfToken& formatId,
    const TfToken& versionString,
    const TfToken& target)
    : SdfFileFormat(formatId,
                    (versionString.IsEmpty()
                     ? SdfTextFileFormatTokens->Version : versionString),
                    (target.IsEmpty()
                     ? SdfTextFileFormatTokens->Target : target),
                    formatId )
{
    // Do Nothing.
}

SdfTextFileFormat::~SdfTextFileFormat()
{
    // Do Nothing.
}

bool
SdfTextFileFormat::CanRead(const string& filePath) const
{
    TRACE_FUNCTION();

    bool canRead = false;
    if (FILE *f = ArchOpenFile(filePath.c_str(), "rb")) {
        canRead = _CanReadImpl(f);
        fclose(f);
    }

    return canRead;
}

bool
SdfTextFileFormat::_CanReadImpl(FILE *fp) const
{
    const string &cookie = GetFileCookie();
    char aLine[512];
    return fgets(aLine, sizeof(aLine), fp) && TfStringStartsWith(aLine, cookie);
}

class Sdf_ScopedFilePointer : boost::noncopyable
{
public:
    explicit Sdf_ScopedFilePointer(const string& filePath)
        : _fp(ArchOpenFile(filePath.c_str(), "rb"))
    { }

    ~Sdf_ScopedFilePointer() {
        if (_fp)
            fclose(_fp);
    }

    FILE* operator *() const {
        return _fp;
    }

private:
    FILE* _fp;
};

bool
SdfTextFileFormat::Read(
    const SdfLayerBasePtr& layerBase,
    const string& resolvedPath,
    bool metadataOnly) const
{
    TRACE_FUNCTION();

    Sdf_ScopedFilePointer fp(resolvedPath);
    if (!*fp)
        return false;

    SdfLayerHandle layer = TfDynamic_cast<SdfLayerHandle>(layerBase);
    if (!TF_VERIFY(layer)) {
        return false;
    }

    // Quick check to see if the file has the magic cookie before spinning up
    // the parser.
    if (!_CanReadImpl(*fp)) {
        TF_RUNTIME_ERROR("File <%s> is not a valid %s file",
                         resolvedPath.c_str(),
                         GetFormatId().GetText());
        return false;
    }
    fseek(*fp, 0, SEEK_SET);

    SdfAbstractDataRefPtr data = InitData(layerBase->GetFileFormatArguments());
    if (!Sdf_ParseMenva(resolvedPath, *fp, 
                           GetFormatId(),
                           GetVersionString(),
                           metadataOnly,
                           TfDynamic_cast<SdfDataRefPtr>(data))) {
        return false;
    }

    if (_LayerIsLoadingAsNew(layer)) {
        // New layer, so we don't need undo inverses or notification.
        // Just swap out the data.
        _SwapLayerData(layer, data);
    } else {
        // Layer has pre-existing data.  Use _SetData() to provide
        // fine-grained inverses and undo registration.
        _SetLayerData(layer, data);
    }

    return true;
}

// Predicate for determining fields that should be included in a
// layer's metadata section.
struct Sdf_IsLayerMetadataField : public Sdf_IsMetadataField
{
    Sdf_IsLayerMetadataField() : Sdf_IsMetadataField(SdfSpecTypePseudoRoot) { }
    
    bool operator()(const TfToken& field) const
    { 
        return (Sdf_IsMetadataField::operator()(field) ||
            field == SdfFieldKeys->SubLayers);
    }
};


#define _Write             Sdf_FileIOUtility::Write
#define _WriteQuotedString Sdf_FileIOUtility::WriteQuotedString
#define _WriteAssetPath    Sdf_FileIOUtility::WriteAssetPath
#define _WriteSdPath       Sdf_FileIOUtility::WriteSdPath
#define _WriteNameVector   Sdf_FileIOUtility::WriteNameVector
#define _WriteLayerOffset  Sdf_FileIOUtility::WriteLayerOffset

static bool
_WriteLayerToMenva(
    const SdfLayer* l,
    std::ostream& out,
    const string& cookie,
    const string& versionString,
    const string& comment)
{
    _Write(out, 0, "%s %s\n", cookie.c_str(), versionString.c_str());

    // Grab the pseudo-root, which is where all layer-specific
    // fields live.
    SdfPrimSpecHandle pseudoRoot = l->GetPseudoRoot();

    // Accumulate header metadata in a stringstream buffer,
    // as an easy way to check later if we have any layer
    // metadata to write at all.
    std::ostringstream header;

    // Partition this layer's fields so that all fields to write out are
    // in the range [fields.begin(), metadataFieldsEnd).
    TfTokenVector fields = pseudoRoot->ListFields();
    TfTokenVector::iterator metadataFieldsEnd = 
        std::partition(fields.begin(), fields.end(), Sdf_IsLayerMetadataField());

    // Write comment at the top of the metadata section for readability.
    if (!comment.empty())
    {
        _WriteQuotedString(header, 1, comment);
        _Write(header, 0, "\n");
    }

    // Write out remaining fields in the metadata section in alphabetical
    // order.
    std::sort(fields.begin(), metadataFieldsEnd);
    for (TfTokenVector::const_iterator fieldIt = fields.begin();
         fieldIt != metadataFieldsEnd; ++fieldIt) {

        const TfToken& field = *fieldIt;

        if (field == SdfFieldKeys->Documentation) {
            if (!l->GetDocumentation().empty()) {
                _Write(header, 1, "doc = ");
                _WriteQuotedString(header, 0, l->GetDocumentation());
                _Write(header, 0, "\n");
            }
        }
        else if (field == SdfFieldKeys->SubLayers) {
            _Write(header, 1, "subLayers = [\n");

            size_t c = l->GetSubLayerPaths().size();
            for(size_t i=0; i<c; i++)
            {
                _WriteAssetPath(header, 2, l->GetSubLayerPaths()[i]);
                _WriteLayerOffset(header, 0, false /* multiLine */, 
                                  l->GetSubLayerOffset(static_cast<int>(i)));
                _Write(header, 0, (i < c-1) ? ",\n" : "\n");
            }
            _Write(header, 1, "]\n");
        }
        else if (field == SdfFieldKeys->HasOwnedSubLayers) {
            if (l->GetHasOwnedSubLayers()) {
                _Write(header, 1, "hasOwnedSubLayers = true\n");
            }
        }
        else {
            Sdf_WriteSimpleField(header, 1, pseudoRoot.GetSpec(), field);
        }

    } // end for each field

    // Write header if not empty.
    string headerStr = header.str();
    if (!headerStr.empty()) {
        _Write(out, 0, "(\n");
        _Write(out, 0, headerStr.c_str());
        _Write(out, 0, ")\n");
    }

    // Root prim reorder statement
    const std::vector<TfToken> &rootPrimNames = l->GetRootPrimOrder();

    if (rootPrimNames.size() > 1)
    {
        _Write(out, 0,"\n");
        _Write(out, 0, "reorder rootPrims = ");
        _WriteNameVector(out, 0, rootPrimNames);
        _Write(out, 0, "\n");
    }
        
    // Root prims
    TF_FOR_ALL(i, l->GetRootPrims())
    {
        _Write(out, 0,"\n");
        (*i)->WriteToStream(out, 0);
    }

    _Write(out, 0,"\n");

    return true;
}

bool
SdfTextFileFormat::WriteToFile(
    const SdfLayerBase* layerBase,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    // open file
    string reason;
    TfAtomicOfstreamWrapper wrapper(filePath);
    if (!wrapper.Open(&reason)) {
        TF_RUNTIME_ERROR(reason);
        return false;
    }

    bool ok = Write(layerBase, wrapper.GetStream(), comment);

    if (ok && !wrapper.Commit(&reason)) {
        TF_RUNTIME_ERROR(reason);
        return false;
    }

    return ok;
}

bool 
SdfTextFileFormat::ReadFromString(
    const SdfLayerBasePtr& layerBase,
    const std::string& str) const
{
    SdfLayerHandle layer = TfDynamic_cast<SdfLayerHandle>(layerBase);
    if (!TF_VERIFY(layer)) {
        return false;
    }

    SdfAbstractDataRefPtr data = InitData(layerBase->GetFileFormatArguments());
    if (!Sdf_ParseMenvaFromString(str, 
                                  GetFormatId(),
                                  GetVersionString(),
                                  TfDynamic_cast<SdfDataRefPtr>(data))) {
        return false;
    }

    if (_LayerIsLoadingAsNew(layer)) {
        // New layer, so we don't need undo inverses or notification.
        // Just swap out the data.
        _SwapLayerData(layer, data);
    } else {
        // Layer has pre-existing data.  Use _SetData() to provide
        // fine-grained inverses and undo registration.
        _SetLayerData(layer, data);
    }

    return true;
}

bool 
SdfTextFileFormat::WriteToString(
    const SdfLayerBase* layerBase,
    std::string* str,
    const std::string& comment) const
{
    std::stringstream ostr;
    if (!Write(layerBase, ostr, comment)) {
        return false;
    }

    *str = ostr.str();
    return true;
}

bool
SdfTextFileFormat::WriteToStream(
    const SdfLayerBase* layerBase,
    std::ostream& ostr) const
{
    return Write(layerBase, ostr);
}

bool
SdfTextFileFormat::Write(
    const SdfLayerBase* layerBase,
    std::ostream& ostr,
    const string& commentOverride) const
{
    TRACE_FUNCTION();

    const SdfLayer *layer = dynamic_cast<const SdfLayer *>(layerBase);
    if (!TF_VERIFY(layer)) {
        return false;
    }

    string comment = commentOverride.empty() ?
        layer->GetComment() : commentOverride;

    return _WriteLayerToMenva(layer, ostr, GetFileCookie(),
                              GetVersionString(), comment);

    return false;
}

bool 
SdfTextFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    return Sdf_WriteToStream(spec.GetSpec(), out, indent);
}

bool
SdfTextFileFormat::_ShouldSkipAnonymousReload() const
{
    return false;
}

bool 
SdfTextFileFormat::_IsStreamingLayer(const SdfLayerBase& layer) const
{
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
