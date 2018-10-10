#
# Copyright 2017 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
from distutils.spawn import find_executable

import argparse
import contextlib
import datetime
import fnmatch
import glob
import multiprocessing
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import urllib2
import zipfile

# Helpers for printing output
verbosity = 1

def Print(msg):
    if verbosity > 0:
        print msg

def PrintWarning(warning):
    if verbosity > 0:
        print "WARNING:", warning

def PrintStatus(status):
    if verbosity >= 1:
        print "STATUS:", status

def PrintInfo(info):
    if verbosity >= 2:
        print "INFO:", info

def PrintCommandOutput(output):
    if verbosity >= 3:
        sys.stdout.write(output)

def PrintError(error):
    print "ERROR:", error

# Helpers for determining platform
def Windows():
    return platform.system() == "Windows"
def Linux():
    return platform.system() == "Linux"
def MacOS():
    return platform.system() == "Darwin"

def GetXcodeDeveloperDirectory():
    """Returns the active developer directory as reported by 'xcode-select -p'.
    Returns None if none is set."""
    if not MacOS():
        return None

    try:
        return subprocess.check_output("xcode-select -p").strip()
    except subprocess.CalledProcessError:
        pass
    return None

def GetVisualStudioCompilerAndVersion():
    """Returns a tuple containing the path to the Visual Studio compiler
    and a tuple for its version, e.g. (19, 00, 24210). If the compiler is
    not found, returns None."""
    if not Windows():
        return None

    msvcCompiler = find_executable('cl')
    if msvcCompiler:
        match = re.search(
            "Compiler Version (\d+).(\d+).(\d+)", 
            subprocess.check_output("cl", stderr=subprocess.STDOUT))
        if match:
            return (msvcCompiler, tuple(int(v) for v in match.groups()))
    return None


def IsVisualStudio2017OrGreater():
    MSVC_2017_COMPILER_VERSION = (19, 10, 00000)
    msvcCompilerAndVersion = GetVisualStudioCompilerAndVersion()
    if msvcCompilerAndVersion:
        _, version = msvcCompilerAndVersion
        return version >= MSVC_2017_COMPILER_VERSION
    return False


def GetCPUCount():
    try:
        return multiprocessing.cpu_count()
    except NotImplementedError:
        return 1

def Run(cmd, logCommandOutput = True):
    """Run the specified command in a subprocess."""
    PrintInfo('Running "{cmd}"'.format(cmd=cmd))

    with open("log.txt", "a") as logfile:
        logfile.write(datetime.datetime.now().strftime("%Y-%m-%d %H:%M"))
        logfile.write("\n")
        logfile.write(cmd)
        logfile.write("\n")

        # Let exceptions escape from subprocess calls -- higher level
        # code will handle them.
        if logCommandOutput:
            p = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE, 
                                 stderr=subprocess.STDOUT)
            while True:
                l = p.stdout.readline()
                if l != "":
                    logfile.write(l)
                    PrintCommandOutput(l)
                elif p.poll() is not None:
                    break
        else:
            p = subprocess.Popen(shlex.split(cmd))
            p.wait()

    if p.returncode != 0:
        # If verbosity >= 3, we'll have already been printing out command output
        # so no reason to print the log file again.
        if verbosity < 3:
            with open("log.txt", "r") as logfile:
                Print(logfile.read())
        raise RuntimeError("Failed to run '{cmd}'\nSee {log} for more details."
                           .format(cmd=cmd, log=os.path.abspath("log.txt")))

@contextlib.contextmanager
def CurrentWorkingDirectory(dir):
    """Context manager that sets the current working directory to the given
    directory and resets it to the original directory when closed."""
    curdir = os.getcwd()
    os.chdir(dir)
    try: yield
    finally: os.chdir(curdir)

def CopyFiles(context, src, dest):
    """Copy files like shutil.copy, but src may be a glob pattern."""
    filesToCopy = glob.glob(src)
    if not filesToCopy:
        raise RuntimeError("File(s) to copy {src} not found".format(src=src))

    instDestDir = os.path.join(context.instDir, dest)
    for f in filesToCopy:
        PrintCommandOutput("Copying {file} to {destDir}\n"
                           .format(file=f, destDir=instDestDir))
        shutil.copy(f, instDestDir)

def CopyDirectory(context, srcDir, destDir):
    """Copy directory like shutil.copytree."""
    instDestDir = os.path.join(context.instDir, destDir)
    if os.path.isdir(instDestDir):
        shutil.rmtree(instDestDir)    

    PrintCommandOutput("Copying {srcDir} to {destDir}\n"
                       .format(srcDir=srcDir, destDir=instDestDir))
    shutil.copytree(srcDir, instDestDir)

def RunCMake(context, force, extraArgs = None):
    """Invoke CMake to configure, build, and install a library whose 
    source code is located in the current working directory."""
    # Create a directory for out-of-source builds in the build directory
    # using the name of the current working directory.
    srcDir = os.getcwd()
    instDir = (context.usdInstDir if srcDir == context.usdSrcDir
               else context.instDir)
    buildDir = os.path.join(context.buildDir, os.path.split(srcDir)[1])
    if force and os.path.isdir(buildDir):
        shutil.rmtree(buildDir)

    if not os.path.isdir(buildDir):
        os.makedirs(buildDir)

    generator = context.cmakeGenerator

    # On Windows, we need to explicitly specify the generator to ensure we're
    # building a 64-bit project. (Surely there is a better way to do this?)
    if generator is None and Windows():
        if IsVisualStudio2017OrGreater():
            generator = "Visual Studio 15 2017 Win64"
        else:
            generator = "Visual Studio 14 2015 Win64"

    if generator is not None:
        generator = '-G "{gen}"'.format(gen=generator)
                
    # On MacOS, enable the use of @rpath for relocatable builds.
    osx_rpath = None
    if MacOS():
        osx_rpath = "-DCMAKE_MACOSX_RPATH=ON"

    with CurrentWorkingDirectory(buildDir):
        Run('cmake '
            '-DCMAKE_INSTALL_PREFIX="{instDir}" '
            '-DCMAKE_PREFIX_PATH="{depsInstDir}" '
            '{osx_rpath} '
            '{generator} '
            '{extraArgs} '
            '"{srcDir}"'
            .format(instDir=instDir,
                    depsInstDir=context.instDir,
                    srcDir=srcDir,
                    osx_rpath=(osx_rpath or ""),
                    generator=(generator or ""),
                    extraArgs=(" ".join(extraArgs) if extraArgs else "")))
        Run("cmake --build . --config Release --target install -- {multiproc}"
            .format(multiproc=("/M:{procs}" if Windows() else "-j{procs}")
                               .format(procs=context.numJobs)))

def PatchFile(filename, patches):
    """Applies patches to the specified file. patches is a list of tuples
    (old string, new string)."""
    oldLines = open(filename, 'r').readlines()
    newLines = oldLines
    for (oldLine, newLine) in patches:
        newLines = [s.replace(oldLine, newLine) for s in newLines]
    if newLines != oldLines:
        PrintInfo("Patching file {filename} (original in {oldFilename})..."
                  .format(filename=filename, oldFilename=filename + ".old"))
        shutil.copy(filename, filename + ".old")
        open(filename, 'w').writelines(newLines)

def DownloadFileWithCurl(url, outputFilename):
    # Don't log command output so that curl's progress
    # meter doesn't get written to the log file.
    Run("curl {progress} -L -o {filename} {url}".format(
        progress="-#" if verbosity >= 2 else "-s",
        filename=outputFilename, url=url), 
        logCommandOutput=False)

