//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PLUGIN_USD_ABC_ALEMBIC_FILE_FORMAT_H
#define PXR_USD_PLUGIN_USD_ABC_ALEMBIC_FILE_FORMAT_H
 
#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/staticTokens.h"
#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define USDABC_ALEMBIC_FILE_FORMAT_TOKENS  \
    ((Id,      "abc"))                  \
    ((Version, "1.0"))                  \
    ((Target,  "usd"))

TF_DECLARE_PUBLIC_TOKENS(UsdAbcAlembicFileFormatTokens, USDABC_ALEMBIC_FILE_FORMAT_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(UsdAbcAlembicFileFormat);

/// \class UsdAbcAlembicFileFormat
///
class UsdAbcAlembicFileFormat : public SdfFileFormat {
public:
    // SdfFileFormat overrides
    virtual SdfAbstractDataRefPtr InitData(const FileFormatArguments&) const override;
    virtual bool CanRead(const std::string &file) const override;
    virtual bool Read(SdfLayer* layer,
                      const std::string& resolvedPath,
                      bool metadataOnly) const override;
    virtual bool WriteToFile(const SdfLayer& layer,
                             const std::string& filePath,
                             const std::string& comment = std::string(),
                             const FileFormatArguments& args = 
                                 FileFormatArguments()) const override;
    virtual bool ReadFromString(SdfLayer* layer,
                                const std::string& str) const override;
    virtual bool WriteToString(const SdfLayer& layer,
                               std::string* str,
                               const std::string& comment=std::string()) 
                               const override;
    virtual bool WriteToStream(const SdfSpecHandle &spec,
                               std::ostream& out,
                               size_t indent) const override;

protected:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    virtual ~UsdAbcAlembicFileFormat();

    UsdAbcAlembicFileFormat();

    bool _ReadDetached(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override;

private:
    SdfFileFormatConstPtr _usda;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ABC_ALEMBIC_FILE_FORMAT_H
