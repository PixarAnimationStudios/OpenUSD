//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/zipFile.h"
#include "pxr/base/tf/pyResultConversions.h"

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
    SdfZipFile zipFile = SdfZipFile::Open(filePath);
    return zipFile ? object(zipFile) : object();
}

static object
_GetFile(const SdfZipFile& zipFile, const std::string& filePath)
{
    auto iter = zipFile.Find(filePath);
    if (iter == zipFile.end()) {
        return object();
    }
    return TfPyCopyBufferToByteArray(iter.GetFile(), iter.GetFileInfo().size);
}

static object
_GetFileInfo(const SdfZipFile& zipFile, const std::string& filePath)
{
    auto iter = zipFile.Find(filePath);
    if (iter == zipFile.end()) {
        return object();
    }
    return object(iter.GetFileInfo());
}

static std::vector<std::string>
_GetFileNames(const SdfZipFile& zipFile)
{
    return std::vector<std::string>(zipFile.begin(), zipFile.end());
}

// XXX: SdfZipFileWriter is a move-only type, but if I return a 
// SdfZipFileWriter by value from this function, pxr_boost::python gives me a
// no to-python converter error.
static SdfZipFileWriter*
_CreateNew(const std::string& filePath)
{
    return new SdfZipFileWriter(SdfZipFileWriter::CreateNew(filePath));
}

static void
_Enter(const SdfZipFileWriter&)
{
    // Nothing to do
}

static void
_Exit(SdfZipFileWriter& w, const object& exc_type, const object&, const object&)
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
wrapZipFile()
{
    {
        scope s = class_<SdfZipFile>
            ("ZipFile", no_init)
            .def("Open", _Open, arg("filePath"))
            .staticmethod("Open")
            
            .def("GetFileNames", &_GetFileNames,
                return_value_policy<TfPySequenceToList>())

            .def("GetFile", &_GetFile, arg("path"))
            .def("GetFileInfo", &_GetFileInfo, arg("path"))

            .def("DumpContents", &SdfZipFile::DumpContents)
            ;

        class_<SdfZipFile::FileInfo>
            ("FileInfo", no_init)
            .def_readonly("dataOffset", &SdfZipFile::FileInfo::dataOffset)
            .def_readonly("size", &SdfZipFile::FileInfo::size)
            .def_readonly("uncompressedSize", 
                &SdfZipFile::FileInfo::uncompressedSize)
            .def_readonly("crc", 
                &SdfZipFile::FileInfo::crc)
            .def_readonly("compressionMethod", 
                &SdfZipFile::FileInfo::compressionMethod)
            .def_readonly("encrypted",
                &SdfZipFile::FileInfo::encrypted)
            ;
    }

    class_<SdfZipFileWriter, noncopyable>
        ("ZipFileWriter", no_init)
        .def("CreateNew", &_CreateNew, arg("filePath"),
            return_value_policy<manage_new_object>())
        .staticmethod("CreateNew")

        .def("AddFile", &SdfZipFileWriter::AddFile, 
            (arg("filePath"), 
             arg("filePathInArchive") = std::string()))
        .def("Save", &SdfZipFileWriter::Save)
        .def("Discard", &SdfZipFileWriter::Discard)

        .def("__enter__", &_Enter, return_self<>())
        .def("__exit__", 
            (void(*)(SdfZipFileWriter&, const object&, const object&, 
                     const object&))&_Exit);
        ;
}
