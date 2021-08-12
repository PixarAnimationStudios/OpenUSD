//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_DATASOURCELOCATOR_H
#define PXR_IMAGING_HD_DATASOURCELOCATOR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/hash.h"

#include "pxr/imaging/hd/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdDataSourceLocator
///
/// Represents an object that can identify the location of a data source.
/// Data Source Locators are meant to be short lists of tokens that, taken
/// together, can represent the location of a given data source.
///
class HdDataSourceLocator
{
public:

    /// Returns a common empty locator.
    ///
    /// This is an often needed locator and is quicker to get this way rather
    /// than creating your own empty one.
    HD_API
    static const HdDataSourceLocator &EmptyLocator();

    /// Creates an empty locator.
    ///
    /// If all you need is an empty locator, see EmptyLocator().
    ///
    HD_API
    HdDataSourceLocator();

    /// The following constructors take a number of tokens and build a locator
    /// with the apporpriate number of tokens in the given order. 
    ///
    /// These are convenience constructors for commonly used patterns. Note
    /// that we generally expect a very small number of entities in a locator,
    /// which is why we haven't gone with a more general N-way solution.
    ///
    HD_API
    explicit HdDataSourceLocator(const TfToken &t1);
    HD_API
    HdDataSourceLocator(const TfToken &t1, const TfToken &t2);
    HD_API
    HdDataSourceLocator(const TfToken &t1, const TfToken &t2, 
                        const TfToken &t3);
    HD_API
    HdDataSourceLocator(const TfToken &t1, const TfToken &t2, const TfToken &t3,
                        const TfToken &t4);
    HD_API
    HdDataSourceLocator(const TfToken &t1, const TfToken &t2, const TfToken &t3,
                        const TfToken &t4, const TfToken &t5);
    HD_API
    HdDataSourceLocator(const TfToken &t1, const TfToken &t2, const TfToken &t3,
                        const TfToken &t4, const TfToken &t5, 
                        const TfToken &t6);

    /// Builds a data source locator from the \p tokens array of the given 
    /// \p count.
    ///
    HD_API
    HdDataSourceLocator(size_t count, const TfToken *tokens);

    /// Copy constructor
    HdDataSourceLocator(const HdDataSourceLocator &rhs) = default;

    /// Returns the number of elements (tokens) in this data source.
    HD_API 
    size_t GetElementCount() const;

    /// Returns the element (token) at index \p i.
    ///
    /// If \p i is out of bounds, the behavior is undefined.
    ///
    HD_API
    const TfToken &GetElement(size_t i) const;
 
    /// Returns the first element, or empty token if none.
    HD_API
    const TfToken &GetFirstElement() const;

    /// Returns the last element, or empty token if none.
    HD_API
    const TfToken &GetLastElement() const;

    /// Returns a copy of this data source locator with the last element 
    /// replaced by the one given by \p name. If this data source locator is 
    /// empty an identical copy is returned.
    ///
    HD_API
    HdDataSourceLocator ReplaceLastElement(const TfToken &name) const;

    /// Returns a copy of this data source locator with the last element
    /// removed.
    ///
    HD_API
    HdDataSourceLocator RemoveLastElement() const;

    /// Returns a copy of this data source locator with the first element
    /// removed.
    ///
    HD_API
    HdDataSourceLocator RemoveFirstElement() const;

    /// Appends \p name to this data source locator.
    HD_API
    HdDataSourceLocator Append(const TfToken &name) const;

    /// Appends all of the elements in \p locator to this data source locator.
    HD_API
    HdDataSourceLocator Append(const HdDataSourceLocator &locator) const;

    /// Prepends \p name to this data source locator.
    HD_API
    HdDataSourceLocator Prepend(const TfToken &name) const;

    /// Prepends all of the elements in \p locator to this data source locator.
    HD_API
    HdDataSourceLocator Prepend(const HdDataSourceLocator &locator) const;