def DownloadFileWithPowershell(url, outputFilename):
    # It's important that we specify to use TLS v1.2 at least or some
    # of the downloads will fail.
    cmd = "powershell [Net.ServicePointManager]::SecurityProtocol = \
            [Net.SecurityProtocolType]::Tls12; \"(new-object \
            System.Net.WebClient).DownloadFile('{url}', '{filename}')\""\
            .format(filename=outputFilename, url=url)

    Run(cmd,logCommandOutput=False)

def DownloadFileWithUrllib(url, outputFilename):
    r = urllib2.urlopen(url)
    with open(outputFilename, "wb") as outfile:
        outfile.write(r.read())

def DownloadURL(url, context, force, dontExtract = None):
    """Download and extract the archive file at given URL to the
    source directory specified in the context. 

    dontExtract may be a sequence of path prefixes that will
    be excluded when extracting the archive.

    Returns the absolute path to the directory where files have 
    been extracted."""
    with CurrentWorkingDirectory(context.srcDir):
        # Extract filename from URL and see if file already exists. 
        filename = url.split("/")[-1]       
        if force and os.path.exists(filename):
            os.remove(filename)

        if os.path.exists(filename):
            PrintInfo("{0} already exists, skipping download"
                      .format(os.path.abspath(filename)))
        else:
            PrintInfo("Downloading {0} to {1}"
                      .format(url, os.path.abspath(filename)))

            # To work around occasional hiccups with downloading from websites
            # (SSL validation errors, etc.), retry a few times if we don't
            # succeed in downloading the file.
            maxRetries = 5
            lastError = None

            # Download to a temporary file and rename it to the expected
            # filename when complete. This ensures that incomplete downloads
            # will be retried if the script is run again.
            tmpFilename = filename + ".tmp"
            if os.path.exists(tmpFilename):
                os.remove(tmpFilename)

            for i in xrange(maxRetries):
                try:
                    context.downloader(url, tmpFilename)
                    break
                except Exception as e:
                    PrintCommandOutput("Retrying download due to error: {err}\n"
                                       .format(err=e))
                    lastError = e
            else:
                errorMsg = str(lastError)
                if "SSL: TLSV1_ALERT_PROTOCOL_VERSION" in errorMsg:
                    errorMsg += ("\n\n"
                                 "Your OS or version of Python may not support "
                                 "TLS v1.2+, which is required for downloading "
                                 "files from certain websites. This support "
                                 "was added in Python 2.7.9."
                                 "\n\n"
                                 "You can use curl to download dependencies "
                                 "by installing it in your PATH and re-running "
                                 "this script.")
                raise RuntimeError("Failed to download {url}: {err}"
                                   .format(url=url, err=errorMsg))

            shutil.move(tmpFilename, filename)

        # Open the archive and retrieve the name of the top-most directory.
        # This assumes the archive contains a single directory with all
        # of the contents beneath it.
        archive = None
        rootDir = None
        members = None
        try:
            if tarfile.is_tarfile(filename):
                archive = tarfile.open(filename)
                rootDir = archive.getnames()[0].split('/')[0]
                if dontExtract != None:
                    members = (m for m in archive.getmembers() 
                               if not any((fnmatch.fnmatch(m.name, p)
                                           for p in dontExtract)))
            elif zipfile.is_zipfile(filename):
                archive = zipfile.ZipFile(filename)
                rootDir = archive.namelist()[0].split('/')[0]
                if dontExtract != None:
                    members = (m for m in archive.getnames() 
                               if not any((fnmatch.fnmatch(m, p)
                                           for p in dontExtract)))
            else:
                raise RuntimeError("unrecognized archive file type")

            with archive:
                extractedPath = os.path.abspath(rootDir)
                if force and os.path.isdir(extractedPath):
                    shutil.rmtree(extractedPath)

                if os.path.isdir(extractedPath):
                    PrintInfo("Directory {0} already exists, skipping extract"
                              .format(extractedPath))
                else:
                    PrintInfo("Extracting archive to {0}".format(extractedPath))

                    # Extract to a temporary directory then move the contents
                    # to the expected location when complete. This ensures that
                    # incomplete extracts will be retried if the script is run
                    # again.
                    tmpExtractedPath = os.path.abspath("extract_dir")
                    if os.path.isdir(tmpExtractedPath):
                        shutil.rmtree(tmpExtractedPath)

                    archive.extractall(tmpExtractedPath, members=members)

                    shutil.move(os.path.join(tmpExtractedPath, rootDir),
                                extractedPath)
                    shutil.rmtree(tmpExtractedPath)

                return extractedPath
        except Exception as e:
            # If extraction failed for whatever reason, assume the
            # archive file was bad and move it aside so that re-running
            # the script will try downloading and extracting again.
            shutil.move(filename, filename + ".bad")
            raise RuntimeError("Failed to extract archive {filename}: {err}"
                               .format(filename=filename, err=e))

############################################################
# 3rd-Party Dependencies

AllDependencies = list()
AllDependenciesByName = dict()

class Dependency(object):
    def __init__(self, name, installer, *files):
        self.name = name
        self.installer = installer
        self.filesToCheck = files

        AllDependencies.append(self)
        AllDependenciesByName.setdefault(name.lower(), self)

    def Exists(self, context):
        return all([os.path.isfile(os.path.join(context.instDir, f))
                    for f in self.filesToCheck])

class PythonDependency(object):
    def __init__(self, name, getInstructions, moduleNames):
        self.name = name
        self.getInstructions = getInstructions
        self.moduleNames = moduleNames

    def Exists(self, context):
        # If one of the modules in our list exists we are good
        for moduleName in self.moduleNames:
            try:
                # Eat all output; we just care if the import succeeded or not.
                subprocess.check_output(shlex.split(
                    'python -c "import {module}"'.format(module=moduleName)),
                    stderr=subprocess.STDOUT)
                return True
            except subprocess.CalledProcessError:
                pass
        return False


def AnyPythonDependencies(deps):
    return any([type(d) is PythonDependency for d in deps])

############################################################
# zlib

ZLIB_URL = "https://github.com/madler/zlib/archive/v1.2.11.zip"

def InstallZlib(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(ZLIB_URL, context, force)):
        RunCMake(context, force, buildArgs)

ZLIB = Dependency("zlib", InstallZlib, "include/zlib.h")
        
############################################################
# boost

if Linux():
    BOOST_URL = "https://downloads.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.gz"
    BOOST_VERSION_FILE = "include/boost/version.hpp"
elif MacOS():
    BOOST_URL = "https://downloads.sourceforge.net/project/boost/boost/1.61.0/boost_1_61_0.tar.gz"
    BOOST_VERSION_FILE = "include/boost/version.hpp"
elif Windows():
    BOOST_URL = "https://downloads.sourceforge.net/project/boost/boost/1.61.0/boost_1_61_0.tar.gz"
    # The default installation of boost on Windows puts headers in a versioned 
    # subdirectory, which we have to account for here. In theory, specifying 
    # "layout=system" would make the Windows install match Linux/MacOS, but that 
    # causes problems for other dependencies that look for boost.
    BOOST_VERSION_FILE = "include/boost-1_61/boost/version.hpp"

    # On Visual Studio 2017 we need at least boost 1.65.1
    if IsVisualStudio2017OrGreater():
        BOOST_URL = "https://downloads.sourceforge.net/project/boost/boost/1.65.1/boost_1_65_1.tar.gz"
        BOOST_VERSION_FILE = "include/boost-1_65_1/boost/version.hpp"



