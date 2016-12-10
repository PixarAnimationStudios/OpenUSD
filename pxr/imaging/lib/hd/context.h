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
#ifndef HD_CONTEXT_H
#define HD_CONTEXT_H

#include <boost/intrusive/list.hpp>

typedef boost::intrusive::link_mode<boost::intrusive::auto_unlink>
                                                           Hd_ContextLinkOption;
typedef boost::intrusive::list_base_hook<Hd_ContextLinkOption>
                                                           Hd_ContextIListBase;

class HdRenderDelegate;
class GalDelegate;
class HdRenderIndex;

///
/// The HdContext class represents the combined state needed for rendering
/// an image of a scene.  What this represents is up to the application, a
/// context could represent a Window in an interactive application or it could
/// represent a worker thread in a batch processor.
///
class HdContext final : public Hd_ContextIListBase {
public:
    ///
    /// Initialize the context with the
    /// renderDelegate - Functionality to render the scene.
    /// galDelegate    - Image Presentation functionality (optional)
    /// index          - Scene to render
    ///
    HdContext(HdRenderDelegate *renderDelegate,
              GalDelegate      *galDelegate,
              HdRenderIndex    *index);
    virtual ~HdContext();


    ///
    /// Accessors
    ///
    HdRenderDelegate *GetRenderDelegate();
    GalDelegate      *GetGalDelegate();
    HdRenderIndex    *GetRenderIndex();

private:
    HdRenderDelegate *_renderDelegate;
    GalDelegate      *_galDelegate;
    HdRenderIndex    *_renderIndex;

    HdContext()                              = delete;
    HdContext(const HdContext &)             = delete;
    HdContext &operator =(const HdContext &) = delete;
};

#endif // HD_CONTEXT_H