    /// Returns \c true if this data source locator has \p prefix as a prefix.
    HD_API
    bool HasPrefix(const HdDataSourceLocator &prefix) const;

    /// Returns a data source locator that represents the common prefix
    /// between this data source and \p other.
    ///
    HD_API 
    HdDataSourceLocator GetCommonPrefix(const HdDataSourceLocator &other) const;

    /// Returns a copy of this data source locator with \p oldPrefix replaced
    /// by \p newPrefix.
    ///
    HD_API
    HdDataSourceLocator ReplacePrefix(
        const HdDataSourceLocator &oldPrefix,
        const HdDataSourceLocator &newPrefix) const;

    /// Returns \c true if either this or other are equal to or contain one 
    /// another
    ///
    HD_API
    bool Intersects(const HdDataSourceLocator &other) const;

    inline bool operator==(const HdDataSourceLocator &rhs) const {
        return _tokens == rhs._tokens;
    }

    inline bool operator!=(const HdDataSourceLocator &rhs) const {
        return _tokens != rhs._tokens;
    }

    HD_API
    bool operator<(const HdDataSourceLocator &rhs) const;

    inline bool IsEmpty() const {
        return _tokens.empty();
    }

    /// Returns a string representation of this data source locator with the
    /// given \p delimiter inserted between each element.
    ///
    HD_API
    std::string GetString(const char *delimiter = "/") const;

    template <class HashState>
    friend void TfHashAppend(HashState &h, HdDataSourceLocator const &myObj) {
        h.AppendContiguous(myObj._tokens.data(), myObj._tokens.size());
    }

    inline size_t Hash() const;

private:
    using _TokenVector = TfSmallVector<TfToken, 6>;
    _TokenVector _tokens;
};

inline size_t
HdDataSourceLocator::Hash() const
{
    return TfHash()(*this);
}

//-----------------------------------------------------------------------------

///
/// \class HdDataSourceLocatorSet
///
/// Represents a set of data source locators.
///
class HdDataSourceLocatorSet
{
private:
    using _Locators = TfSmallVector<HdDataSourceLocator, 8>;
public:
    using const_iterator = typename _Locators::const_iterator;

    explicit HdDataSourceLocatorSet() {}

    HdDataSourceLocatorSet(const HdDataSourceLocator &locator);

    // Initializer list constructor.
    HdDataSourceLocatorSet(
        const std::initializer_list<const HdDataSourceLocator> &l);

    /// Copy Ctor
    HdDataSourceLocatorSet(const HdDataSourceLocatorSet &rhs) = default;

    /// Move Ctor.
    HdDataSourceLocatorSet(HdDataSourceLocatorSet &&rhs) = default;

    /// Move assignment operator.
    HdDataSourceLocatorSet &operator=(HdDataSourceLocatorSet &&rhs) = default;

    /// Copy assignment operator.
    HdDataSourceLocatorSet &operator=(const HdDataSourceLocatorSet &rhs) 
        = default;

    HD_API
    void insert(const HdDataSourceLocator &locator);
    HD_API
    void insert(const HdDataSourceLocatorSet &locatorSet);

    /// append() is semantically equivalent to insert(), but works much faster
    /// if \p locator would be added to the end of the set, lexicographically.
    HD_API
    void append(const HdDataSourceLocator &locator);

    bool operator==(const HdDataSourceLocatorSet &rhs) const {
        return _locators == rhs._locators;
    }

    bool operator!=(const HdDataSourceLocatorSet &rhs) const {
        return !(*this == rhs);
    }

    HD_API
    const_iterator begin() const;
    HD_API
    const_iterator end() const;

    HD_API
    bool Intersects(const HdDataSourceLocator &locator) const;
    HD_API
    bool Intersects(const HdDataSourceLocatorSet &locatorSet) const;
    HD_API
    bool IsEmpty() const;

private:
    void _InsertAndDeleteSuffixes(_Locators::iterator *position,
                                  const HdDataSourceLocator &locator);

    _Locators _locators;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_DATASOURCELOCATOR_H