def InstallBoost(context, force, buildArgs):
    # Documentation files in the boost archive can have exceptionally
    # long paths. This can lead to errors when extracting boost on Windows,
    # since paths are limited to 260 characters by default on that platform.
    # To avoid this, we skip extracting all documentation.
    #
    # For some examples, see: https://svn.boost.org/trac10/ticket/11677
    dontExtract = ["*/doc/*", "*/libs/*/doc/*"]

    with CurrentWorkingDirectory(DownloadURL(BOOST_URL, context, force, 
                                             dontExtract)):
        bootstrap = "bootstrap.bat" if Windows() else "./bootstrap.sh"
        Run('{bootstrap} --prefix="{instDir}"'
            .format(bootstrap=bootstrap, instDir=context.instDir))

        # b2 supports at most -j64 and will error if given a higher value.
        num_procs = min(64, context.numJobs)

        b2_settings = [
            '--prefix="{instDir}"'.format(instDir=context.instDir),
            '--build-dir="{buildDir}"'.format(buildDir=context.buildDir),
            '-j{procs}'.format(procs=num_procs),
            'address-model=64',
            'link=shared',
            'runtime-link=shared',
            'threading=multi', 
            'variant=release',
            '--with-atomic',
            '--with-date_time',
            '--with-filesystem',
            '--with-program_options',
            '--with-regex',
            '--with-system',
            '--with-thread'
        ]

        if context.buildPython:
            b2_settings.append("--with-python")

        if force:
            b2_settings.append("-a")

        if Windows():

            if IsVisualStudio2017OrGreater():
                b2_settings.append("toolset=msvc-14.1")
            else:
                b2_settings.append("toolset=msvc-14.0")

        if MacOS():
            # Must specify toolset=clang to ensure install_name for boost
            # libraries includes @rpath
            b2_settings.append("toolset=clang")

        # Add on any user-specified extra arguments.
        b2_settings += buildArgs

        b2 = "b2" if Windows() else "./b2"
        Run('{b2} {options} install'
            .format(b2=b2, options=" ".join(b2_settings)))

BOOST = Dependency("boost", InstallBoost, BOOST_VERSION_FILE)

############################################################
# Intel TBB

if Windows():
    TBB_URL = "https://github.com/01org/tbb/releases/download/2017_U5/tbb2017_20170226oss_win.zip"
elif MacOS():
    TBB_URL = "https://github.com/01org/tbb/archive/2017_U2.tar.gz"
else:
    TBB_URL = "https://github.com/01org/tbb/archive/4.4.6.tar.gz"

def InstallTBB(context, force, buildArgs):
    if Windows():
        InstallTBB_Windows(context, force, buildArgs)
    elif Linux() or MacOS():
        InstallTBB_LinuxOrMacOS(context, force, buildArgs)

def InstallTBB_Windows(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(TBB_URL, context, force)):
        # On Windows, we simply copy headers and pre-built DLLs to
        # the appropriate location.

        if buildArgs:
            PrintWarning("Ignoring build arguments {}, TBB is "
                         "not built from source on this platform."
                         .format(buildArgs))

        CopyFiles(context, "bin\\intel64\\vc14\\*.*", "bin")
        CopyFiles(context, "lib\\intel64\\vc14\\*.*", "lib")
        CopyDirectory(context, "include\\serial", "include\\serial")
        CopyDirectory(context, "include\\tbb", "include\\tbb")

def InstallTBB_LinuxOrMacOS(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(TBB_URL, context, force)):
        # TBB does not support out-of-source builds in a custom location.
        Run('make -j{procs} {buildArgs}'
            .format(procs=context.numJobs, 
                    buildArgs=" ".join(buildArgs)))

        CopyFiles(context, "build/*_release/libtbb*.*", "lib")
        CopyDirectory(context, "include/serial", "include/serial")
        CopyDirectory(context, "include/tbb", "include/tbb")

TBB = Dependency("TBB", InstallTBB, "include/tbb/tbb.h")

############################################################
# JPEG

if Windows():
    JPEG_URL = "https://github.com/libjpeg-turbo/libjpeg-turbo/archive/1.5.1.zip"
else:
    JPEG_URL = "http://www.ijg.org/files/jpegsrc.v9b.tar.gz"

def InstallJPEG(context, force, buildArgs):
    if Windows():
        InstallJPEG_Turbo(context, force, buildArgs)
    else:
        InstallJPEG_Lib(context, force, buildArgs)

def InstallJPEG_Turbo(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(JPEG_URL, context, force)):
        RunCMake(context, force, buildArgs)

def InstallJPEG_Lib(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(JPEG_URL, context, force)):
        Run('./configure --prefix="{instDir}" '
            '--disable-static --enable-shared '
            '{buildArgs}'
            .format(instDir=context.instDir,
                    buildArgs=" ".join(buildArgs)))
        Run('make -j{procs} install'
            .format(procs=context.numJobs))

JPEG = Dependency("JPEG", InstallJPEG, "include/jpeglib.h")
        
############################################################
# TIFF

TIFF_URL = "https://download.osgeo.org/libtiff/tiff-4.0.7.zip"

def InstallTIFF(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(TIFF_URL, context, force)):
        # libTIFF has a build issue on Windows where tools/tiffgt.c
        # unconditionally includes unistd.h, which does not exist.
        # To avoid this, we patch the CMakeLists.txt to skip building
        # the tools entirely. We do this on Linux and MacOS as well
        # to avoid requiring some GL and X dependencies.
        #
        # We also need to skip building tests, since they rely on 
        # the tools we've just elided.
        PatchFile("CMakeLists.txt", 
                   [("add_subdirectory(tools)", "# add_subdirectory(tools)"),
                    ("add_subdirectory(test)", "# add_subdirectory(test)")])
        RunCMake(context, force, buildArgs)

TIFF = Dependency("TIFF", InstallTIFF, "include/tiff.h")

############################################################
# PNG

PNG_URL = "https://downloads.sourceforge.net/project/libpng/libpng16/older-releases/1.6.29/libpng-1.6.29.tar.gz"

def InstallPNG(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(PNG_URL, context, force)):
        RunCMake(context, force, buildArgs)

PNG = Dependency("PNG", InstallPNG, "include/png.h")

############################################################
# IlmBase/OpenEXR

OPENEXR_URL = "https://github.com/openexr/openexr/archive/v2.2.0.zip"

def InstallOpenEXR(context, force, buildArgs):
    srcDir = DownloadURL(OPENEXR_URL, context, force)

    ilmbaseSrcDir = os.path.join(srcDir, "IlmBase")
    with CurrentWorkingDirectory(ilmbaseSrcDir):
        RunCMake(context, force, buildArgs)

    openexrSrcDir = os.path.join(srcDir, "OpenEXR")
    with CurrentWorkingDirectory(openexrSrcDir):
        RunCMake(context, force,
                 ['-DILMBASE_PACKAGE_PREFIX="{instDir}"'
                  .format(instDir=context.instDir)] + buildArgs)

OPENEXR = Dependency("OpenEXR", InstallOpenEXR, "include/OpenEXR/ImfVersion.h")

############################################################
# GLEW

if Windows():
    GLEW_URL = "https://downloads.sourceforge.net/project/glew/glew/2.0.0/glew-2.0.0-win32.zip"
else:
    # Important to get source package from this URL and NOT github. This package
    # contains pre-generated code that the github repo does not.
    GLEW_URL = "https://downloads.sourceforge.net/project/glew/glew/2.0.0/glew-2.0.0.tgz"

def InstallGLEW(context, force, buildArgs):
    if Windows():
        InstallGLEW_Windows(context, force)
    elif Linux() or MacOS():
        InstallGLEW_LinuxOrMacOS(context, force, buildArgs)

