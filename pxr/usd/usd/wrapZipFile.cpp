//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/usd/zipFile.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/noncopyable.hpp>
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/manage_new_object.hpp"
#include "pxr/external/boost/python/return_arg.hpp"
#include "pxr/external/boost/python/return_value_policy.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
    return TfPyCopyBufferToByteArray(iter.GetFile(), iter.GetFileInfo().size);
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
// UsdZipFileWriter by value from this function, pxr_boost::python gives me a
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
            .def_readonly("crc", 
                &UsdZipFile::FileInfo::crc)
            .def_readonly("compressionMethod", 
                &UsdZipFile::FileInfo::compressionMethod)
            .def_readonly("encrypted",
                &UsdZipFile::FileInfo::encrypted)
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
