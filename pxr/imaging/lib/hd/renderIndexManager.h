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
#ifndef HD_RENDER_INDEX_MANAGER_H
#define HD_RENDER_INDEX_MANAGER_H

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"

#include <memory>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


class HdRenderIndex;

///
/// Provides a global collection of all render indexes that have been
/// created.
///
class Hd_RenderIndexManager final {
public:
    ///
    /// Returns the singleton for \c Hd_RenderIndexManager
    ///
    static Hd_RenderIndexManager &GetInstance();

    ///
    /// Create a new Render Index.
    ///
    HdRenderIndex *CreateRenderIndex();

    ///
    /// Increment reference count on a render index.
    ///
    bool AddRenderIndexReference(HdRenderIndex *renderIndex);

    ///
    /// Decrement reference count on a render index, if no longer in use
    /// the memory gets freed.
    ///
    void ReleaseRenderIndex(HdRenderIndex *renderIndex);

private:
    // Friend required by TfSingleton to access constructor (as it is private).
    friend class TfSingleton<Hd_RenderIndexManager>;

    // Key == Render Index Pointer
    // Value == Ref Count.
    typedef std::unordered_map<HdRenderIndex *, int> RenderIndexMap;

    RenderIndexMap _renderIndexes;

    // Singleton gets private constructed
    Hd_RenderIndexManager();
    ~Hd_RenderIndexManager();

    ///
    /// This class is not intended to be copied.
    ///
    Hd_RenderIndexManager(const Hd_RenderIndexManager &)             = delete;
    Hd_RenderIndexManager &operator =(const Hd_RenderIndexManager &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_RENDER_INDEX_MANAGER_H
