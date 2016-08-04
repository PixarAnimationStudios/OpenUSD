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
#ifndef SDF_CHANGEBLOCK_H
#define SDF_CHANGEBLOCK_H

#include "pxr/usd/sdf/api.h"

/// \class SdfChangeBlock
///
/// <b>DANGER DANGER DANGER</b>
///
/// Please make sure you have read and fully understand the
/// issues below before using a changeblock!  They are very
/// easy to use in an unsafe way that could make the system
/// crash or corrupt data.  If you have any questions, please
/// contact the USD team, who would be happy to help!
///
/// SdfChangeBlock provides a way to group a round of related changes to
/// scene description in order to process them more efficiently.
///
/// Normally, Sdf sends notification immediately as changes are made so
/// that downstream representations in Csd and Mf can update accordingly.
///
/// However, sometimes it can be advantageous to group a series of Sdf
/// changes into a batch so that they can be processed more efficiently,
/// with a single round of change processing.  An example might be when
/// setting many avar values on a model at the same time.
///
/// Opening a changeblock tells Sdf to delay sending notification about
/// changes until the outermost changeblock is exited.  Until then,
/// Sdf internally queues up the notification it needs to send.
///
/// \note  It is *not* safe to use Csd, Mf, or other downstream API
/// while a changeblock is open!!111  This is because those derived
/// representations will not have had a chance to update while the
/// changeblock is open.  Not only will their view of the world be
/// stale, it could be unsafe to even make queries from, since they
/// may be holding onto expired handles to Sdf objects that no longer
/// exist.
///
/// If you need to make a bunch of changes to scene description,
/// the best approach is to build a list of necessary changes that
/// can be performed directly via the Sdf API, then submit those all
/// inside a changeblock without talking to any downstream libraries.
/// For example, this is how Csd performs namespace edits.
///
class SdfChangeBlock {
public:
    SDF_API
    SdfChangeBlock();
    SDF_API
    ~SdfChangeBlock();
};

#endif // SDF_CHANGEBLOCK_H
