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
#include "GEO_IOTranslator.h"

#include "GU_PackedUSD.h"

#include <GU/GU_Detail.h>
#include <UT/UT_IStream.h>
#include <CH/CH_Manager.h>

PXR_NAMESPACE_OPEN_SCOPE

// drand48 and srand48 defined in SYS_Math.h as of 13.5.153. and conflicts with imath.
#undef drand48
#undef srand48

using std::cerr;
using std::endl;

//##############################################################################
// class GusdGEO_IOTranslator implementation
//##############################################################################
GusdGEO_IOTranslator::
GusdGEO_IOTranslator()
{}


GusdGEO_IOTranslator::
~GusdGEO_IOTranslator()
{}


GEO_IOTranslator* GusdGEO_IOTranslator::
duplicate() const
{
    return new GusdGEO_IOTranslator(*this);
}


const char* GusdGEO_IOTranslator::
formatName() const
{
    return "Universal Scene Description";
}


int GusdGEO_IOTranslator::
checkExtension(const char* name)
{
    UT_String nameStr(name);
    if(nameStr.fileExtension()
     &&  (!strcmp(nameStr.fileExtension(), ".usd")
       || !strcmp(nameStr.fileExtension(), ".usda")
       || !strcmp(nameStr.fileExtension(), ".usdc"))) {
        return true;
    }
    return false;
}


int GusdGEO_IOTranslator::
checkMagicNumber(unsigned /*number*/)
{
    return 0;
}


GA_Detail::IOStatus GusdGEO_IOTranslator::
fileLoad(GEO_Detail* gdp, UT_IStream& is, bool ate_magic)
{
    UT_WorkBuffer buffer;
    if(!is.isRandomAccessFile(buffer)) {
        return GA_Detail::IOStatus(false);
    }

    const std::string fileName = buffer.toStdString();
    auto stage = UsdStage::Open( fileName, UsdStage::LoadNone);
    if( !stage ) {
        return GA_Detail::IOStatus( false );
    }

    float f = CHgetSampleFromTime( CHgetEvalTime() );

    GU_Detail* detail = dynamic_cast<GU_Detail *>(gdp); 
    if( !detail ) {
        return GA_Detail::IOStatus( false );
    }

    // If the file contains a default prim, load that,  otherwise load 
    // all the top level prims.
    auto defPrim = stage->GetDefaultPrim();
    if(  defPrim ) {
        GusdGU_PackedUSD::Build(*detail, fileName, defPrim.GetPath(), f, NULL,
	    GusdPurposeSet(GUSD_PURPOSE_DEFAULT | GUSD_PURPOSE_PROXY));
    }
    else {
        for( const auto &child : stage->GetPseudoRoot().GetChildren() ) {
            GusdGU_PackedUSD::Build(*detail, fileName, child.GetPath(), f, NULL,
		GusdPurposeSet(GUSD_PURPOSE_DEFAULT | GUSD_PURPOSE_PROXY));
        }
    }

    return GA_Detail::IOStatus(true);
}


GA_Detail::IOStatus GusdGEO_IOTranslator::
fileSave(const GEO_Detail*, std::ostream&)
{
    return 0;
}

PXR_NAMESPACE_CLOSE_SCOPE

//##############################################################################


