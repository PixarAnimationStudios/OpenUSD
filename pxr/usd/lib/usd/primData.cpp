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
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"

#include "pxr/usd/kind/registry.h"

#include "pxr/base/tf/stringUtils.h"

#include <algorithm>
#include <sstream>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Static assertion on PrimData size.  We want to be warned when its size
// changes.
static_assert(sizeof(Usd_PrimData) == 64,
              "Expected sizeof(Usd_PrimData) == 64");

Usd_PrimData::Usd_PrimData(UsdStage *stage, const SdfPath& path)
    : _stage(stage)
    , _primIndex(NULL)
    , _path(path)
    , _firstChild(NULL)
    , _refCount(0)
{
    if (!stage)
        TF_FATAL_ERROR("Attempted to construct with null stage");

    TF_DEBUG(USD_PRIM_LIFETIMES).Msg(
        "Usd_PrimData::ctor<%s,%s,%s>\n",
        _typeName.GetText(), path.GetText(),
        _stage->GetRootLayer()->GetIdentifier().c_str());
}

Usd_PrimData::~Usd_PrimData() {
    TF_DEBUG(USD_PRIM_LIFETIMES).Msg(
        "~Usd_PrimData::dtor<%s,%s,%s>\n",
        _typeName.GetText(), _path.GetText(),
        _stage ? _stage->GetRootLayer()->GetIdentifier().c_str() :
        "prim is invalid/expired");
}

Usd_PrimDataConstPtr
Usd_PrimData::GetParent() const
{
    if (Usd_PrimDataPtr parentLink = GetParentLink())
        return parentLink;

    SdfPath parent = _path.GetParentPath();
    return parent == SdfPath::EmptyPath() ?
        NULL : _stage->_GetPrimDataAtPath(parent);
}

const PcpPrimIndex &
Usd_PrimData::GetPrimIndex() const
{
    static const PcpPrimIndex dummyPrimIndex;
    return ARCH_UNLIKELY(IsMaster()) ? dummyPrimIndex : *_primIndex;
}

const PcpPrimIndex &
Usd_PrimData::GetSourcePrimIndex() const
{
    TF_AXIOM(_primIndex);
    return *_primIndex;
}

SdfSpecifier
Usd_PrimData::GetSpecifier() const
{
    return _stage->_GetSpecifier(this);
}

void
Usd_PrimData::_ComposeAndCacheFlags(Usd_PrimDataConstPtr parent, 
                                    bool isMasterPrim)
{
    // We do not have to clear _flags here since in the pseudo root or instance
    // master case the values never change, and in the ordinary prim case we set
    // every flag.

    // Special-case the root (the only prim which has no parent) and
    // instancing masters.
    if (ARCH_UNLIKELY(!parent || isMasterPrim)) {
        _flags[Usd_PrimActiveFlag] = true;
        _flags[Usd_PrimLoadedFlag] = true;
        _flags[Usd_PrimModelFlag] = true;
        _flags[Usd_PrimGroupFlag] = true;
        _flags[Usd_PrimDefinedFlag] = true;
        _flags[Usd_PrimMasterFlag] = isMasterPrim;
    } 
    else {
        // Compose and cache 'active'.
        UsdPrim self(Usd_PrimDataIPtr(this), _path);
        bool active = true;
        self.GetMetadata(SdfFieldKeys->Active, &active);
        _flags[Usd_PrimActiveFlag] = active;

        // Cache whether or not this prim has a payload.
        bool hasPayload = _primIndex->HasPayload();
        _flags[Usd_PrimHasPayloadFlag] = hasPayload;

        // An active prim is loaded if it's loadable and in the load set, or
        // it's not loadable and its parent is loaded.
        _flags[Usd_PrimLoadedFlag] = active &&
            (hasPayload ?
             _stage->_GetPcpCache()->IsPayloadIncluded(_primIndex->GetPath()) :
             parent->IsLoaded());

        // According to Model hierarchy rules, only Model Groups may have Model
        // children (groups or otherwise).  So if our parent is not a Model
        // Group, then this prim cannot be a model (or a model group).
        // Otherwise we look up the kind metadata and consult the kind registry.
        bool isGroup = false, isModel = false;
        if (parent->IsGroup()) {
            static TfToken kindToken("kind");
            TfToken kind;
            self.GetMetadata(kindToken, &kind);
            // Use the kind registry to determine model/groupness.
            if (!kind.IsEmpty()) {
                isGroup = KindRegistry::IsA(kind, KindTokens->group);
                isModel = isGroup || KindRegistry::IsA(kind, KindTokens->model);
            }
        }
        _flags[Usd_PrimGroupFlag] = isGroup;
        _flags[Usd_PrimModelFlag] = isModel;

        // Get specifier.
        SdfSpecifier specifier = GetSpecifier();

        // This prim is abstract if its parent is or if it's a class.
        _flags[Usd_PrimAbstractFlag] =
            parent->IsAbstract() || specifier == SdfSpecifierClass;

        // Cache whether or not this prim has an authored defining specifier.
        const bool isDefiningSpec = SdfIsDefiningSpecifier(specifier);
        _flags[Usd_PrimHasDefiningSpecifierFlag] = isDefiningSpec;

        // This prim is defined if its parent is and its specifier is defining.
        _flags[Usd_PrimDefinedFlag] = isDefiningSpec && parent->IsDefined();

        // The presence of clips that may affect attributes on this prim
        // is computed and set in UsdStage. Default to false.
        _flags[Usd_PrimClipsFlag] = false;

        // These flags indicate whether this prim is an instance or an
        // instance master.
        _flags[Usd_PrimInstanceFlag] = active && _primIndex->IsInstanceable();
        _flags[Usd_PrimMasterFlag] = parent->IsInMaster();
    }
}

Usd_PrimDataConstPtr 
Usd_PrimData::GetPrimDataAtPathOrInMaster(const SdfPath &path) const
{
    return _stage->_GetPrimDataAtPathOrInMaster(path);
}

Usd_PrimDataConstPtr 
Usd_PrimData::GetMaster() const
{
    return _stage->_GetMasterForInstance(this);
}

bool
Usd_PrimData::_ComposePrimChildNames(TfTokenVector* nameOrder)
{
    // TODO: would be nice to not compute the name order until it is needed
    // TODO: What do we do with prohibitedNames?
    PcpTokenSet prohibitedNames;
    GetSourcePrimIndex().ComputePrimChildNames(nameOrder, &prohibitedNames);
    return true;
}

std::string
Usd_DescribePrimData(const Usd_PrimData *p)
{
    if (!p)
        return "null prim";

    return TfStringPrintf(
        "%s%sprim <%s> %s",
        Usd_IsDead(p) ? "expired " : (p->_flags[Usd_PrimActiveFlag] ?
                                      "" : "inactive "),
        p->_typeName.IsEmpty() ? "" :
            TfStringPrintf("'%s' ", p->_typeName.GetText()).c_str(),
        p->_path.GetText(),
        p->_stage ? TfStringPrintf(
            "on stage %s", UsdDescribe(p->_stage).c_str()).c_str() : "");
}

void
Usd_IssueFatalPrimAccessError(const Usd_PrimData *p)
{
    TF_FATAL_ERROR("Used %s", Usd_DescribePrimData(p).c_str());
}


PXR_NAMESPACE_CLOSE_SCOPE

