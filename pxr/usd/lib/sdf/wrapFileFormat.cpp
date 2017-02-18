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
///
/// \file Sdf/wrapFileFormat.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyStaticTokens.h"

#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// File format factory wrapper.  Python will give us a file format type
// and we must provide a factory to return instances of it.
class Sdf_PyFileFormatFactory : public Sdf_FileFormatFactoryBase {
public:
    typedef std::unique_ptr<Sdf_PyFileFormatFactory> Ptr;

    static Ptr New(const object& classObject)
    {
        return Ptr(new Sdf_PyFileFormatFactory(classObject));
    }

    virtual SdfFileFormatRefPtr New() const
    {
        return _factory();
    }

private:
    Sdf_PyFileFormatFactory(const object& classObject) :
        _factory(classObject)
    {
        // Do nothing
    }

private:
    // XXX -- TfPyCall::operator() should be const.
    mutable TfPyCall<SdfFileFormatRefPtr> _factory;
};

// Helper function for registering a file format from Python.
// Shamelessly stolen from Mf/wrapMapper.cpp and Mf/wrapExpression.cpp.
void
_RegisterFileFormat(object classObject)
{
    std::string typeName =
        extract<std::string>( classObject.attr("__name__") );

    TfType fileFormatType = TfType_DefinePythonTypeAndBases(classObject);

    if (fileFormatType.IsUnknown()) {
        // CODE_COVERAGE_OFF - The code above registers this TfType, and
        // currently never fails.
        TF_CODING_ERROR("Could not define Python type for %s.",
                        typeName.c_str());
        return;
        // CODE_COVERAGE_ON
    }

    // Set an type alias under MfPrim with the module-less name of the
    // Python class.
    fileFormatType.AddAlias( TfType::Find<SdfFileFormat>(), typeName );

    // Register the factory function with the type.
    fileFormatType.SetFactory(Sdf_PyFileFormatFactory::New(classObject));
}

}

void wrapFileFormat()
{
    typedef SdfFileFormat This;
    typedef SdfFileFormatPtr ThisPtr;

    scope s = 
        class_<This, ThisPtr, boost::noncopyable>("FileFormat", no_init)

        .def(TfPyRefAndWeakPtr())

        .add_property("formatId",
            make_function(&This::GetFormatId,
                return_value_policy<return_by_value>()))
        .add_property("target",
            make_function(&This::GetTarget,
                return_value_policy<return_by_value>()))
        .add_property("fileCookie",
            make_function(&This::GetFileCookie,
                return_value_policy<return_by_value>()))
        .add_property("primaryFileExtension",
            make_function(&This::GetPrimaryFileExtension,
                return_value_policy<return_by_value>()))

        .def("GetFileExtensions", &This::GetFileExtensions,
            return_value_policy<return_by_value>())

        .def("IsSupportedExtension", &This::IsSupportedExtension)

        .def("CanRead", &This::CanRead)

        .def("GetFileExtension", &This::GetFileExtension)
        .staticmethod("GetFileExtension")

        .def("FindById", &This::FindById)
        .staticmethod("FindById")

        .def("FindByExtension", &This::FindByExtension,
             ( arg("extension"),
               arg("target") = std::string() ))
        .staticmethod("FindByExtension")

        .def("RegisterFileFormat", &_RegisterFileFormat)
        .staticmethod("RegisterFileFormat")
        ;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "Tokens", SdfFileFormatTokens, SDF_FILE_FORMAT_TOKENS);
}

TF_REFPTR_CONST_VOLATILE_GET(SdfFileFormat)

PXR_NAMESPACE_CLOSE_SCOPE