def InstallGLEW_Windows(context, force):
    with CurrentWorkingDirectory(DownloadURL(GLEW_URL, context, force)):
        # On Windows, we install headers and pre-built binaries per
        # https://glew.sourceforge.net/install.html
        # Note that we are installing just the shared library. This is required
        # by the USD build; if the static library is present, that one will be
        # used and that causes errors with USD and OpenSubdiv.
        CopyFiles(context, "bin\\Release\\x64\\glew32.dll", "bin")
        CopyFiles(context, "lib\\Release\\x64\\glew32.lib", "lib")
        CopyDirectory(context, "include\\GL", "include\\GL")

def InstallGLEW_LinuxOrMacOS(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(GLEW_URL, context, force)):
        Run('make GLEW_DEST="{instDir}" -j{procs} {buildArgs} install'
            .format(instDir=context.instDir,
                    procs=context.numJobs,
                    buildArgs=" ".join(buildArgs)))

GLEW = Dependency("GLEW", InstallGLEW, "include/GL/glew.h")

############################################################
# Ptex

PTEX_URL = "https://github.com/wdas/ptex/archive/v2.1.28.zip"

def InstallPtex(context, force, buildArgs):
    if Windows():
        InstallPtex_Windows(context, force, buildArgs)
    else:
        InstallPtex_LinuxOrMacOS(context, force, buildArgs)

def InstallPtex_Windows(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(PTEX_URL, context, force)):
        # Ptex has a bug where the import library for the dynamic library and
        # the static library both get the same name, Ptex.lib, and as a
        # result one clobbers the other. We hack the appropriate CMake
        # file to prevent that. Since we don't need the static library we'll
        # rename that.
        #
        # In addition src\tests\CMakeLists.txt adds -DPTEX_STATIC to the 
        # compiler but links tests against the dynamic library, causing the 
        # links to fail. We patch the file to not add the -DPTEX_STATIC
        PatchFile('src\\ptex\\CMakeLists.txt', 
                  [("set_target_properties(Ptex_static PROPERTIES OUTPUT_NAME Ptex)",
                    "set_target_properties(Ptex_static PROPERTIES OUTPUT_NAME Ptexs)")])
        PatchFile('src\\tests\\CMakeLists.txt',
                  [("add_definitions(-DPTEX_STATIC)", 
                    "# add_definitions(-DPTEX_STATIC)")])

        RunCMake(context, force, buildArgs)

def InstallPtex_LinuxOrMacOS(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(PTEX_URL, context, force)):
        RunCMake(context, force, buildArgs)

PTEX = Dependency("Ptex", InstallPtex, "include/PtexVersion.h")

############################################################
# OpenImageIO

OIIO_URL = "https://github.com/OpenImageIO/oiio/archive/Release-1.7.14.zip"

def InstallOpenImageIO(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(OIIO_URL, context, force)):
        extraArgs = ['-DOIIO_BUILD_TOOLS=OFF',
                     '-DOIIO_BUILD_TESTS=OFF',
                     '-DUSE_PYTHON=OFF',
                     '-DSTOP_ON_WARNING=OFF']

        # OIIO's FindOpenEXR module circumvents CMake's normal library 
        # search order, which causes versions of OpenEXR installed in
        # /usr/local or other hard-coded locations in the module to
        # take precedence over the version we've built, which would 
        # normally be picked up when we specify CMAKE_PREFIX_PATH. 
        # This may lead to undefined symbol errors at build or runtime. 
        # So, we explicitly specify the OpenEXR we want to use here.
        extraArgs.append('-DOPENEXR_HOME="{instDir}"'
                         .format(instDir=context.instDir))

        # If Ptex support is disabled in USD, disable support in OpenImageIO
        # as well. This ensures OIIO doesn't accidentally pick up a Ptex
        # library outside of our build.
        if not context.enablePtex:
            extraArgs.append('-DUSE_PTEX=OFF')

        # Add on any user-specified extra arguments.
        extraArgs += buildArgs

        RunCMake(context, force, extraArgs)

OPENIMAGEIO = Dependency("OpenImageIO", InstallOpenImageIO,
                         "include/OpenImageIO/oiioversion.h")

############################################################
# OpenSubdiv

OPENSUBDIV_URL = "https://github.com/PixarAnimationStudios/OpenSubdiv/archive/v3_1_1.zip"

def InstallOpenSubdiv(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(OPENSUBDIV_URL, context, force)):
        extraArgs = [
            '-DNO_EXAMPLES=ON',
            '-DNO_TUTORIALS=ON',
            '-DNO_REGRESSION=ON',
            '-DNO_DOC=ON',
            '-DNO_OMP=ON',
            '-DNO_CUDA=ON',
            '-DNO_OPENCL=ON',
            '-DNO_DX=ON',
            '-DNO_TESTS=ON',
        ]

        # OpenSubdiv's FindGLEW module won't look in CMAKE_PREFIX_PATH, so
        # we need to explicitly specify GLEW_LOCATION here.
        extraArgs.append('-DGLEW_LOCATION="{instDir}"'
                         .format(instDir=context.instDir))

        # If Ptex support is disabled in USD, disable support in OpenSubdiv
        # as well. This ensures OSD doesn't accidentally pick up a Ptex
        # library outside of our build.
        if not context.enablePtex:
            extraArgs.append('-DNO_PTEX=ON')

        # NOTE: For now, we disable TBB in our OpenSubdiv build.
        # This avoids an issue where OpenSubdiv will link against
        # all TBB libraries it finds, including libtbbmalloc and
        # libtbbmalloc_proxy. On Linux and MacOS, this has the
        # unwanted effect of replacing the system allocator with
        # tbbmalloc, which can cause problems with the Maya plugin.
        extraArgs.append('-DNO_TBB=ON')

        # Add on any user-specified extra arguments.
        extraArgs += buildArgs

        RunCMake(context, force, extraArgs)

OPENSUBDIV = Dependency("OpenSubdiv", InstallOpenSubdiv, 
                        "include/opensubdiv/version.h")

############################################################
# PyOpenGL

def GetPyOpenGLInstructions():
    return ('PyOpenGL is not installed. If you have pip '
            'installed, run "pip install PyOpenGL" to '
            'install it, then re-run this script.\n'
            'If PyOpenGL is already installed, you may need to '
            'update your PYTHONPATH to indicate where it is '
            'located.')

PYOPENGL = PythonDependency("PyOpenGL", GetPyOpenGLInstructions, 
                            moduleNames=["OpenGL"])

############################################################
# PySide

def GetPySideInstructions():
    # For licensing reasons, this script cannot install PySide itself.
    if MacOS():
        # There are issues with the PySide package available via pip, so
        # we direct users to installing PySide2 instead.
        return ('PySide is not installed. If you have pip '
                'installed, follow the instructions at '
                'https://wiki.qt.io/Qt_for_Python/GettingStarted '
                'to install PySide2 from published wheels, '
                'then re-run this script.\n'
                'If PySide is already installed, you may need to '
                'update your PYTHONPATH to indicate where it is '
                'located.')
    else:                       
        return ('PySide is not installed. If you have pip '
                'installed, run "pip install PySide" '
                'to install it, then re-run this script.\n'
                'If PySide is already installed, you may need to '
                'update your PYTHONPATH to indicate where it is '
                'located.')

PYSIDE = PythonDependency("PySide", GetPySideInstructions,
                          moduleNames=["PySide", "PySide2"])

############################################################
# HDF5

HDF5_URL = "https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.0-patch1/src/hdf5-1.10.0-patch1.zip"

def InstallHDF5(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(HDF5_URL, context, force)):
        RunCMake(context, force,
                 ['-DBUILD_TESTING=OFF',
                  '-DHDF5_BUILD_TOOLS=OFF',
                  '-DHDF5_BUILD_EXAMPLES=OFF'] + buildArgs)
                 
