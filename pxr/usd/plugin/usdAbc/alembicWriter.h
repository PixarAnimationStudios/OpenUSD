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
/// \file alembicWriter.h
#ifndef USDABC_ALEMBICWRITER_H
#define USDABC_ALEMBICWRITER_H

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/declarePtrs.h"
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <set>
#include <string>

// Note -- Even though this header is private we try to keep Alembic headers
//         out of it anyway for simplicity's sake.

TF_DECLARE_WEAK_AND_REF_PTRS(SdfAbstractData);

/// \class UsdAbc_AlembicDataWriter
///
/// An alembic writer suitable for an SdfAbstractData.
///
class UsdAbc_AlembicDataWriter : boost::noncopyable {
public:
    UsdAbc_AlembicDataWriter();
    ~UsdAbc_AlembicDataWriter();

    bool Open(const std::string& filePath, const std::string& comment);
    bool Write(const SdfAbstractDataConstPtr& data);
    bool Close();

    bool IsValid() const;
    std::string GetErrors() const;

    void SetFlag(const TfToken&, bool set = true);

private:
    boost::scoped_ptr<class UsdAbc_AlembicDataWriterImpl> _impl;
    std::string _errorLog;
};

#endif
