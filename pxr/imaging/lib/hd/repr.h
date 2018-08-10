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
#ifndef HD_REPR_H
#define HD_REPR_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/imaging/hd/drawItem.h"
#include <vector>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


struct HdRprimSharedData;

/// \class HdReprSelector
///
/// Describes one of more authored rprim display representations.
/// Display opinions are separated by the topology index they represent.
/// This allows the application to specify one or more topological
/// representations for a given HdRprim. For some visualizations it is
/// important to allow the application to provide an opinion for the display
/// of the refined surface, the unrefined hull, and the points separately from
/// the Rprim's authored opinions. This allows the representations to merge
/// and create a final composite representation to be used for rendering.
class HdReprSelector {
public:
    explicit HdReprSelector()
    : refinedToken()
    , unrefinedToken()
    , pointsToken() { }

    explicit HdReprSelector(TfToken const &token)
    : refinedToken(token)
    , unrefinedToken()
    , pointsToken() { }

    explicit HdReprSelector(
        TfToken const &refined,
        TfToken const &unrefined)
    : refinedToken(refined)
    , unrefinedToken(unrefined)
    , pointsToken() { }

    explicit HdReprSelector(
        TfToken const &refined,
        TfToken const &unrefined,
        TfToken const &points)
    : refinedToken(refined)
    , unrefinedToken(unrefined)
    , pointsToken(points) { }
    
    /// Returns true if the passed in reprToken is in the set of tokens
    /// for any topology index.
    HD_API
    bool Contains(TfToken reprToken) const;

    /// Returns true if all the tokens for all the topologies are empty.
    HD_API
    bool IsEmpty() const;
    
    /// Returns a selector that is the composite of this selector 'over'
    /// the passed in selector.
    /// For each token that IsEmpty in this selector return the corresponding
    /// token in the passed in selector.
    /// Effectively this performs a merge operation where this selector wins
    /// for each topological index it has an opinion on.
    HD_API
    HdReprSelector CompositeOver(const HdReprSelector &under) const;

    HD_API    
    bool operator==(const HdReprSelector &rhs) const;

    HD_API
    bool operator!=(const HdReprSelector &rhs) const;
    
    HD_API
    bool operator<(const HdReprSelector &rhs) const;
    
    HD_API
    size_t Hash() const;

    HD_API    
    char const* GetText() const;

    HD_API    
    friend std::ostream &operator <<(std::ostream &stream,
                                     HdReprSelector const& t);
    
    HD_API
    size_t size() const;

    HD_API
    TfToken const &operator[](int index) const;
    
private:
    TfToken refinedToken;
    TfToken unrefinedToken;
    TfToken pointsToken;
};

/// \class HdRepr
///
/// An HdRepr owns one or more draw item(s) that visually represent it.
///
/// The relevant compositional hierarchy is:
/// 
/// HdRprim
///  |
///  +--HdRepr(s)
///       |
///       +--HdDrawItem(s)
///  
/// For example, an HdMesh rprim could have a "Surface + Hull" repr with two 
/// draw items; one to draw its subdivided surface and another to draw its 
/// hull lines.
///
class HdRepr {
public:
    typedef std::vector<HdDrawItem*> DrawItems;

    HD_API
    HdRepr();
    HD_API
    virtual ~HdRepr();

    // Noncopyable
    HdRepr(const HdRepr&) = delete;
    HdRepr& operator=(const HdRepr&) = delete;

    /// Returns the draw items for this representation.
    HD_API
    const DrawItems& GetDrawItems() {
        return _drawItems;
    }

    /// Transfers ownership of a draw item to this repr.
    HD_API
    void AddDrawItem(HdDrawItem *item) {
        _drawItems.push_back(item);
    }

    /// Returns the draw item at the requested index.
    ///
    /// Note that the pointer returned is owned by this object and must not be
    /// deleted.
    HD_API
    HdDrawItem* GetDrawItem(size_t index) {
        return _drawItems[index];
    }

private:
    DrawItems _drawItems;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_REPR_H
