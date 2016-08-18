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
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>
#include <boost/python.hpp>

#include "usdMaya/editUtil.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyEnum.h"

#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MFnAssembly.h>

using namespace std;
using namespace boost::python;
using namespace boost;

#define BOOST_PYTHON_NONE boost::python::object()

static boost::python::object
_GetEditFromString(
    const std::string &assemblyPath,
    const std::string &editString )
{
    MSelectionList selList;
    selList.add( MString(assemblyPath.c_str()) );

    MObject assemblyObj;
    if( !selList.getDependNode(0, assemblyObj) )
    {
        TF_CODING_ERROR( "EditUtil.GetEditFromString:"
                         " assembly dag path expected, not found!" );
        return BOOST_PYTHON_NONE;
    }
    
    MStatus status;
    MFnAssembly assemblyFn(assemblyObj,&status);
    if( !status )
    {
        TF_CODING_ERROR( "EditUtil.GetEditFromString:"
                         " assembly dag path expected, not found!" );
        return BOOST_PYTHON_NONE;
    }
    
    SdfPath editPath;
    PxrUsdMayaEditUtil::RefEdit refEdit;
    if( !PxrUsdMayaEditUtil::GetEditFromString( assemblyFn, editString,
                                                &editPath, &refEdit) )
    {
        TF_CODING_ERROR( "EditUtil.GetEditFromString: invalid edit" );
        return BOOST_PYTHON_NONE;
    }
    
    return boost::python::make_tuple(editPath,refEdit);
}

static boost::python::object
_GetEditsForAssembly(
    const std::string &assemblyPath )
{
    PxrUsdMayaEditUtil::PathEditMap refEdits;
    std::vector< std::string > invalidEdits;

    MSelectionList selList;
    selList.add( MString(assemblyPath.c_str()) );

    MObject assemblyObj;
    if( !selList.getDependNode(0, assemblyObj) )
    {
        TF_CODING_ERROR( "EditUtil.GetEditsForAssembly:"
                         " assembly dag path expected, not found!" );
        return BOOST_PYTHON_NONE;
    }
    
    PxrUsdMayaEditUtil::GetEditsForAssembly(assemblyObj,&refEdits,&invalidEdits);
    
    boost::python::dict editDict;
    TF_FOR_ALL( pathEdits, refEdits )
    {
        boost::python::list editList;
        TF_FOR_ALL( edit, pathEdits->second )
        {
            editList.append( *edit );
        }
        
        editDict[ pathEdits->first ] = editList;
    }

    return boost::python::make_tuple(editDict,invalidEdits);
}

static bool
_GetRefEditsFromDict(
    boost::python::dict &refEditDict,
    PxrUsdMayaEditUtil::PathEditMap *refEdits )
{
    boost::python::list keys = refEditDict.keys();  
    for( int i=0; i<len(keys); i++ )
    {
        boost::python::extract<SdfPath> extractedKey(keys[i]);
        if( !extractedKey.check() )
        {
            TF_CODING_ERROR( "EditUtil.ApplyEditsToProxy:"
                             " SdfPath key expected, not found!" );
            return false;
        }
        else
        {
            SdfPath path = extractedKey;
            
            PxrUsdMayaEditUtil::RefEditVec pathEdits;
            
            boost::python::extract<boost::python::list>
                    extractedList( refEditDict[path] );
            
            if( !extractedList.check() )
            {
                TF_CODING_ERROR( "EditUtil.ApplyEditsToProxy:"
                                 " list value expected, not found!" );
                return false;
            }
            else
            {
                boost::python::list editList = extractedList;
                for( int j=0; j<len(extractedList); j++ )
                {
                    boost::python::extract<PxrUsdMayaEditUtil::RefEdit>
                            extractedEdit(editList[j]);
                    
                    if( !extractedEdit.check() )
                    {
                        TF_CODING_ERROR( "EditUtil.ApplyEditsToProxy:"
                                         " RefEdit expected in list, not found!");
                        return false;
                    }
                    else
                    {
                        pathEdits.push_back( extractedEdit );
                    }
                }
            }
            
            (*refEdits)[path] = pathEdits;
        }
    }
    
    return true;
}

static boost::python::object
_ApplyEditsToProxy(
    boost::python::dict &refEditDict,
    const UsdStagePtr &stage,
    const UsdPrim &proxyRootPrim )
{
    PxrUsdMayaEditUtil::PathEditMap refEdits;
    if( !_GetRefEditsFromDict( refEditDict, &refEdits ) )
        return BOOST_PYTHON_NONE;
   
    std::vector< std::string > failedEdits;
    
    PxrUsdMayaEditUtil::ApplyEditsToProxy(
            refEdits,stage,proxyRootPrim,&failedEdits);
    
    return boost::python::make_tuple(failedEdits.empty(),failedEdits);
}

static boost::python::object
_GetAvarEdits(
    boost::python::dict &refEditDict )
{
    PxrUsdMayaEditUtil::PathEditMap refEdits;
    if( !_GetRefEditsFromDict( refEditDict, &refEdits ) )
        return BOOST_PYTHON_NONE;
    
    PxrUsdMayaEditUtil::PathAvarMap avarMap;
    PxrUsdMayaEditUtil::GetAvarEdits( refEdits, &avarMap );
    
    boost::python::dict pathDict;
    TF_FOR_ALL( pathEdits, avarMap )
    {
        boost::python::dict valueMap;
        TF_FOR_ALL( avarEdit, pathEdits->second )
        {
            valueMap[avarEdit->first] = avarEdit->second;
        }
        
        pathDict[ pathEdits->first ] = valueMap;
    }

    return pathDict;
}

void wrapEditUtil()
{
    {
        scope EditUtil =
            class_< PxrUsdMayaEditUtil,
                    boost::noncopyable>("EditUtil", "UsdMaya edit utilities")
            .def("GetEditFromString",
                 &_GetEditFromString)
            .staticmethod("GetEditFromString")
            .def("GetEditsForAssembly",
                 &_GetEditsForAssembly)
            .staticmethod("GetEditsForAssembly")
            .def("ApplyEditsToProxy",
                 &_ApplyEditsToProxy)
            .staticmethod("ApplyEditsToProxy")
            .def("GetAvarEdits",
                 &_GetAvarEdits)
            .staticmethod("GetAvarEdits")
        ;
        
        enum_<PxrUsdMayaEditUtil::EditOp>("EditOp")
            .value("OP_TRANSLATE", PxrUsdMayaEditUtil::OP_TRANSLATE)
            .value("OP_ROTATE", PxrUsdMayaEditUtil::OP_ROTATE)
            .value("OP_SCALE", PxrUsdMayaEditUtil::OP_SCALE)
        ;
        
        enum_<PxrUsdMayaEditUtil::EditSet>("EditSet")
            .value("SET_ALL", PxrUsdMayaEditUtil::SET_ALL)
            .value("SET_X", PxrUsdMayaEditUtil::SET_X)
            .value("SET_Y", PxrUsdMayaEditUtil::SET_Y)
            .value("SET_Z", PxrUsdMayaEditUtil::SET_Z)
        ;
        
        typedef PxrUsdMayaEditUtil::RefEdit RefEdit;
        class_<RefEdit>("RefEdit", "Assembly edit")
            .def_readwrite("editString", &RefEdit::editString)
            .def_readwrite("op", &RefEdit::op)
            .def_readwrite("set", &RefEdit::set)
            .add_property("value",
                make_getter(&RefEdit::value,
                    return_value_policy<return_by_value>()),
                make_setter(&RefEdit::value))
        ;
    }
}
