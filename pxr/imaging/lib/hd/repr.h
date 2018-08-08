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

/// \struct HdReprSelector
///
/// Describes one of more authored rprim display representations.
struct HdReprSelector {
    explicit HdReprSelector()
    : reprToken() { }
    explicit HdReprSelector(TfToken const &token)
    : reprToken(token) { }
    
    bool operator==(const HdReprSelector &rhs) const {
        return reprToken == rhs.reprToken;
    }

    bool operator!=(const HdReprSelector &rhs) const {
        return reprToken != rhs.reprToken;
    }
    
    bool operator<(const HdReprSelector &rhs) const {
        return reprToken < rhs.reprToken;
    }
    
    bool IsEmpty() const {
        return reprToken.IsEmpty();
    }

    size_t Hash() const { 
        return reprToken.Hash();
    }
    
    char const* GetText() const {
        return reprToken.GetText();
    }
    
    friend std::ostream &operator <<(std::ostream &stream,
                                     HdReprSelector const& t) {
        return stream << t.reprToken;
    }
    
    TfToken const &GetReprToken() const {
        return reprToken;
    }
    
private:
    TfToken reprToken;
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
