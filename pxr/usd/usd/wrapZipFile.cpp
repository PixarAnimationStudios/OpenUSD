//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"

#include "pxr/usd/usd/zipFile.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/noncopyable.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/return_value_policy.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

static boost::python::object
_Open(const std::string& filePath)
{
    UsdZipFile zipFile = UsdZipFile::Open(filePath);
    return zipFile ? boost::python::object(zipFile) : boost::python::object();
}

static boost::python::object
_GetFile(const UsdZipFile& zipFile, const std::string& filePath)
{
    auto iter = zipFile.Find(filePath);
    if (iter == zipFile.end()) {
        return boost::python::object();
    }
    return TfPyCopyBufferToByteArray(iter.GetFile(), iter.GetFileInfo().size);
}

static boost::python::object
_GetFileInfo(const UsdZipFile& zipFile, const std::string& filePath)
{
    auto iter = zipFile.Find(filePath);
    if (iter == zipFile.end()) {
        return boost::python::object();
    }
    return boost::python::object(iter.GetFileInfo());
}

static std::vector<std::string>
_GetFileNames(const UsdZipFile& zipFile)
{
    return std::vector<std::string>(zipFile.begin(), zipFile.end());
}

// XXX: UsdZipFileWriter is a move-only type, but if I return a 
// UsdZipFileWriter by value from this function, boost::python gives me a
// no to-python converter error.
static UsdZipFileWriter*
_CreateNew(const std::string& filePath)
{
    return new UsdZipFileWriter(UsdZipFileWriter::CreateNew(filePath));
}

static void
_Enter(const UsdZipFileWriter&)
{
    // Nothing to do
}

static void
_Exit(UsdZipFileWriter& w, const boost::python::object& exc_type, const boost::python::object&, const boost::python::object&)
{
    if (w) {
        if (TfPyIsNone(exc_type)) {
            w.Save();
        }
        else {
            w.Discard();
        }
    }
}

void
wrapUsdZipFile()
{
    {
        boost::python::scope s = boost::python::class_<UsdZipFile>
            ("ZipFile", boost::python::no_init)
            .def("Open", _Open, boost::python::arg("filePath"))
            .staticmethod("Open")
            
            .def("GetFileNames", &_GetFileNames,
                boost::python::return_value_policy<TfPySequenceToList>())

            .def("GetFile", &_GetFile, boost::python::arg("path"))
            .def("GetFileInfo", &_GetFileInfo, boost::python::arg("path"))

            .def("DumpContents", &UsdZipFile::DumpContents)
            ;

        boost::python::class_<UsdZipFile::FileInfo>
            ("FileInfo", boost::python::no_init)
            .def_readonly("dataOffset", &UsdZipFile::FileInfo::dataOffset)
            .def_readonly("size", &UsdZipFile::FileInfo::size)
            .def_readonly("uncompressedSize", 
                &UsdZipFile::FileInfo::uncompressedSize)
            .def_readonly("compressionMethod", 
                &UsdZipFile::FileInfo::compressionMethod)
            .def_readonly("encrypted",
                &UsdZipFile::FileInfo::encrypted)
            ;
    }

    boost::python::class_<UsdZipFileWriter, boost::noncopyable>
        ("ZipFileWriter", boost::python::no_init)
        .def("CreateNew", &_CreateNew, boost::python::arg("filePath"),
            boost::python::return_value_policy<boost::python::manage_new_object>())
        .staticmethod("CreateNew")

        .def("AddFile", &UsdZipFileWriter::AddFile, 
            (boost::python::arg("filePath"), 
             boost::python::arg("filePathInArchive") = std::string()))
        .def("Save", &UsdZipFileWriter::Save)
        .def("Discard", &UsdZipFileWriter::Discard)

        .def("__enter__", &_Enter, boost::python::return_self<>())
        .def("__exit__", 
            (void(*)(UsdZipFileWriter&, const boost::python::object&, const boost::python::object&, 
                     const boost::python::object&))&_Exit);
        ;
}
