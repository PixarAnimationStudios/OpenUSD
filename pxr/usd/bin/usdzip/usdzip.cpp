//
// Copyright 2022 Apple
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


#include <pxr/pxr.h>
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/ar/resolverContextBinder.h>
#include <pxr/base/arch/fileSystem.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/fileUtils.h>
#include "pxr/base/tf/stringUtils.h"
#include <pxr/usd/usd/zipFile.h>
#include <pxr/base/tf/debug.h>
#include <pxr/usd/usdUtils/dependencies.h>

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include <Python.h>
#endif // PXR_PYTHON_SUPPORT_ENABLED


PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_CLI;

struct Args {
    std::string usdzFile;
    std::vector<std::string> inputFiles = {};
    bool recurse = false;
    std::string asset;
    std::string arkitAsset;
    bool checkCompliance = false;
    std::string listTarget;
    std::string dumpTarget;
    bool verbose = false;
};

void Configure(CLI::App *app, Args &args) {
    app->add_option("usdzFile", args.usdzFile,
                    "Name of the .usdz file to create or to inspect the contents of.");
    app->add_option("inputFiles", args.inputFiles,
                    "Files to include in the .usdz files");
    app->add_flag("-r,--recurse", args.recurse,
                  "If specified, files in sub-directories are recursively added to the package");
    app->add_option("-a,--asset", args.asset,
                    "Resolvable asset path pointing to the root layer" \
                  "of the asset to be isolated and copied into the package.");
    app->add_option("--arkitAsset", args.arkitAsset,
                    "Similar to the --asset option, the --arkitAsset "\
                  "option packages all of the dependencies of the named scene file.\n"\
                  "Assets targeted at the initial usdz " \
                  "implementation in ARKit operate under greater " \
                  "constraints than usdz files for more general 'in " \
                  "house' uses, and this option attempts to ensure that " \
                  "these constraints are honored; this may involve more " \
                  "transformations to the data, which may cause loss of " \
                  "features such as VariantSets."
    );

    app->add_flag("-c,--checkCompliance", args.checkCompliance,
                  "(Currently does nothing)"
                  "Perform compliance checking of the input files."
                  "If the input asset or \"root\" layer fails any of the compliance checks,"
                  "the package is not created and the program fails."
    );
    app->add_option("-l,--list", args.listTarget,
                    "List contents of the specified usdz file. If "
                    "a file-path argument is provided, the list is output "
                    "to a file at the given path. If no argument is "
                    "provided or if '-' is specified as the argument, the "
                    "list is output to stdout"
    );
    app->add_option("-d,--dump", args.dumpTarget,
                    "Dump contents of the specified usdz file. If "
                    "a file-path argument is provided, the contents are "
                    "output to a file at the given path. If no argument is "
                    "provided or if '-' is specified as the argument, the "
                    "contents are output to stdout."
    );
    app->add_flag("-v,--verbose", args.verbose,
                  "Enable verbose mode, which causes messages "
                  "regarding files being added to the package to be"
                  " output to stdout");
}

/// CheckCompliance has to use the Python checker functions currently.
/// So we call out to Python if USD has been built with Python, otherwise we print an error and fail
/// Return true if successful or false if not
bool CheckCompliance(const std::string &rootLayer, bool arkit = false) {
#ifdef PXR_PYTHON_SUPPORT_ENABLED
    auto cmd = TfStringPrintf(
            "import sys\n"
            "from pxr import Ar, Sdf, Tf, Usd, UsdUtils\n"
            "def _Err(msg):\n"
            "    sys.stderr.write(msg + '\\n')\n"
            "\n\n"
            "def _CheckUsdzCompliance():\n"
            "    checker = UsdUtils.ComplianceChecker(arkit=%d, skipARKitRootLayerCheck=True)\n"
            "    checker.CheckCompliance('%s')\n"
            "    errors = checker.GetErrors()\n"
            "    failedChecks = checker.GetFailedChecks()\n"
            "    warnings = checker.GetWarnings()\n"
            "    for msg in errors + failedChecks:\n"
            "        _Err(msg)\n"
            "    if len(warnings) > 0:\n"
            "        _Err(\"*********************************************\\n\"\n"
            "             \"Possible correctness problems to investigate:\\n\"\n"
            "             \"*********************************************\\n\")\n"
            "        for msg in warnings:\n"
            "            _Err(msg)\n"
            "    return len(errors) == 0 and len(failedChecks) == 0\n", arkit, rootLayer.c_str());

    // Using regular Py_Initialize because TfPyInitialize was causing random segfaults
    Py_Initialize();
    PyObject * pymodule = PyImport_AddModule("__main__");
    if (!pymodule) {
        TF_CODING_ERROR("Failed to initialize __main__ module");
        return false;
    }
    auto localDict = PyDict_Copy(PyModule_GetDict(pymodule));
    if (!localDict) {
        TF_CODING_ERROR("Failed to initialize __main__ module dictionary");
        return false;
    }
    PyObject * code = Py_CompileString(cmd.c_str(), "__main__", Py_file_input);
    if (!code) {
        TF_CODING_ERROR("Failed to compile checker Python code");
        return false;
    }
    auto evaluated = PyEval_EvalCode(code, localDict, localDict);
    if (!evaluated) {
        TF_CODING_ERROR("Failed to evaluate checker Python code");
        return false;
    }
    auto func = PyDict_GetItemString(localDict, "_CheckUsdzCompliance");
    if (!func) {
        TF_CODING_ERROR("Failed to find _CheckUsdzCompliance function.");
        return false;
    }

    PyObject * args = PyTuple_New(0);
    auto called = PyObject_Call(func, args, NULL);
    if (!called) {
        TF_CODING_ERROR("Failed to run checker python code.");
        return false;
    }
    bool value = PyObject_IsTrue(called);
    Py_Finalize();
    if (!value) {
        std::cerr << "Failed USD Checker." << std::endl;
    }
    return value;
#else
    std::cerr << "Compliance checking requires a build with Python." << std::endl;
    return false;
#endif // PXR_PYTHON_SUPPORT_ENABLED
}

