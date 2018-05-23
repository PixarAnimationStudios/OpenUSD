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
#include "lightWrapper.h"
#include "lightFactory.h"

#include "context.h"
#include "UT_Gf.h"
#include "GU_USD.h"

#include <OP/OP_OperatorTable.h>
#include <OP/OP_Director.h>
#include <GU/GU_Detail.h>
#include <OP/OP_Operator.h>
#include <OP/OP_Node.h>
#include <OP/OP_Network.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Interrupt.h>
#include <SYS/SYS_Math.h>
#include <climits>


#include <GT/GT_RefineParms.h>
#include <GT/GT_Refine.h>
#include <GT/GT_TransformArray.h>
#include <GT/GT_PrimInstance.h>
#include <GT/GT_GEOPrimPacked.h>

PXR_NAMESPACE_OPEN_SCOPE

using std::cerr;
using std::endl;
using std::string;
using std::map;

#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

//static
std::string UsdLightWrapper::getParentNetworkPath(const UsdPrim& prim, const TransformMapping& transformMapping)
{
    auto parent = prim.GetParent();
    std::string parentPathStr = "";

    if (!parent || parent.IsPseudoRoot())
    {
        return "";
    }

    auto parentPath = parent.GetPath();
    if (transformMapping.find(parentPath) != transformMapping.end())
    {
        return transformMapping[parentPath];
    }

    parentPathStr = parentPath.GetString();
    if (parentPathStr.empty())
    {
        return parentPathStr;
    }

    return parentPathStr.substr(1, parentPathStr.size() - 1);
}

//static
OP_Network* UsdLightWrapper::findPrimParentNetwork(const UsdPrim& prim, const TransformMapping& transformMapping)
{
    auto root = getRootScene();
    std::string parentNetworkPath = getParentNetworkPath(prim, transformMapping);
    if (parentNetworkPath.empty())
    {
        return root;
    }
    auto node = root->findNode(parentNetworkPath.c_str());
    if (node)
    {
        return dynamic_cast<OP_Network*>(node);
    }
    return root;
}

// static
OP_Node* UsdLightWrapper::read(const UsdPrim& prim, const UT_String& overridePolicy, const bool useNetboxes, const TransformMapping& transformMapping)
{
    std::string primName = prim.GetName();
    auto parentNetwork = findPrimParentNetwork(prim, transformMapping);
    auto node = parentNetwork->findNode(primName.c_str());

    if (node)
    {
        // Light already exists
        // If we want to override it, set the node we read into to the old node
        if (overridePolicy == UT_String("overrideLight"))
        {
            if (parentNetwork->canDestroyNode(node))
            {
                parentNetwork->destroyNode(node);
            }
            else
            {
                // TODO: Rename and disable instead?
                std::cerr << "Node " << node->getName().c_str() << " cannot be destroyed.\n";
            }
            return LightFactory::create(prim, parentNetwork, useNetboxes);
        }
        if (overridePolicy == UT_String("skip"))
        {
            return node;
        }
        if (overridePolicy == UT_String("duplicate"))
        {
            return LightFactory::create(prim, parentNetwork, useNetboxes);
        }
        std::cerr << "[UsdLightWrapper::read] unknown policy " << overridePolicy.c_str() << std::endl;
        return node;
    }

    return LightFactory::create(prim, parentNetwork, useNetboxes);
}

// static
bool UsdLightWrapper::
canBeWritten(const OP_Node* node)
{
    return LightFactory::canBeWritten(node);
}

// static
UsdPrim UsdLightWrapper::
write(const UsdStagePtr& stage, OP_Node* node, const float time, const UsdTimeCode& timeCode)
{
    return LightFactory::create(stage, node, time, timeCode);
}

OP_Network*
UsdLightWrapper::getRootScene()
{
    return dynamic_cast<OP_Network*>(OPgetDirector()->findNode("/obj"));
}

PXR_NAMESPACE_CLOSE_SCOPE
