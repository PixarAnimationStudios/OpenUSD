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
#ifndef GLF_IMAGE_REGISTRY_H
#define GLF_IMAGE_REGISTRY_H

#include "pxr/imaging/glf/api.h"

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/token.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

typedef boost::shared_ptr<class GlfImage> GlfImageSharedPtr;

class GlfRankedTypeMap;

/// \class GlfImageRegistry
///
/// Manages plugin registration and loading for GlfImage subclasses.
///
class GlfImageRegistry : public TfSingleton<GlfImageRegistry> {
public:
    static GlfImageRegistry& GetInstance() {
        return TfSingleton<GlfImageRegistry>::GetInstance();
    }

    bool IsSupportedImageFile(std::string const & filename);

private:
    friend class TfSingleton<GlfImageRegistry>;
    GlfImageRegistry();

    void _RegisterPluginForType(TfToken const & fileExtension,
                                TfType const & type,
                                int precedence);

protected:
    friend class GlfImage;

    GlfImageSharedPtr _ConstructImage(std::string const & filename);

private:
    friend class TfSingleton<GlfImageRegistry>;
    boost::scoped_ptr<GlfRankedTypeMap> _typeMap;
};

#endif // GLF_IMAGE_REGISTRY_H
