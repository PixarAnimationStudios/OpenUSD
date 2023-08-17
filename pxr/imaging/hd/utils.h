//
// Copyright 2023 Pixar
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

#ifndef PXR_IMAGING_HD_UTILS_H
#define PXR_IMAGING_HD_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include "pxr/base/tf/declarePtrs.h"

#include <memory>
#include <string>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdSceneIndexBase);

class SdfPath;

namespace HdUtils {

/// A simple facility to associate an application object managed by
/// std::shared_ptr with a render instance id.
///
/// This is useful when using the scene index callback registration facility.
/// The callback is registered only once, but may be invoked each time the
/// scene index graph is created (this currently happens during the render index
/// construction).
/// Futhermore, an application may spawn several render index instances and thus
/// the (same) callback may be invoked several times, necessitating a way to
/// map the callback back to the associated scene index instance.
///
/// The \name RenderInstanceTracker facility below provides a simple way
/// to register, unregister and query an object that is tied to a render
/// instance id, which is provided as a callback argument.
///
/// \note
/// The \em RegisterInstance method should be invoked before the scene index
/// callback is invoked (i.e., prior to render index construction).
///
/// The \em UnregisterInstance method is typically invoked prior to render
/// index destruction.
/// 
/// \note This facility isn't thread-safe.
///
/// \sa HdSceneIndexPluginRegistry::SceneIndexAppendCallback
/// \sa HdSceneIndexPluginRegistry::RegisterSceneIndexForRenderer
///
template <typename T>
class RenderInstanceTracker
{
public:
    using TWeakPtr = std::weak_ptr<T>;
    using TSharedPtr = std::shared_ptr<T>;

    void RegisterInstance(
        std::string const &renderInstanceId,
        TSharedPtr const &sp)
    {
        if (!sp) {
            return;
        }

        auto res = idInstanceMap.insert({renderInstanceId, sp});
        if (!res.second) { // wasn't inserted
            TWeakPtr &wp = res.first->second;
            if (auto handle = wp.lock()) {
                // Found entry with valid handle. This can happen if the
                // renderInstanceId isn't unique enough. Leave the existing
                // entry as-is.
                TF_WARN(
                    "An instance with renderInstanceId %s was already "
                    "registered previously.", renderInstanceId.c_str());
                return;
            }
            res.first->second = sp;
        }
    }

    void UnregisterInstance(
        std::string const &renderInstanceId)
    {
        idInstanceMap.erase(renderInstanceId);
    }

    TSharedPtr GetInstance(
        std::string const &id)
    {
        const auto it = idInstanceMap.find(id);
        if (it != idInstanceMap.end()) {
            if (TSharedPtr sp = it->second.lock()) {
                return sp;
            }
        }
        return nullptr;
    }
    
private:
    // Use a weak reference to the object.
    using _IdToInstanceMap = std::unordered_map<std::string, TWeakPtr>;
    _IdToInstanceMap idInstanceMap;
};

/// Retreives the active render settings prim path from the input scene index
/// \p si. Returns true if a data source for the associated locator was found
/// with the result in \p primPath, and false otherwise.
///
HD_API
bool
HasActiveRenderSettingsPrim(
    const HdSceneIndexBaseRefPtr &si,
    SdfPath *primPath = nullptr);

}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_UTILS_H
