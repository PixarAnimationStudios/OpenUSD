//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PLUGIN_USD_ABC_ALEMBIC_WRITER_H
#define PXR_USD_PLUGIN_USD_ABC_ALEMBIC_WRITER_H

/// \file usdAbc/alembicWriter.h

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/declarePtrs.h"
#include <memory>
#include <set>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// Note -- Even though this header is private we try to keep Alembic headers
//         out of it anyway for simplicity's sake.

TF_DECLARE_WEAK_AND_REF_PTRS(SdfAbstractData);

/// \class UsdAbc_AlembicDataWriter
///
/// An alembic writer suitable for an SdfAbstractData.
///
class UsdAbc_AlembicDataWriter {
public:
    UsdAbc_AlembicDataWriter();
    UsdAbc_AlembicDataWriter (const UsdAbc_AlembicDataWriter&) = delete;
    UsdAbc_AlembicDataWriter& operator= (const UsdAbc_AlembicDataWriter&) = delete;
    ~UsdAbc_AlembicDataWriter();

    bool Open(const std::string& filePath, const std::string& comment);
    bool Write(const SdfAbstractDataConstPtr& data);
    bool Close();

    bool IsValid() const;
    std::string GetErrors() const;

    void SetFlag(const TfToken&, bool set = true);

private:
    std::unique_ptr<class UsdAbc_AlembicDataWriterImpl> _impl;
    std::string _errorLog;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
