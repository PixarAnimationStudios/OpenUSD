//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/zipFile.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static std::optional<SdfZipFile>
_Open(const std::string& filePath)
{
    SdfZipFile zipFile = SdfZipFile::Open(filePath);
    return zipFile ? std::optional<SdfZipFile>(zipFile) : std::nullopt;
}

static nb::object
_GetFile(const SdfZipFile& zipFile, const std::string& filePath)
{
    auto iter = zipFile.Find(filePath);
    if (iter == zipFile.end()) {
        return nb::none();
    }
    return nb::borrow<nb::object>(
        TfPyCopyBufferToByteArray(iter.GetFile(), iter.GetFileInfo().size).ptr());
}

static std::optional<SdfZipFile::FileInfo>
_GetFileInfo(const SdfZipFile& zipFile, const std::string& filePath)
{
    auto iter = zipFile.Find(filePath);
    if (iter == zipFile.end()) {
        return std::nullopt;
    }
    return iter.GetFileInfo();
}

static std::vector<std::string>
_GetFileNames(const SdfZipFile& zipFile)
{
    return std::vector<std::string>(zipFile.begin(), zipFile.end());
}

static void
_ExitWriter(SdfZipFileWriter& w, nb::args args)
{
    if (w) {
        if (args.size() == 0 || args[0].is_none()) {
            w.Save();
        }
        else {
            w.Discard();
        }
    }
}

void
wrapZipFile(nanobind::module_& m)
{
    {
        nb::class_<SdfZipFile> s(m, "ZipFile");
        s.def_static("Open", _Open, nb::arg("filePath"))
            
            .def("GetFileNames", &_GetFileNames)

            .def("GetFile", &_GetFile, nb::arg("path"))
            .def("GetFileInfo", &_GetFileInfo, nb::arg("path"))

            .def("DumpContents", &SdfZipFile::DumpContents)
            ;

        nb::class_<SdfZipFile::FileInfo>(s, "FileInfo")
            .def_ro("dataOffset", &SdfZipFile::FileInfo::dataOffset)
            .def_ro("size", &SdfZipFile::FileInfo::size)
            .def_ro("uncompressedSize", 
                &SdfZipFile::FileInfo::uncompressedSize)
            .def_ro("crc", 
                &SdfZipFile::FileInfo::crc)
            .def_ro("compressionMethod", 
                &SdfZipFile::FileInfo::compressionMethod)
            .def_ro("encrypted",
                &SdfZipFile::FileInfo::encrypted)
            ;

    }

    nb::class_<SdfZipFileWriter>(m, "ZipFileWriter")
        .def_static("CreateNew", &SdfZipFileWriter::CreateNew, nb::arg("filePath"))

        .def("AddFile", &SdfZipFileWriter::AddFile, 
            nb::arg("filePath"), 
            nb::arg("filePathInArchive") = std::string())
        .def("Save", &SdfZipFileWriter::Save)
        .def("Discard", &SdfZipFileWriter::Discard)

        .def("__enter__", [](nb::object self) { return self; })
        .def("__exit__", &_ExitWriter)
        ;

}
