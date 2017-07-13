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
/**
   \file
   \brief Shared PRM objects.
*/
#ifndef __GUSD_PRM_SHARED_H__
#define __GUSD_PRM_SHARED_H__

#include <pxr/pxr.h>

#include "gusd/api.h"

#include <PRM/PRM_Include.h>
#include <PRM/PRM_SpareData.h>

PXR_NAMESPACE_OPEN_SCOPE

/** Common PRM data for USD.*/
class GusdPRM_Shared
{
public:
    GUSD_API
    GusdPRM_Shared();

    struct Components
    {
        Components();

        /** Pattern of all USD-backed extensions.*/
        UT_String       filePattern;

        /** Spare data with extension pattern for usd files.
            The different variations are for the read/write mode of the
            file chooser dialog.
            @{ */
        PRM_SpareData   usdFileROData;
        PRM_SpareData   usdFileRWData;
        PRM_SpareData   usdFileWOData;
        /** @} */

        PRM_Name        filePathName; /*! file */

        PRM_Name        primPathName; /*! primpath */

        /** Dynamic menu for selecting prim paths.
            Must be paired with a PRM_SpareData giving 'fileprm', whose value
            is the name of a string parm on the same prim, which gives the path
            to the tds file.
            @todo This currently uses a simple drop-down menu. When the new UI
            API access rolls out in the HDK, this should be updated to use a proper
            hierarchy UI, like the regular operator picker menus in Houdini.*/
        PRM_ChoiceList  primMenu;

        /** Variant of the above @em primMenu that can be used to
            pick multiple prims.*/
        PRM_ChoiceList  multiPrimMenu;

        /** Dynamic menu for selecting prim attributes.
            Like @a primMenu, must be paired with spair data giveing 'fileprm'
            as well as 'primpathprm'. Additional spare data 'primattrcondtiion'
            optionally provide conditional parm expressions for determining whether
            or not the attribute keys are included.*/
        PRM_ChoiceList  primAttrMenu;

        /** Has fileprm=>'fileprm', the common mapping for @a primMenu. */
        PRM_SpareData   fileParm;

        /** Has primpathprm=>'primpath', and fileprm=>'fileprm', the mapping
            commonly used for @a primAttrMenu. */
        PRM_SpareData   primAttrData;

        /** Multi-select menu for all UsdSchema-inherited types.*/
        PRM_ChoiceList  typesMenu;

        /** Multi-select menu for all model kinds.*/
        PRM_ChoiceList  modelKindsMenu;

        /** Multi-select menu for all imageable purposes.*/
        PRM_ChoiceList  purposesMenu;

        PRM_Default     pathAttrDefault;        //! usdpath
        PRM_Default     primPathAttrDefault;    //! usdprimpath
        PRM_Default     variantsAttrDefault;    //! usdvariants
    };

    Components*   operator->() const  { return &_components; }
    Components&   operator*() const   { return _components; }

private:
    Components& _components;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* __GUSD_PRM_PRMSHARED_H__ */