HDF5 = Dependency("HDF5", InstallHDF5, "include/hdf5.h")

############################################################
# Alembic

ALEMBIC_URL = "https://github.com/alembic/alembic/archive/1.7.1.zip"

def InstallAlembic(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(ALEMBIC_URL, context, force)):
        cmakeOptions = ['-DUSE_BINARIES=OFF', '-DUSE_TESTS=OFF']
        if context.enableHDF5:
            # HDF5 requires the H5_BUILT_AS_DYNAMIC_LIB macro be defined if
            # it was built with CMake as a dynamic library.
            cmakeOptions += [
                '-DUSE_HDF5=ON',
                '-DHDF5_ROOT="{instDir}"'.format(instDir=context.instDir),
                '-DCMAKE_CXX_FLAGS="-D H5_BUILT_AS_DYNAMIC_LIB"']
                
            if Windows():
                # Alembic doesn't link against HDF5 libraries on Windows 
                # whether or not USE_HDF5=ON or not.  There is a line to link 
                # against HDF5 on DARWIN so we hijack it to also link on WIN32.
                PatchFile("lib\\Alembic\\CMakeLists.txt", 
                          [("ALEMBIC_SHARED_LIBS AND DARWIN",
                            "ALEMBIC_SHARED_LIBS AND DARWIN OR ALEMBIC_SHARED_LIBS AND WIN32")])
        else:
           cmakeOptions += ['-DUSE_HDF5=OFF']
                 
        cmakeOptions += buildArgs

        RunCMake(context, force, cmakeOptions)

ALEMBIC = Dependency("Alembic", InstallAlembic, "include/Alembic/Abc/Base.h")

############################################################
# MaterialX

MATERIALX_URL = "https://github.com/materialx/MaterialX/archive/v1.36.0.zip"

def InstallMaterialX(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(MATERIALX_URL, context, force)):
        RunCMake(context, force, buildArgs)

MATERIALX = Dependency("MaterialX", InstallMaterialX, "include/MaterialXCore/Library.h")

############################################################
# USD

def InstallUSD(context, force, buildArgs):
    with CurrentWorkingDirectory(context.usdSrcDir):
        extraArgs = []

        if context.buildPython:
            extraArgs.append('-DPXR_ENABLE_PYTHON_SUPPORT=ON')
        else:
            extraArgs.append('-DPXR_ENABLE_PYTHON_SUPPORT=OFF')

        if context.buildShared:
            extraArgs.append('-DBUILD_SHARED_LIBS=ON')
        elif context.buildMonolithic:
            extraArgs.append('-DPXR_BUILD_MONOLITHIC=ON')
        
        if context.buildDocs:
            extraArgs.append('-DPXR_BUILD_DOCUMENTATION=ON')
        else:
            extraArgs.append('-DPXR_BUILD_DOCUMENTATION=OFF')
    
        if context.buildTests:
            extraArgs.append('-DPXR_BUILD_TESTS=ON')
        else:
            extraArgs.append('-DPXR_BUILD_TESTS=OFF')
            
        if context.buildImaging:
            extraArgs.append('-DPXR_BUILD_IMAGING=ON')
            if context.enablePtex:
                extraArgs.append('-DPXR_ENABLE_PTEX_SUPPORT=ON')
            else:
                extraArgs.append('-DPXR_ENABLE_PTEX_SUPPORT=OFF')

            if context.buildEmbree:
                if context.embreeLocation:
                    extraArgs.append('-DEMBREE_LOCATION="{location}"'
                                     .format(location=context.embreeLocation))
                extraArgs.append('-DPXR_BUILD_EMBREE_PLUGIN=ON')
            else:
                extraArgs.append('-DPXR_BUILD_EMBREE_PLUGIN=OFF')
            
            if context.buildOIIO:
                extraArgs.append('-DPXR_BUILD_OPENIMAGEIO_PLUGIN=ON')
            else:
                extraArgs.append('-DPXR_BUILD_OPENIMAGEIO_PLUGIN=OFF')
                
        else:
            extraArgs.append('-DPXR_BUILD_IMAGING=OFF')

        if context.buildUsdImaging:
            extraArgs.append('-DPXR_BUILD_USD_IMAGING=ON')
        else:
            extraArgs.append('-DPXR_BUILD_USD_IMAGING=OFF')

        if context.buildUsdview:
            extraArgs.append('-DPXR_BUILD_USDVIEW=ON')
        else:
            extraArgs.append('-DPXR_BUILD_USDVIEW=OFF')

        if context.buildAlembic:
            extraArgs.append('-DPXR_BUILD_ALEMBIC_PLUGIN=ON')
            if context.enableHDF5:
                extraArgs.append('-DPXR_ENABLE_HDF5_SUPPORT=ON')

                # CMAKE_PREFIX_PATH isn't sufficient for the FindHDF5 module 
                # to find the HDF5 we've built, so provide an extra hint.
                extraArgs.append('-DHDF5_ROOT="{instDir}"'
                                 .format(instDir=context.instDir))
            else:
                extraArgs.append('-DPXR_ENABLE_HDF5_SUPPORT=OFF')
        else:
            extraArgs.append('-DPXR_BUILD_ALEMBIC_PLUGIN=OFF')

        if context.buildMaterialX:
            extraArgs.append('-DPXR_BUILD_MATERIALX_PLUGIN=ON')
        else:
            extraArgs.append('-DPXR_BUILD_MATERIALX_PLUGIN=OFF')

        if context.buildMaya:
            if context.mayaLocation:
                extraArgs.append('-DMAYA_LOCATION="{mayaLocation}"'
                                 .format(mayaLocation=context.mayaLocation))
            extraArgs.append('-DPXR_BUILD_MAYA_PLUGIN=ON')
        else:
            extraArgs.append('-DPXR_BUILD_MAYA_PLUGIN=OFF')

        if context.buildKatana:
            if context.katanaApiLocation:
                extraArgs.append('-DKATANA_API_LOCATION="{apiLocation}"'
                                 .format(apiLocation=context.katanaApiLocation))
            extraArgs.append('-DPXR_BUILD_KATANA_PLUGIN=ON')
        else:
            extraArgs.append('-DPXR_BUILD_KATANA_PLUGIN=OFF')

        if context.buildHoudini:
            if context.houdiniLocation:
                extraArgs.append('-DHOUDINI_ROOT="{houdiniLocation}"'
                                 .format(houdiniLocation=context.houdiniLocation))
            extraArgs.append('-DPXR_BUILD_HOUDINI_PLUGIN=ON')
        else:
            extraArgs.append('-DPXR_BUILD_HOUDINI_PLUGIN=OFF')

        if Windows():
            # Increase the precompiled header buffer limit.
            extraArgs.append('-DCMAKE_CXX_FLAGS="/Zm150"')

        extraArgs += buildArgs

        RunCMake(context, force, extraArgs)

USD = Dependency("USD", InstallUSD, "include/pxr/pxr.h")

############################################################
# Install script

programDescription = """\
Installation Script for USD

Builds and installs USD and 3rd-party dependencies to specified location.

- Libraries:
The following is a list of libraries that this script will download and build
as needed. These names can be used to identify libraries for various script
options, like --force or --build-args.

{libraryList}

- Downloading Libraries:
If curl or powershell (on Windows) are installed and located in PATH, they
will be used to download dependencies. Otherwise, a built-in downloader will 
be used.

- Specifying Custom Build Arguments:
Users may specify custom build arguments for libraries using the --build-args
option. This values for this option must take the form <library name>,<option>. 
For example:

%(prog)s --build-args boost,cxxflags=... USD,-DPXR_STRICT_BUILD_MODE=ON ...
%(prog)s --build-args USD,"-DPXR_STRICT_BUILD_MODE=ON -DPXR_HEADLESS_TEST_MODE=ON" ...

These arguments will be passed directly to the build system for the specified 
library. Multiple quotes may be needed to ensure arguments are passed on 
exactly as desired. Users must ensure these arguments are suitable for the
specified library and do not conflict with other options, otherwise build 
errors may occur.
""".format(
    libraryList=" ".join(sorted([d.name for d in AllDependencies])))

parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description=programDescription)

parser.add_argument("install_dir", type=str, 
                    help="Directory where USD will be installed")
parser.add_argument("-n", "--dry_run", dest="dry_run", action="store_true",
                    help="Only summarize what would happen")
                    
group = parser.add_mutually_exclusive_group()
group.add_argument("-v", "--verbose", action="count", default=1,
                   dest="verbosity",
                   help="Increase verbosity level (1-3)")
group.add_argument("-q", "--quiet", action="store_const", const=0,
                   dest="verbosity",
                   help="Suppress all output except for error messages")

group = parser.add_argument_group(title="Build Options")
group.add_argument("-j", "--jobs", type=int, default=GetCPUCount(),
                   help=("Number of build jobs to run in parallel. "
                         "(default: # of processors [{0}])"
                         .format(GetCPUCount())))
group.add_argument("--build", type=str,
                   help=("Build directory for USD and 3rd-party dependencies " 
                         "(default: <install_dir>/build)"))
group.add_argument("--build-args", type=str, nargs="*", default=[],
                   help=("Custom arguments to pass to build system when "
                         "building libraries (see docs above)"))
group.add_argument("--force", type=str, action="append", dest="force_build",
                   default=[],
                   help=("Force download and build of specified library "
                         "(see docs above)"))
group.add_argument("--force-all", action="store_true",
                   help="Force download and build of all libraries")
group.add_argument("--generator", type=str,
                   help=("CMake generator to use when building libraries with "
                         "cmake"))

group = parser.add_argument_group(title="3rd Party Dependency Build Options")
group.add_argument("--src", type=str,
                   help=("Directory where dependencies will be downloaded "
                         "(default: <install_dir>/src)"))
group.add_argument("--inst", type=str,
                   help=("Directory where dependencies will be installed "
                         "(default: <install_dir>)"))

group = parser.add_argument_group(title="USD Options")

(SHARED_LIBS, MONOLITHIC_LIB) = (0, 1)
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--build-shared", dest="build_type",
                      action="store_const", const=SHARED_LIBS, 
                      default=SHARED_LIBS,
                      help="Build individual shared libraries (default)")
subgroup.add_argument("--build-monolithic", dest="build_type",
                      action="store_const", const=MONOLITHIC_LIB,
                      help="Build a single monolithic shared library")

subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--tests", dest="build_tests", action="store_true",
                      default=False, help="Build unit tests")
subgroup.add_argument("--no-tests", dest="build_tests", action="store_false",
                      help="Do not build unit tests (default)")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--docs", dest="build_docs", action="store_true",
                      default=False, help="Build documentation")
subgroup.add_argument("--no-docs", dest="build_docs", action="store_false",
                      help="Do not build documentation (default)")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--python", dest="build_python", action="store_true",
                      default=True, help="Build python based components "
                                         "(default)")
subgroup.add_argument("--no-python", dest="build_python", action="store_false",
                      help="Do not build python based components")

(NO_IMAGING, IMAGING, USD_IMAGING) = (0, 1, 2)

group = parser.add_argument_group(title="Imaging and USD Imaging Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--imaging", dest="build_imaging", 
                      action="store_const", const=IMAGING, default=USD_IMAGING,
                      help="Build imaging component")
subgroup.add_argument("--usd-imaging", dest="build_imaging", 
                      action="store_const", const=USD_IMAGING,
                      help="Build imaging and USD imaging components (default)")
subgroup.add_argument("--no-imaging", dest="build_imaging", 
                      action="store_const", const=NO_IMAGING,
                      help="Do not build imaging or USD imaging components")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--ptex", dest="enable_ptex", action="store_true", 
                      default=False, 
                      help="Enable Ptex support in imaging")
subgroup.add_argument("--no-ptex", dest="enable_ptex", 
                      action="store_false",
                      help="Disable Ptex support in imaging (default)")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--usdview", dest="build_usdview",
                      action="store_true", default=True,
                      help="Build usdview (default)")
subgroup.add_argument("--no-usdview", dest="build_usdview",
                      action="store_false", 
                      help="Do not build usdview")

group = parser.add_argument_group(title="Imaging Plugin Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--embree", dest="build_embree", action="store_true",
                      default=False,
                      help="Build Embree sample imaging plugin")
subgroup.add_argument("--no-embree", dest="build_embree", action="store_false",
                      help="Do not build Embree sample imaging plugin (default)")
group.add_argument("--embree-location", type=str,
                   help="Directory where Embree is installed.")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--openimageio", dest="build_oiio", action="store_true", 
                      default=False,
                      help="Build OpenImageIO plugin for USD")
subgroup.add_argument("--no-openimageio", dest="build_oiio", action="store_false",
                      help="Do not build OpenImageIO plugin for USD (default)")

group = parser.add_argument_group(title="Alembic Plugin Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--alembic", dest="build_alembic", action="store_true", 
                      default=False,
                      help="Build Alembic plugin for USD")
subgroup.add_argument("--no-alembic", dest="build_alembic", action="store_false",
                      help="Do not build Alembic plugin for USD (default)")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--hdf5", dest="enable_hdf5", action="store_true", 
                      default=False,
                      help="Enable HDF5 support in the Alembic plugin")
subgroup.add_argument("--no-hdf5", dest="enable_hdf5", action="store_false",
                      help="Disable HDF5 support in the Alembic plugin (default)")

group = parser.add_argument_group(title="MaterialX Plugin Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--materialx", dest="build_materialx", action="store_true", 
                      default=False,
                      help="Build MaterialX plugin for USD")
subgroup.add_argument("--no-materialx", dest="build_materialx", action="store_false",
                      help="Do not build MaterialX plugin for USD (default)")

group = parser.add_argument_group(title="Maya Plugin Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--maya", dest="build_maya", action="store_true", 
                      default=False,
                      help="Build Maya plugin for USD")
subgroup.add_argument("--no-maya", dest="build_maya", action="store_false",
                      help="Do not build Maya plugin for USD (default)")
group.add_argument("--maya-location", type=str,
                   help="Directory where Maya is installed.")

group = parser.add_argument_group(title="Katana Plugin Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--katana", dest="build_katana", action="store_true", 
                      default=False,
                      help="Build Katana plugin for USD")
subgroup.add_argument("--no-katana", dest="build_katana", action="store_false",
                      help="Do not build Katana plugin for USD (default)")
group.add_argument("--katana-api-location", type=str,
                   help="Directory where the Katana SDK is installed.")

group = parser.add_argument_group(title="Houdini Plugin Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--houdini", dest="build_houdini", action="store_true", 
                      default=False,
                      help="Build Houdini plugin for USD")
subgroup.add_argument("--no-houdini", dest="build_houdini", action="store_false",
                      help="Do not build Houdini plugin for USD (default)")
group.add_argument("--houdini-location", type=str,
                   help="Directory where Houdini is installed.")

args = parser.parse_args()

