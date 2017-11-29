//
// Copyright 2017 Pixar
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
#ifndef _GUSD_STAGEOPTS_H_
#define _GUSD_STAGEOPTS_H_

#include <SYS/SYS_Hash.h>

#include "gusd/api.h"

#include <pxr/pxr.h>
#include "pxr/usd/usd/stage.h"


PXR_NAMESPACE_OPEN_SCOPE


/// Options for configuring creation of a new stage.
/// This currently just includes the initial load set,
/// but may include other options in the future.
class GUSD_API GusdStageOpts
{
public:
    using InitialLoadSet = UsdStage::InitialLoadSet;

    GusdStageOpts(const GusdStageOpts& o) = default;

    GusdStageOpts(InitialLoadSet loadSet=UsdStage::LoadAll)
        : _loadSet(loadSet) {}

    /// Return options that a configure a stage to be loaded with payloads.
    static GusdStageOpts    LoadAll()
                            { return GusdStageOpts(UsdStage::LoadAll); }

    /// Return options that a configure a stage to be loaded without payloads.
    static GusdStageOpts    LoadNone()
                            { return GusdStageOpts(UsdStage::LoadNone); }

    InitialLoadSet          GetLoadSet() const
                            { return _loadSet; }

    void                    SetLoadSet(InitialLoadSet loadSet)
                            { _loadSet = loadSet; }

    size_t                  GetHash() const
                            { return SYShash(_loadSet); }

    bool                    operator==(const GusdStageOpts& o) const
                            { return _loadSet == o._loadSet; }
  
private:
    InitialLoadSet _loadSet;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif /*_GUSD_STAGEOPTS_H_*/
