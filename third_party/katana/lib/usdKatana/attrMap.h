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
#ifndef PXRUSDKATANA_ATTRMAP_H
#define PXRUSDKATANA_ATTRMAP_H

#include <FnAttribute/FnGroupBuilder.h>
#include <FnGeolib/op/FnGeolibOp.h>

/// \brief An object to store attributes.  The current implementation uses
/// a Foundry::Katana::GroupBuilder behind the scenes, but the dependence on
/// that thus far is somewhat minimal and not all of the behavior of
/// GroupBuilder has been exposed.
///
/// This class is here in case we need to have different behavior than the
/// GroupBuilder.
class PxrUsdKatanaAttrMap
{

public:

    /// \brief set \p attr at \p path.
    void set(const std::string& path, const Foundry::Katana::Attribute& attr);

    /// \brief delete attribute at \p path
    void del(const std::string& path);

    /// \brief build a group attribute
    FnAttribute::GroupAttribute build();

    /// \brief sets attrs in \p attrs onto the \p interface.
    void toInterface(Foundry::Katana::GeolibCookInterface& interface);

private:

    Foundry::Katana::GroupBuilder _groupBuilder;

};

#endif // PXRUSDKATANA_ATTRMAP_H