class InstallContext:
    def __init__(self, args):
        # Assume the USD source directory is in the parent directory
        self.usdSrcDir = os.path.normpath(
            os.path.join(os.path.abspath(os.path.dirname(__file__)), ".."))

        # Directory where USD will be installed
        self.usdInstDir = os.path.abspath(args.install_dir)

        # Directory where dependencies will be installed
        self.instDir = (os.path.abspath(args.inst) if args.inst 
                        else self.usdInstDir)

        # Directory where dependencies will be downloaded and extracted
        self.srcDir = (os.path.abspath(args.src) if args.src
                       else os.path.join(self.usdInstDir, "src"))
        
        # Directory where USD and dependencies will be built
        self.buildDir = (os.path.abspath(args.build) if args.build
                         else os.path.join(self.usdInstDir, "build"))

        # Determine which downloader to use.  The reason we don't simply
        # use urllib2 all the time is that some older versions of Python
        # don't support TLS v1.2, which is required for downloading some
        # dependencies.
        if find_executable("curl"):
            self.downloader = DownloadFileWithCurl
            self.downloaderName = "curl"
        elif Windows() and find_executable("powershell"):
            self.downloader = DownloadFileWithPowershell
            self.downloaderName = "powershell"
        else:
            self.downloader = DownloadFileWithUrllib
            self.downloaderName = "built-in"

        # CMake generator
        self.cmakeGenerator = args.generator

        # Number of jobs
        self.numJobs = args.jobs
        if self.numJobs <= 0:
            raise ValueError("Number of jobs must be greater than 0")

        # Build arguments
        self.buildArgs = dict()
        for a in args.build_args:
            (depName, _, arg) = a.partition(",")
            if not depName or not arg:
                raise ValueError("Invalid argument for --build-args: {}"
                                 .format(a))
            if depName.lower() not in AllDependenciesByName:
                raise ValueError("Invalid library for --build-args: {}"
                                 .format(depName))

            self.buildArgs.setdefault(depName.lower(), []).append(arg)

        # Build type
        self.buildShared = (args.build_type == SHARED_LIBS)
        self.buildMonolithic = (args.build_type == MONOLITHIC_LIB)

        # Dependencies that are forced to be built
        self.forceBuildAll = args.force_all
        self.forceBuild = [dep.lower() for dep in args.force_build]

        # Optional components
        self.buildTests = args.build_tests
        self.buildDocs = args.build_docs
        self.buildPython = args.build_python

        # - Imaging
        self.buildImaging = (args.build_imaging == IMAGING or
                             args.build_imaging == USD_IMAGING)
        self.enablePtex = self.buildImaging and args.enable_ptex

        # - USD Imaging
        self.buildUsdImaging = (args.build_imaging == USD_IMAGING)

        # - usdview
        self.buildUsdview = (self.buildUsdImaging and 
                             self.buildPython and 
                             args.build_usdview)

        # - Imaging plugins
        self.buildEmbree = self.buildImaging and args.build_embree
        self.embreeLocation = (os.path.abspath(args.embree_location)
                               if args.embree_location else None)
        self.buildOIIO = args.build_oiio

        # - Alembic Plugin
        self.buildAlembic = args.build_alembic
        self.enableHDF5 = self.buildAlembic and args.enable_hdf5

        # - MaterialX Plugin
        self.buildMaterialX = args.build_materialx

        # - Maya Plugin
        self.buildMaya = args.build_maya
        self.mayaLocation = (os.path.abspath(args.maya_location) 
                             if args.maya_location else None)

        # - Katana Plugin
        self.buildKatana = args.build_katana
        self.katanaApiLocation = (os.path.abspath(args.katana_api_location)
                                  if args.katana_api_location else None)

        # - Houdini Plugin
        self.buildHoudini = args.build_houdini
        self.houdiniLocation = (os.path.abspath(args.houdini_location)
                                if args.houdini_location else None)
       
    def GetBuildArguments(self, dep):
        return self.buildArgs.get(dep.name.lower(), [])
       
    def ForceBuildDependency(self, dep):
        # Never force building a Python dependency, since users are required
        # to build these dependencies themselves.
        if type(dep) is PythonDependency:
            return False
        return self.forceBuildAll or dep.name.lower() in self.forceBuild

try:
    context = InstallContext(args)
except Exception as e:
    PrintError(str(e))
    sys.exit(1)

verbosity = args.verbosity

# Augment PATH on Windows so that 3rd-party dependencies can find libraries
# they depend on. In particular, this is needed for building IlmBase/OpenEXR.
extraPaths = []
extraPythonPaths = []
if Windows():
    extraPaths.append(os.path.join(context.instDir, "lib"))
    extraPaths.append(os.path.join(context.instDir, "bin"))

if extraPaths:
    paths = os.environ.get('PATH', '').split(os.pathsep) + extraPaths
    os.environ['PATH'] = os.pathsep.join(paths)

if extraPythonPaths:
    paths = os.environ.get('PYTHONPATH', '').split(os.pathsep) + extraPythonPaths
    os.environ['PYTHONPATH'] = os.pathsep.join(paths)

# Determine list of dependencies that are required based on options
# user has selected.
requiredDependencies = [ZLIB, BOOST, TBB]

if context.buildAlembic:
    if context.enableHDF5:
        requiredDependencies += [HDF5]
    requiredDependencies += [OPENEXR, ALEMBIC]

if context.buildMaterialX:
    requiredDependencies += [MATERIALX]

if context.buildImaging:
    if context.enablePtex:
        requiredDependencies += [PTEX]

    requiredDependencies += [OPENEXR, GLEW, 
                             OPENSUBDIV]
    
    if context.buildOIIO:
        requiredDependencies += [JPEG, TIFF, PNG, OPENIMAGEIO]
                             
if context.buildUsdview:
    requiredDependencies += [PYOPENGL, PYSIDE]

# Assume zlib already exists on Linux platforms and don't build
# our own. This avoids potential issues where a host application
# loads an older version of zlib than the one we'd build and link
# our libraries against.
if Linux():
    requiredDependencies.remove(ZLIB)

# Error out if user explicitly specified building usdview without required
# components. Otherwise, usdview will be silently disabled. This lets users
# specify "--no-python" without explicitly having to specify "--no-usdview",
# for instance.
if "--usdview" in sys.argv:
    if not context.buildUsdImaging:
        PrintError("Cannot build usdview when usdImaging is disabled.")
        sys.exit(1)
    if not context.buildPython:
        PrintError("Cannot build usdview when Python support is disabled.")
        sys.exit(1)

# Error out if we try to build any third party plugins with python disabled.
if not context.buildPython:
    pythonPluginErrorMsg = (
        "%s plugin cannot be built when python support is disabled")
    if context.buildMaya:
        PrintError(pythonPluginErrorMsg % "Maya")
        sys.exit(1)
    if context.buildHoudini:
        PrintError(pythonPluginErrorMsg % "Houdini")
        sys.exit(1)
    if context.buildKatana:
        PrintError(pythonPluginErrorMsg % "Katana")
        sys.exit(1)

# Error out if we're building the Maya plugin and have enabled Ptex support
# in imaging. Maya includes its own copy of Ptex, which we believe is 
# version 2.0.41. We would need to build imaging against this version to
# avoid symbol lookup errors due to library version differences when running
# the Maya plugin. However, the current version of OpenImageIO requires
# a later version of Ptex. Rather than try to untangle this, we punt for
# now.
if context.buildMaya and PTEX in requiredDependencies:
    PrintError("Cannot enable Ptex support when building the Maya "
               "plugin, since using a separately-built Ptex library "
               "would conflict with the version used by Maya.")
    sys.exit(1)

dependenciesToBuild = []
for dep in requiredDependencies:
    if context.ForceBuildDependency(dep) or not dep.Exists(context):
        if dep not in dependenciesToBuild:
            dependenciesToBuild.append(dep)

