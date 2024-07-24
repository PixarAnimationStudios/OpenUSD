//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PLUGIN_USD_ABC_ALEMBIC_DATA_H
#define PXR_USD_PLUGIN_USD_ABC_ALEMBIC_DATA_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/declarePtrs.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(UsdAbc_AlembicData);

/// \class UsdAbc_AlembicData
///
/// Provides an SdfAbstractData interface to Alembic data.
///
class UsdAbc_AlembicData : public SdfAbstractData {
public:
    /// Returns a new \c UsdAbc_AlembicData object.  Outside a successful
    /// \c Open() and \c Close() pairing, the data acts as if it contains
    /// a pseudo-root prim spec at the absolute root path.
    static UsdAbc_AlembicDataRefPtr New(
                    SdfFileFormat::FileFormatArguments = {});

    /// Opens the Alembic file at \p filePath read-only (closing any open
    /// file).  Alembic is not meant to be used as an in-memory store for
    /// editing so methods that modify the file are not supported.
    /// \sa Write().
    bool Open(const std::string& filePath);

    /// Closes the Alembic file.  This does nothing if already closed.
    /// After the call it's as if the object was just created by New().
    void Close();

    /// Write the contents of \p data to a new or truncated Alembic file at
    /// \p filePath with the comment \p comment.  \p data is not modified.
    static bool Write(const SdfAbstractDataConstPtr& data,
                      const std::string& filePath,
                      const std::string& comment);

    // SdfAbstractData overrides
    virtual bool StreamsData() const;
    virtual void CreateSpec(const SdfPath&, SdfSpecType specType);
    virtual bool HasSpec(const SdfPath&) const;
    virtual void EraseSpec(const SdfPath&);
    virtual void MoveSpec(const SdfPath& oldPath, const SdfPath& newPath);
    virtual SdfSpecType GetSpecType(const SdfPath&) const;
    virtual bool Has(const SdfPath&, const TfToken& fieldName,
                     SdfAbstractDataValue* value) const;
    virtual bool Has(const SdfPath&, const TfToken& fieldName,
                     VtValue* value = NULL) const;
    virtual VtValue Get(const SdfPath&, const TfToken& fieldName) const;
    virtual void Set(const SdfPath&, const TfToken& fieldName,
                     const VtValue& value);
    virtual void Set(const SdfPath&, const TfToken& fieldName,
                     const SdfAbstractDataConstValue& value);
    virtual void Erase(const SdfPath&, const TfToken& fieldName);
    virtual std::vector<TfToken> List(const SdfPath&) const;
    virtual std::set<double>
    ListAllTimeSamples() const;
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfPath&) const;
    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;
    virtual size_t
    GetNumTimeSamplesForPath(const SdfPath& path) const;
    virtual bool
    GetBracketingTimeSamplesForPath(const SdfPath&,
                                    double time,
                                    double* tLower, double* tUpper) const;
    virtual bool
    QueryTimeSample(const SdfPath&, double time,
                    SdfAbstractDataValue* value) const;
    virtual bool
    QueryTimeSample(const SdfPath&, double time,
                    VtValue* value) const;
    virtual void
    SetTimeSample(const SdfPath&, double, const VtValue&);
    virtual void
    EraseTimeSample(const SdfPath&, double);

protected:
    UsdAbc_AlembicData(SdfFileFormat::FileFormatArguments);
    virtual ~UsdAbc_AlembicData();

    // SdfAbstractData overrides
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

private:
    std::shared_ptr<class UsdAbc_AlembicDataReader> _reader;
    const SdfFileFormat::FileFormatArguments _arguments;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PLUGIN_USD_ABC_ALEMBIC_DATA_H
