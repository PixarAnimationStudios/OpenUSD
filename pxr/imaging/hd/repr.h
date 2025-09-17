//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_REPR_H
#define PXR_IMAGING_HD_REPR_H

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
class HdReprSelector
{
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
    // TfHash support.
    template <class HashState>
    friend void
    TfHashAppend(HashState &h, HdReprSelector const &rs) {
        h.Append(rs.refinedToken, rs.unrefinedToken, rs.pointsToken);
    }

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
class HdRepr final
{
public:
    using DrawItemUniquePtr = std::unique_ptr<HdDrawItem>;
    using DrawItemUniquePtrVector = std::vector<DrawItemUniquePtr>;

    HD_API
    HdRepr();
    HD_API
    ~HdRepr();

    /// Returns the draw items for this representation.
    const DrawItemUniquePtrVector& GetDrawItems() const {
        return _drawItems;
    }

    /// Transfers ownership of a draw item to this repr.
    /// Do not use for adding geom subset draw items.
    void AddDrawItem(std::unique_ptr<HdDrawItem> &&item) {
        _drawItems.insert(_drawItems.begin() + _geomSubsetsStart, 
            std::move(item));
        _geomSubsetsStart++;
    }

    /// Returns the draw item at the requested index.
    ///
    /// Note that the pointer returned is owned by this object and must not be
    /// deleted.
    HdDrawItem* GetDrawItem(size_t index) const {
        return _drawItems[index].get();
    }

    /// HdRepr can hold geom subset draw items, which are unique in that they
    /// not created at the time the repr is created, but instead when populating
    /// the mesh topology of a mesh rprim. The number of geom subset draw items
    /// in a repr can change over time. 
    /// We make some assumptions when using these geom subset related functions.
    /// We assume the geom subset draw items will only be added (or cleared) 
    /// after all of the main draw items for a repr have been added.
    /// We also assume that the geom subset draw items for a repr desc are all
    /// added one after the other before moving onto the next repr desc. 
    /// Thus the order of draw items in the _drawItems member might go something
    /// like (assuming two repr descs and three geom subsets in this example):
    /// [ main DI for desc 1, main DI for desc 2, GS1 DI for desc 1, 
    ///   GS2 DI for desc 1, GS3 DI for desc 1, GS1 DI for desc 2, 
    ///   GS2 DI for desc 2, GS3 DI for desc 2 ]
    /// It is also possible for there to exist a main draw item for a particular
    /// repr desc but no geom subsets for that repr desc, while having geom
    /// subsets exist for a different repr desc.

    /// Transfers ownership of a draw item to this repr.
    /// To be used only for geom subset draw items.
    void AddGeomSubsetDrawItem(std::unique_ptr<HdDrawItem> &&item) {
        _drawItems.push_back(std::move(item));
    }

    /// Utility similar to GetDrawItem for getting geom subset draw items.
    HdDrawItem* GetDrawItemForGeomSubset(size_t reprDescIndex, 
        size_t numGeomSubsets, size_t geomSubsetIndex) const {        
        return _drawItems[_geomSubsetsStart + reprDescIndex * numGeomSubsets + 
            geomSubsetIndex].get();
    }

    /// Removes all of the geom subset draw items from the repr.
    void ClearGeomSubsetDrawItems() {
        _drawItems.erase(
            _drawItems.begin() + _geomSubsetsStart, _drawItems.end());
    }

private:
    // Noncopyable
    HdRepr(const HdRepr&) = delete;
    HdRepr& operator=(const HdRepr&) = delete;

private:
    // Contains normal draw items first, potentially followed by geom subset
    // draw items
    DrawItemUniquePtrVector _drawItems;

    // Index into _drawItems indicating where the geom subset draw items begin.
    size_t _geomSubsetsStart; 
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_REPR_H
