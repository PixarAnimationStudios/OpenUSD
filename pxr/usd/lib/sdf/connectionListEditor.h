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
#ifndef SDF_CONNECTION_LIST_EDITOR_H
#define SDF_CONNECTION_LIST_EDITOR_H

#include "pxr/usd/sdf/listOpListEditor.h"

#include "pxr/usd/sdf/childrenPolicies.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/proxyPolicies.h"
#include "pxr/usd/sdf/types.h"

SDF_DECLARE_HANDLES(SdfSpec);
class TfToken;

/// \class Sdf_ConnectionListEditor
///
/// List editor implementation that ensures that the appropriate target
/// specs are created or destroyed when connection/relationship targets are 
/// added to the underlying list operation.
///
template <class ConnectionChildPolicy>
class Sdf_ConnectionListEditor 
    : public Sdf_ListOpListEditor<SdfPathKeyPolicy>
{
protected:
    virtual ~Sdf_ConnectionListEditor();

    Sdf_ConnectionListEditor(
        const SdfSpecHandle& connectionOwner,
        const TfToken& connectionListField,             
        const SdfPathKeyPolicy& typePolicy = SdfPathKeyPolicy());
    
    void _OnEdit(SdfListOpType op,
        SdfSpecType specType,
        const std::vector<SdfPath>& oldItems,
        const std::vector<SdfPath>& newItems) const;

private:
    typedef Sdf_ListOpListEditor<SdfPathKeyPolicy> Parent;
};

/// \class Sdf_AttributeConnectionListEditor
///
/// List editor implementation for attribute connections.
///
class Sdf_AttributeConnectionListEditor
    : public Sdf_ConnectionListEditor<Sdf_AttributeConnectionChildPolicy>
{
public:
    virtual ~Sdf_AttributeConnectionListEditor();

    Sdf_AttributeConnectionListEditor(
        const SdfSpecHandle& owner,
        const SdfPathKeyPolicy& typePolicy = SdfPathKeyPolicy());

    virtual void _OnEdit(SdfListOpType op,
        const std::vector<SdfPath>& oldItems,
        const std::vector<SdfPath>& newItems) const;
private:
    typedef Sdf_ConnectionListEditor<Sdf_AttributeConnectionChildPolicy> Parent;
};

/// \class Sdf_RelationshipTargetListEditor
///
/// List editor implementation for attribute connections.
///
class Sdf_RelationshipTargetListEditor
    : public Sdf_ConnectionListEditor<Sdf_RelationshipTargetChildPolicy>
{
public:
    virtual ~Sdf_RelationshipTargetListEditor();

    Sdf_RelationshipTargetListEditor(
        const SdfSpecHandle& owner,
        const SdfPathKeyPolicy& typePolicy = SdfPathKeyPolicy());

    virtual void _OnEdit(SdfListOpType op,
        const std::vector<SdfPath>& oldItems,
        const std::vector<SdfPath>& newItems) const;
private:
    typedef Sdf_ConnectionListEditor<Sdf_RelationshipTargetChildPolicy> Parent;
};

#endif // SDF_CONNECTION_LIST_EDITOR_H
