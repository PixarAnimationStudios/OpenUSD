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
#ifndef USDABC_ALEMBICREADER_H
#define USDABC_ALEMBICREADER_H

/// \file usdAbc/alembicReader.h

#include "pxr/usd/sdf/abstractData.h"
#include "pxr/base/tf/token.h"
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <stdint.h>
#include <string>
#include <vector>

// Note -- Even though this header is private we try to keep Alembic headers
//         out of it anyway for simplicity's sake.

/// \class UsdAbc_AlembicDataReader
///
/// An alembic reader suitable for an SdfAbstractData.
///
class UsdAbc_AlembicDataReader : boost::noncopyable {
public:
    typedef int64_t Index;

    UsdAbc_AlembicDataReader();
    ~UsdAbc_AlembicDataReader();

    /// Open a file.  Returns \c true on success;  errors are reported by
    /// \c GetErrors().
    bool Open(const std::string& filePath);

    /// Close the file.
    void Close();

    /// Return any errors.
    std::string GetErrors() const;

    /// Set a reader flag.
    void SetFlag(const TfToken&, bool set = true);

    /// Test for the existence of a spec at \p id.
    bool HasSpec(const SdfAbstractDataSpecId& id) const;

    /// Returns the spec type for the spec at \p id.
    SdfSpecType GetSpecType(const SdfAbstractDataSpecId& id) const;

    /// Test for the existence of and optionally return the value at
    /// (\p id,\p fieldName).
    bool HasField(const SdfAbstractDataSpecId& id,
                  const TfToken& fieldName,
                  SdfAbstractDataValue* value) const;

    /// Test for the existence of and optionally return the value at
    /// (\p id,\p fieldName).
    bool HasField(const SdfAbstractDataSpecId& id,
                  const TfToken& fieldName,
                  VtValue* value) const;

    /// Test for the existence of and optionally return the value of the
    /// property at \p id at index \p index.
    bool HasValue(const SdfAbstractDataSpecId& id, Index index,
                  SdfAbstractDataValue* value) const;

    /// Test for the existence of and optionally return the value of the
    /// property at \p id at index \p index.
    bool HasValue(const SdfAbstractDataSpecId& id, Index index,
                  VtValue* value) const;

    /// Visit the specs.
    void VisitSpecs(const SdfAbstractData& owner,
                    SdfAbstractDataSpecVisitor* visitor) const;

    /// List the fields.
    TfTokenVector List(const SdfAbstractDataSpecId& id) const;

    /// The type holds a set of Usd times and can return an Alembic index
    /// for each time.
    class TimeSamples {
        typedef std::vector<double> _UsdTimeCodes;
    public:
        typedef _UsdTimeCodes::const_iterator const_iterator;

        /// Construct an empty set of samples.
        TimeSamples();

        /// Construct from the time samples which must be monotonically
        /// increasing.
        TimeSamples(const std::vector<double>& times);

        /// Swaps the contents of this with \p other.
        void Swap(TimeSamples& other);

        /// Returns \c true iff there are no samples.
        bool IsEmpty() const;

        /// Returns the number of samples.
        size_t GetSize() const;

        /// Returns the Usd times.
        std::set<double> GetTimes() const;

        /// Returns the time sample at index \p index.
        double operator[](size_t index) const;

        /// Add these Usd times to the given set.
        void AddTo(std::set<double>*) const;

        /// Returns the index for Usd time \p usdTime and returns \c true
        /// or returns \c false if \p usdTime is not in the set of samples.
        bool FindIndex(double usdTime, Index* index) const;

        /// Returns the times bracketing \p time.
        bool Bracket(double usdTime, double* tLower, double* tUpper) const;

        /// Returns the times bracketing \p time.
        template <class T>
        static bool Bracket(const T&, double usdTime,
                            double* tLower, double* tUpper);

    private:
        // The monotonically increasing Usd times.
        _UsdTimeCodes _times;
    };

    /// Returns the sampled times over all properties.
    const std::set<double>& ListAllTimeSamples() const;

    /// Returns the sampled times for the property with id \p id.
    const TimeSamples& 
    ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const;

private:
    boost::scoped_ptr<class UsdAbc_AlembicDataReaderImpl> _impl;
    std::string _errorLog;
};

#endif
