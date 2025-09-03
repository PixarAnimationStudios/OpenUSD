//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/usd/zipFile.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapUsdZipFile()
{
    // NOTE: These are deprecated in favor of Sdf.ZipFile,
    // Sdf.ZipFile.FileInfo, and Sdf.ZipFileWriter.
    scope().attr("ZipFile") =
        TfPyGetClassObject<SdfZipFile>();
    scope().attr("ZipFile").attr("FileInfo") =
        TfPyGetClassObject<SdfZipFile::FileInfo>();
    scope().attr("ZipFileWriter") =
        TfPyGetClassObject<SdfZipFileWriter>();
}