# Verify toolchain needed to build required dependencies
if (not find_executable("g++") and
    not find_executable("clang") and
    not GetXcodeDeveloperDirectory() and
    not GetVisualStudioCompilerAndVersion()):
    PrintError("C++ compiler not found -- please install a compiler")
    sys.exit(1)

if not find_executable("python"):
    PrintError("python not found -- please ensure python is included in your "
               "PATH")
    sys.exit(1)

if not find_executable("cmake"):
    PrintError("CMake not found -- please install it and adjust your PATH")
    sys.exit(1)

if context.buildDocs:
    if not find_executable("doxygen"):
        PrintError("doxygen not found -- please install it and adjust your PATH")
        sys.exit(1)
        
    if not find_executable("dot"):
        PrintError("dot not found -- please install graphviz and adjust your "
                   "PATH")
        sys.exit(1)

if PYSIDE in requiredDependencies:
    # The USD build will skip building usdview if pyside-uic or pyside2-uic is 
    # not found, so check for it here to avoid confusing users. This list of 
    # PySide executable names comes from cmake/modules/FindPySide.cmake
    pysideUic = ["pyside-uic", "python2-pyside-uic", "pyside-uic-2.7"]
    found_pysideUic = any([find_executable(p) for p in pysideUic])
    pyside2Uic = ["pyside2-uic", "python2-pyside2-uic", "pyside2-uic-2.7"]
    found_pyside2Uic = any([find_executable(p) for p in pyside2Uic])
    if not found_pysideUic and not found_pyside2Uic:
        PrintError("pyside-uic not found -- please install PySide and adjust "
                   "your PATH. (Note that this program may be named {0} "
                   "depending on your platform)"
                   .format(" or ".join(pysideUic)))
        sys.exit(1)

if JPEG in requiredDependencies:
    # NASM is required to build libjpeg-turbo
    if (Windows() and not find_executable("nasm")):
        PrintError("nasm not found -- please install it and adjust your PATH")
        sys.exit(1)

# Summarize
summaryMsg = """
Building with settings:
  USD source directory          {usdSrcDir}
  USD install directory         {usdInstDir}
  3rd-party source directory    {srcDir}
  3rd-party install directory   {instDir}
  Build directory               {buildDir}
  CMake generator               {cmakeGenerator}
  Downloader                    {downloader}

  Building                      {buildType}
    Imaging                     {buildImaging}
      Ptex support:             {enablePtex}
      OpenImageIO support:      {buildOIIO} 
    UsdImaging                  {buildUsdImaging}
      usdview:                  {buildUsdview}
    Python support              {buildPython}
    Documentation               {buildDocs}
    Tests                       {buildTests}
    Alembic Plugin              {buildAlembic}
      HDF5 support:             {enableHDF5}
    MaterialX Plugin            {buildMaterialX}
    Maya Plugin                 {buildMaya}
    Katana Plugin               {buildKatana}
    Houdini Plugin              {buildHoudini}

  Dependencies                  {dependencies}"""

if context.buildArgs:
    summaryMsg += """
  Build arguments               {buildArgs}"""

def FormatBuildArguments(buildArgs):
    s = ""
    for depName in sorted(buildArgs.iterkeys()):
        args = buildArgs[depName]
        s += """
                                {name}: {args}""".format(
            name=AllDependenciesByName[depName].name,
            args=" ".join(args))
    return s.lstrip()

summaryMsg = summaryMsg.format(
    usdSrcDir=context.usdSrcDir,
    usdInstDir=context.usdInstDir,
    srcDir=context.srcDir,
    buildDir=context.buildDir,
    instDir=context.instDir,
    cmakeGenerator=("Default" if not context.cmakeGenerator
                    else context.cmakeGenerator),
    downloader=(context.downloaderName),
    dependencies=("None" if not dependenciesToBuild else 
                  ", ".join([d.name for d in dependenciesToBuild])),
    buildArgs=FormatBuildArguments(context.buildArgs),
    buildType=("Shared libraries" if context.buildShared
               else "Monolithic shared library" if context.buildMonolithic
               else ""),
    buildImaging=("On" if context.buildImaging else "Off"),
    enablePtex=("On" if context.enablePtex else "Off"),
    buildOIIO=("On" if context.buildOIIO else "Off"),
    buildUsdImaging=("On" if context.buildUsdImaging else "Off"),
    buildUsdview=("On" if context.buildUsdview else "Off"),
    buildPython=("On" if context.buildPython else "Off"),
    buildDocs=("On" if context.buildDocs else "Off"),
    buildTests=("On" if context.buildTests else "Off"),
    buildAlembic=("On" if context.buildAlembic else "Off"),
    buildMaterialX=("On" if context.buildMaterialX else "Off"),
    enableHDF5=("On" if context.enableHDF5 else "Off"),
    buildMaya=("On" if context.buildMaya else "Off"),
    buildKatana=("On" if context.buildKatana else "Off"),
    buildHoudini=("On" if context.buildHoudini else "Off"))

Print(summaryMsg)

if args.dry_run:
    sys.exit(0)

# Scan for any dependencies that the user is required to install themselves
# and print those instructions first.
pythonDependencies = \
    [dep for dep in dependenciesToBuild if type(dep) is PythonDependency]
if pythonDependencies:
    for dep in pythonDependencies:
        Print(dep.getInstructions())
    sys.exit(1)

# Ensure directory structure is created and is writable.
for dir in [context.usdInstDir, context.instDir, context.srcDir, 
            context.buildDir]:
    try:
        if os.path.isdir(dir):
            testFile = os.path.join(dir, "canwrite")
            open(testFile, "w").close()
            os.remove(testFile)
        else:
            os.makedirs(dir)
    except Exception as e:
        PrintError("Could not write to directory {dir}. Change permissions "
                   "or choose a different location to install to."
                   .format(dir=dir))
        sys.exit(1)

try:
    # Download and install 3rd-party dependencies, followed by USD.
    for dep in dependenciesToBuild + [USD]:
        PrintStatus("Installing {dep}...".format(dep=dep.name))
        dep.installer(context, 
                      buildArgs=context.GetBuildArguments(dep),
                      force=context.ForceBuildDependency(dep))
except Exception as e:
    PrintError(str(e))
    sys.exit(1)

# Done. Print out a final status message.
requiredInPythonPath = set([
    os.path.join(context.usdInstDir, "lib", "python")
])
requiredInPythonPath.update(extraPythonPaths)

requiredInPath = set([
    os.path.join(context.usdInstDir, "bin")
])
requiredInPath.update(extraPaths)

if Windows():
    requiredInPath.update([
        os.path.join(context.usdInstDir, "lib"),
        os.path.join(context.instDir, "bin"),
        os.path.join(context.instDir, "lib")
    ])

Print("""
Success! To use USD, please ensure that you have:""")

if context.buildPython:
    Print("""
    The following in your PYTHONPATH environment variable:
    {requiredInPythonPath}""".format(
        requiredInPythonPath="\n    ".join(sorted(requiredInPythonPath))))

Print("""
    The following in your PATH environment variable:
    {requiredInPath}
""".format(requiredInPath="\n    ".join(sorted(requiredInPath))))

if context.buildMaya:
    Print("See documentation at https://graphics.pixar.com/usd/docs/Maya-USD-Plugins.html "
          "for setting up the Maya plugin.\n")
    
if context.buildKatana:
    Print("See documentation at https://graphics.pixar.com/usd/docs/Katana-USD-Plugins.html "
          "for setting up the Katana plugin.\n")

if context.buildHoudini:
    Print("See documentation at https://graphics.pixar.com/usd/docs/Houdini-USD-Plugins.html "
          "for setting up the Houdini plugin.\n")
    
    
