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
#ifndef __GUSD_IOTRANSLATOR_H__
#define __GUSD_IOTRANSLATOR_H__

#include <pxr/pxr.h>

#include <GEO/GEO_IOTranslator.h>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

class GusdGEO_IOTranslator : public GEO_IOTranslator
{
public:
    GusdGEO_IOTranslator();
    virtual ~GusdGEO_IOTranslator();

    // GEO_IOTranslator interface ----------------------------------------------

public:
    virtual GEO_IOTranslator* duplicate() const;

    virtual const char* formatName() const;

    virtual int checkExtension(const char* name);
    
    virtual int checkMagicNumber(unsigned magic);

    virtual GA_Detail::IOStatus fileLoad(GEO_Detail*, UT_IStream&, bool ate_magic);

    virtual GA_Detail::IOStatus fileSave(const GEO_Detail*, std::ostream&);
    
    // -------------------------------------------------------------------------
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_IOTRANSLATOR_H__