bool CreateUsdzPackage(const std::string &usdzFile, const std::vector<std::string> &filesToAdd, bool recurse,
                       bool checkCompliance, bool verbose) {
    std::vector<std::string> fileList;
    for (auto &path: filesToAdd) {
        if (TfIsDir(path)) {
            auto paths = TfListDir(path, recurse);
            for (auto &p: paths) {
                if (!TfIsDir(p) && ArchGetFileLength(p.c_str()) > 0) {
                    if (verbose) {
                        std::cout << ".. adding: " << p << std::endl;
                    }
                    fileList.emplace_back(p);
                }
            }
        } else {
            if (ArchGetFileLength(path.c_str()) > 0) {
                if (verbose) {
                    std::cout << ".. adding: " << path << std::endl;
                }
                fileList.emplace_back(path);
            }
        }
    }

    if (fileList.empty()) {
        std::cerr << "No files to package" << std::endl;
        return false;
    }

    if (checkCompliance) {
        if (!CheckCompliance(fileList[0])) {
            return false;
        }
    }

    auto writer = UsdZipFileWriter::CreateNew(usdzFile);
    for (auto &f: fileList) {
        if (writer.AddFile(f).empty()) {
            std::cerr << "Failed to add file " << f << " to package. Discarding package" << std::endl;
            writer.Discard();
            return false;
        }
    }

    return true;
}

void ListContents(const std::string &path, UsdZipFile &zipfile) {
    std::ostream *out = &std::cout;
    bool closeAfterUse = false;
    if (path != "-") {
        closeAfterUse = true;
        out = new std::ofstream(path);
    }

    std::vector<std::string> filenames(zipfile.begin(), zipfile.end());
    for (auto &fn: filenames) {
        *out << fn << std::endl;
    }

    if (closeAfterUse) {
        static_cast<std::ofstream *>(out)->close();
        delete out;
    }

}

std::string padded(const size_t data, const size_t padding, const char spacer = ' ') {
    auto str = std::to_string(data);
    if (padding > str.size())
        str.insert(0, padding - str.size(), spacer);

    return str;
}

void DumpContents(const std::string &path, UsdZipFile &zipfile) {
    std::ostream *out = &std::cout;
    bool closeAfterUse = false;
    if (path != "-") {
        closeAfterUse = true;
        out = new std::ofstream(path);
    }
    std::vector<std::string> filenames(zipfile.begin(), zipfile.end());

    *out << "    Offset\t      Comp\t    Uncomp\tName" << std::endl;
    *out << "    ------\t      ----\t    ------\t----" << std::endl;

    for (auto &fn: filenames) {
        // Copying the logic from Python, instead of trying to be fast
        auto info = zipfile.Find(fn).GetFileInfo();
        *out << padded(info.dataOffset, 10) << "\t"
             << padded(info.size, 10) << "\t"
             << padded(info.uncompressedSize, 10) << "\t"
             << fn << std::endl;

    }

    *out << "----------\n" << std::to_string(filenames.size()) << " files total" << std::endl;

    if (closeAfterUse) {
        static_cast<std::ofstream *>(out)->close();
        delete out;
    }
}

