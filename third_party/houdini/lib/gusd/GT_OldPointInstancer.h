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
#ifndef __GUSD_GT_OLDPOINTINSTANCER_H__
#define __GUSD_GT_OLDPOINTINSTANCER_H__

#include <GT/GT_PrimPointMesh.h>

/// A GusdGT_PointInstancer is identical to a GT_PrimPointMesh except
/// that it is treated differently by the GusdRefiner and has
/// a different GusdPrimWrapper.

class GusdGT_OldPointInstancer : public GT_PrimPointMesh
{
public:
    GusdGT_OldPointInstancer() {}
    GusdGT_OldPointInstancer(const GT_AttributeListHandle &points,
                          const GT_AttributeListHandle &uniform) :
        GT_PrimPointMesh( points, uniform )
    {
    }
    GusdGT_OldPointInstancer(const GusdGT_OldPointInstancer &src) :
        GT_PrimPointMesh( src )
    {
    }
    virtual ~GusdGT_OldPointInstancer();

    virtual const char *className() const override { return "GusdGT_OldPointInstancer"; }

    static int getStaticPrimitiveType();

    virtual int getPrimitiveType() const override;
};

#endif // __GUSD_GT_OLDPOINTINSTANCER_H__