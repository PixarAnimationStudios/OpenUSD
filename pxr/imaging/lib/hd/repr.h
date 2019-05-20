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
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/tokens.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdReprSelector
///
/// Describes one or more authored display representations for an rprim.
/// Display opinions are separated by the topology index they represent.
/// This allows the application to specify one or more topological
/// representations for a given HdRprim.
/// For some visualizations, an application may choose to provide an opinion for
/// the display of the refined surface, the unrefined hull and the points
/// separately from the rprim's authored opinions.
/// HdReprSelector allows these opinions to compose/merge into a final composite
/// representation to be used for rendering.
///
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

    /// Currenly support upto 3 topology tokens.
    static const size_t MAX_TOPOLOGY_REPRS = 3;

    /// Returns true if the passed in reprToken is in the set of tokens
    /// for any topology index.
    HD_API
    bool Contains(const TfToken &reprToken) const;

    /// Returns true if the topology token at an index is active, i.e., neither
    /// empty nor disabled.
    HD_API
    bool IsActiveRepr(size_t topologyIndex) const;

    /// Returns true if any of the topology tokens is valid, i.e., neither
    /// empty nor disabled.
    HD_API
    bool AnyActiveRepr() const;
    
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
    TfToken const &operator[](size_t topologyIndex) const;
    
private:
    TfToken refinedToken;
    TfToken unrefinedToken;
    TfToken pointsToken;
};

/// \class HdRepr
///
/// An HdRepr refers to a (single) topological representation of an rprim, and 
/// owns the draw item(s) that visually represent it. The draw items are
/// populated by the rprim.
/// The relevant compositional hierarchy is:
/// 
/// HdRprim
///  |
///  +--HdRepr(s)
///       |
///       +--HdDrawItem(s)
/// 
/// When multiple topological representations are required for an rprim, we use
/// HdReprSelector to compose the individual representations.
///
class HdRepr {
public:
    typedef std::vector<HdDrawItem*> DrawItems;

    HD_API
    HdRepr();
    HD_API
    virtual ~HdRepr();

    /// Returns the draw items for this representation.
    const DrawItems& GetDrawItems() {
        return _drawItems;
    }

    /// Transfers ownership of a draw item to this repr.
    void AddDrawItem(HdDrawItem *item) {
        _drawItems.push_back(item);
    }

    /// Returns the draw item at the requested index.
    ///
    /// Note that the pointer returned is owned by this object and must not be
    /// deleted.
    HdDrawItem* GetDrawItem(size_t index) {
        return _drawItems[index];
    }

private:
    // Noncopyable
    HdRepr(const HdRepr&) = delete;
    HdRepr& operator=(const HdRepr&) = delete;

private:
    DrawItems _drawItems;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_REPR_H
