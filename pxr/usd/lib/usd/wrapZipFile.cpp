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

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static object
_Open(const std::string& filePath)
{
    UsdZipFile zipFile = UsdZipFile::Open(filePath);
    return zipFile ? object(zipFile) : object();
}

static object
_GetFile(const UsdZipFile& zipFile, const std::string& filePath)
{
    auto iter = zipFile.Find(filePath);
    if (iter == zipFile.end()) {
        return object();
    }
    std::vector<char> data(
        iter.GetFile(), iter.GetFile() + iter.GetFileInfo().size);
    return TfPyCopySequenceToList(data);
}

static object
_GetFileInfo(const UsdZipFile& zipFile, const std::string& filePath)
{
    auto iter = zipFile.Find(filePath);
    if (iter == zipFile.end()) {
        return object();
    }
    return object(iter.GetFileInfo());
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
_Exit(UsdZipFileWriter& w, const object& exc_type, const object&, const object&)
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
        scope s = class_<UsdZipFile>
            ("ZipFile", no_init)
            .def("Open", _Open, arg("filePath"))
            .staticmethod("Open")
            
            .def("GetFileNames", &_GetFileNames,
                return_value_policy<TfPySequenceToList>())

            .def("GetFile", &_GetFile, arg("path"))
            .def("GetFileInfo", &_GetFileInfo, arg("path"))

            .def("DumpContents", &UsdZipFile::DumpContents)
            ;

        class_<UsdZipFile::FileInfo>
            ("FileInfo", no_init)
            .def_readonly("dataOffset", &UsdZipFile::FileInfo::dataOffset)
            .def_readonly("size", &UsdZipFile::FileInfo::size)
            .def_readonly("uncompressedSize", 
                &UsdZipFile::FileInfo::uncompressedSize)
            .def_readonly("compressionMethod", 
                &UsdZipFile::FileInfo::compressionMethod)
            ;
    }

    class_<UsdZipFileWriter, boost::noncopyable>
        ("ZipFileWriter", no_init)
        .def("CreateNew", &_CreateNew, arg("filePath"),
            return_value_policy<manage_new_object>())
        .staticmethod("CreateNew")

        .def("AddFile", &UsdZipFileWriter::AddFile, 
            (arg("filePath"), 
             arg("filePathInArchive") = std::string()))
        .def("Save", &UsdZipFileWriter::Save)
        .def("Discard", &UsdZipFileWriter::Discard)

        .def("__enter__", &_Enter, return_self<>())
        .def("__exit__", 
            (void(*)(UsdZipFileWriter&, const object&, const object&, 
                     const object&))&_Exit);
        ;
}