int USDZip(Args &args) {
    if (!args.asset.empty() and !args.arkitAsset.empty()) {
        std::cerr << "Specify either --asset or --arkitAsset, not both." << std::endl;
        return 1;
    }

    if (args.inputFiles.size() > 0 && (!args.asset.empty() || !args.arkitAsset.empty())) {
        std::cerr << "Specify either inputFiles or an asset (via --asset or "
                  << "--arKitAsset, not both" << std::endl;
        return 1;
    }

    // If usdzFile is not specified directly as an argument, check if it has been
    // specified as an argument to the --list or --dump options. In these cases,
    // output the list or the contents to stdout.
    std::string listTarget = args.listTarget;
    std::string dumpTarget = args.dumpTarget;
    std::string usdzFile = args.usdzFile;
    if (usdzFile.empty()) {
        if (!listTarget.empty() && listTarget != "-"
            && TfGetExtension(listTarget) != "usdz"
            && TfPathExists(listTarget)) {
            usdzFile = listTarget;
            listTarget = "-";
        } else if (!dumpTarget.empty() && dumpTarget != "-"
                   && TfGetExtension(dumpTarget) != "usdz"
                   && TfPathExists(dumpTarget)) {
            usdzFile = dumpTarget;
            dumpTarget = "-";
        } else {
            std::cerr << "No usdz file specified." << std::endl;
            return 1;
        }

    }

    // Check if we're in package creation mode and verbose mode is enabled
    // print some useful information
    if (!args.asset.empty() || !args.arkitAsset.empty() || args.inputFiles.size() > 0) {
        if (TfGetExtension(usdzFile) != "usdz") {
            usdzFile += ".usdz";
        }

        if (args.verbose) {
            if (TfPathExists(usdzFile)) {
                std::cout << "File at path " << usdzFile << " already exists. "
                          << "Overwriting file." << std::endl;
            }

            if (args.inputFiles.size() > 0) {
                std::cout << "Creating package " << usdzFile << "with files ";
                for (auto &i: args.inputFiles) {
                    std::cout << i << ", ";
                }
                std::cout << std::endl;
            }

            if (!args.asset.empty() || !args.arkitAsset.empty()) {
                TfDebug::SetDebugSymbolsByName("USDUTILS_CREATE_USDZ_PACKAGE", 1);
            }

            if (!args.recurse) {
                std::cout << "Not recursing into sub-directories." << std::endl;
            }
        }
    } else {
        if (args.checkCompliance) {
            std::cerr << "--checkCompliance should only be specified when "
                      << "creating a usdz package. Please use 'usdchecker' to check "
                      << "compliance of an existing .usdz file." << std::endl;
            return 1;
        }
    }

    bool success = true;
    if (args.inputFiles.size() > 0) {
        success = CreateUsdzPackage(usdzFile, args.inputFiles, args.recurse,
                                    args.checkCompliance, args.verbose) && success;
    } else if (!args.asset.empty()) {
        ArResolver &resolver = ArGetResolver();
        auto resolvedAsset = resolver.Resolve(args.asset);
        if (args.checkCompliance) {
            success = CheckCompliance(resolvedAsset, false) && success;
        }

        auto context = resolver.CreateDefaultContextForAsset(resolvedAsset);
        auto binder = ArResolverContextBinder(context);
        // Create the package only if the compliance check was passed.
        success = success && UsdUtilsCreateNewUsdzPackage(SdfAssetPath(args.asset), args.usdzFile);
    } else if (!args.arkitAsset.empty()) {
        ArResolver &resolver = ArGetResolver();
        auto resolvedAsset = resolver.Resolve(args.arkitAsset);
        if (args.checkCompliance) {
            success = CheckCompliance(resolvedAsset, true) && success;
        }

        auto context = resolver.CreateDefaultContextForAsset(resolvedAsset);
        auto binder = ArResolverContextBinder(context);
        // Create the package only if the compliance check was passed.

        success = success && UsdUtilsCreateNewARKitUsdzPackage(SdfAssetPath(args.arkitAsset), args.usdzFile);
    }

    if (!success) {
        std::cerr << "Failed to author USDZ file" << std::endl;
        return 1;
    }

    if (!listTarget.empty() || !dumpTarget.empty()) {
        if (TfPathExists(usdzFile)) {
            auto zipfile = UsdZipFile::Open(usdzFile);
            if (zipfile) {
                if (!dumpTarget.empty()) {
                    if (dumpTarget == usdzFile) {
                        std::cerr << "The file into which the contents will be dumped "
                                  << usdzFile << " must be different from the file itself." << std::endl;
                        return 1;
                    }
                    DumpContents(dumpTarget, zipfile);
                }
                if (!listTarget.empty()) {
                    if (listTarget == usdzFile) {
                        std::cerr << "The file into which the contents will be listed "
                                  << usdzFile << " must be different from the file itself." << std::endl;
                        return 1;
                    }
                    ListContents(listTarget, zipfile);
                }
            } else {
                std::cerr << "Failed to open usdz file at path " << usdzFile << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Can't find usdz file at path " << usdzFile << std::endl;
            return 1;
        }
    }

    return 0;
}

std::string GetVersionString() {
    return std::to_string(PXR_MAJOR_VERSION) + "." + std::to_string(PXR_MINOR_VERSION) + "." +
           std::to_string(PXR_PATCH_VERSION);
}

int main(int argc, char const *argv[]) {
    CLI::App app("Utility for creating a .usdz file containing USD assets and for "
                 "inspecting existing .usdz files.", "usdzip");
    Args args;
    Configure(&app, args);
    CLI11_PARSE(app, argc, argv);

    return USDZip(args);
}