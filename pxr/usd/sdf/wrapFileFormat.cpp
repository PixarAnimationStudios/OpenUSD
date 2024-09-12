//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file Sdf/wrapFileFormat.cpp

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/scope.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
