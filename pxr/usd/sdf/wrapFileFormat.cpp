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
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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


} // anonymous namespace 

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

        .def("IsPackage", &This::IsPackage)

        .def("CanRead", &This::CanRead)

        .def("GetFileExtension", &This::GetFileExtension)
        .staticmethod("GetFileExtension")

        .def("FindAllFileFormatExtensions", &This::FindAllFileFormatExtensions,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("FindAllFileFormatExtensions")

        .def("FindAllDerivedFileFormatExtensions",
             &This::FindAllDerivedFileFormatExtensions,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("FindAllDerivedFileFormatExtensions")

        .def("FindById", &This::FindById)
        .staticmethod("FindById")

        .def("SupportsReading", &SdfFileFormat::SupportsReading)
        .def("SupportsWriting", &SdfFileFormat::SupportsWriting)
        .def("SupportsEditing", &SdfFileFormat::SupportsEditing)

        .def("FormatSupportsReading", 
                SdfFileFormat::FormatSupportsReading,
                ( arg("extension"),
                  arg("target") = std::string() ))
        .staticmethod("FormatSupportsReading")

        .def("FormatSupportsWriting", 
                SdfFileFormat::FormatSupportsWriting,
                ( arg("extension"),
                  arg("target") = std::string() ))
        .staticmethod("FormatSupportsWriting")

        .def("FormatSupportsEditing", 
                SdfFileFormat::FormatSupportsEditing,
                ( arg("extension"),
                  arg("target") = std::string() ))
        .staticmethod("FormatSupportsEditing")

        .def("FindByExtension",
             (SdfFileFormatConstPtr(*)(const std::string&, const std::string&))
                &This::FindByExtension,
             ( arg("extension"),
               arg("target") = std::string() ))
        .def("FindByExtension",
             (SdfFileFormatConstPtr(*)
                (const std::string&, const SdfFileFormat::FileFormatArguments&))
                &This::FindByExtension,
             ( arg("extension"),
               arg("args") ))
        .staticmethod("FindByExtension")

        ;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "Tokens", SdfFileFormatTokens, SDF_FILE_FORMAT_TOKENS);

}
