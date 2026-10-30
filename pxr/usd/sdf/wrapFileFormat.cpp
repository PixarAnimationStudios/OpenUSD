//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file Sdf/wrapFileFormat.cpp

#include "pxr/pxr.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyResultConversions.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// File format factory wrapper.  Python will give us a file format type
// and we must provide a factory to return instances of it.
class Sdf_PyFileFormatFactory : public Sdf_FileFormatFactoryBase {
public:
    typedef std::unique_ptr<Sdf_PyFileFormatFactory> Ptr;

    static Ptr New(const nanobind::object& classObject)
    {
        return Ptr(new Sdf_PyFileFormatFactory(classObject));
    }

    virtual SdfFileFormatRefPtr New() const
    {
        return _factory();
    }

private:
    Sdf_PyFileFormatFactory(const nanobind::object& classObject) :
        _factory(classObject)
    {
        // Do nothing
    }

private:
    // XXX -- TfPyCall::operator() should be const.
    mutable TfPyCall<SdfFileFormatRefPtr> _factory;
};

} // anonymous namespace 

void wrapFileFormat(nanobind::module_ &m)
{
    namespace nb = nanobind;
    typedef SdfFileFormat This;
    typedef SdfFileFormatPtr ThisPtr;

    nb::class_<This> s(m, "FileFormat",
                       nb::is_weak_referenceable(), nb::dynamic_attr());
    s
        .def(TfNbRefAndWeakPtr())

        .def_prop_ro("formatId", &This::GetFormatId, nb::rv_policy::copy)
        .def_prop_ro("target", &This::GetTarget, nb::rv_policy::copy)
        .def_prop_ro("fileCookie", &This::GetFileCookie, nb::rv_policy::copy)
        .def_prop_ro("primaryFileExtension",
            &This::GetPrimaryFileExtension, nb::rv_policy::copy)

        .def("GetFileExtensions", &This::GetFileExtensions,
            nb::rv_policy::copy)

        .def("IsSupportedExtension", &This::IsSupportedExtension)

        .def("IsPackage", &This::IsPackage)

        .def("CanRead", &This::CanRead)
        .def("CanRead", [](const This& self, const ArResolvedPath& file) {
            return self.CanRead(file);
        })

        .def_static("GetFileExtension", &This::GetFileExtension)
        .def_static("GetFileExtension", [](const ArResolvedPath& s) {
            return This::GetFileExtension(s);
        })

        .def_static("FindAllFileFormatExtensions", []() {
            return TfNbSequenceToList(This::FindAllFileFormatExtensions());
        })

        .def_static("FindAllDerivedFileFormatExtensions",
             [](const TfType& baseType) {
                 return TfNbSequenceToList(
                     This::FindAllDerivedFileFormatExtensions(baseType));
             })

        .def_static("FindById", &This::FindById)

        .def("SupportsReading", &SdfFileFormat::SupportsReading)
        .def("SupportsWriting", &SdfFileFormat::SupportsWriting)
        .def("SupportsEditing", &SdfFileFormat::SupportsEditing)

        .def_static("FormatSupportsReading", 
                &SdfFileFormat::FormatSupportsReading,
                nb::arg("extension"),
                nb::arg("target") = std::string())

        .def_static("FormatSupportsWriting", 
                &SdfFileFormat::FormatSupportsWriting,
                nb::arg("extension"),
                nb::arg("target") = std::string())

        .def_static("FormatSupportsEditing", 
                &SdfFileFormat::FormatSupportsEditing,
                nb::arg("extension"),
                nb::arg("target") = std::string())

        .def_static("FindByExtension",
             (SdfFileFormatConstPtr(*)(const std::string&, const std::string&))
                &This::FindByExtension,
             nb::arg("extension"),
             nb::arg("target") = std::string())
        .def_static("FindByExtension",
             (SdfFileFormatConstPtr(*)
                (const std::string&, const SdfFileFormat::FileFormatArguments&))
                &This::FindByExtension,
             nb::arg("extension"),
             nb::arg("args"))

        ;

    TF_NB_WRAP_PUBLIC_TOKENS(
        s, "Tokens", SdfFileFormatTokens, SDF_FILE_FORMAT_TOKENS);

}
