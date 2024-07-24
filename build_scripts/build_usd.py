#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Check whether this script is being run under Python 2 first. Otherwise,
# any Python 3-only code below will cause the script to fail with an
# unhelpful error message.
import sys
if sys.version_info.major == 2:
    sys.exit("ERROR: USD does not support Python 2. Use a supported version "
             "of Python 3 instead.")

import argparse
import codecs
import contextlib
import ctypes
import datetime
import fnmatch
import glob
import locale
import multiprocessing
import os
import platform
import re
import shlex
import shutil
import subprocess
import sys
import sysconfig
import zipfile

from urllib.request import urlopen
from shutil import which

# Helpers for printing output
verbosity = 1

def Print(msg):
    if verbosity > 0:
        print(msg)

def PrintWarning(warning):
    if verbosity > 0:
        print("WARNING:", warning)

def PrintStatus(status):
    if verbosity >= 1:
        print("STATUS:", status)

def PrintInfo(info):
    if verbosity >= 2:
        print("INFO:", info)

def PrintCommandOutput(output):
    if verbosity >= 3:
        sys.stdout.write(output)

def PrintError(error):
    if verbosity >= 3 and sys.exc_info()[1] is not None:
        import traceback
        traceback.print_exc()
    print ("ERROR:", error)

# Helpers for determining platform
def Windows():
    return platform.system() == "Windows"
def Linux():
    return platform.system() == "Linux"
def MacOS():
    return platform.system() == "Darwin"

if MacOS():
    import apple_utils

def MacOSTargetEmbedded(context):
    return MacOS() and apple_utils.TargetEmbeddedOS(context)

def GetLocale():
    if Windows():
        # Windows handles encoding a little differently then Linux / Mac
        # For interactive streams (isatty() == True) it will use the
        # console codepage, otherwise it will return an ANSI codepage.
        # To keep things consistent we'll force UTF-8 for non-interactive
        # streams (which is recommended by Microsoft). See:
        # https://docs.python.org/3.6/library/sys.html#sys.stdout
        # https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
        if sys.stdout.isatty():
            return sys.stdout.encoding
        else:
            return "UTF-8"

    return sys.stdout.encoding or locale.getdefaultlocale()[1] or "UTF-8"

def GetCommandOutput(command):
    """Executes the specified command and returns output or None."""
    try:
        return subprocess.check_output(
            shlex.split(command), 
            stderr=subprocess.STDOUT).decode(GetLocale(), 'replace').strip()
    except subprocess.CalledProcessError:
        pass
    return None

def GetXcodeDeveloperDirectory():
    """Returns the active developer directory as reported by 'xcode-select -p'.
    Returns None if none is set."""
    if not MacOS():
        return None

    return GetCommandOutput("xcode-select -p")

def GetVisualStudioCompilerAndVersion():
    """Returns a tuple containing the path to the Visual Studio compiler
    and a tuple for its version, e.g. (14, 0). If the compiler is not found
    or version number cannot be determined, returns None."""
    if not Windows():
        return None

    msvcCompiler = which('cl')
    if msvcCompiler:
        # VCToolsVersion environment variable should be set by the
        # Visual Studio Command Prompt.
        match = re.search(
            r"(\d+)\.(\d+)",
            os.environ.get("VCToolsVersion", ""))
        if match:
            return (msvcCompiler, tuple(int(v) for v in match.groups()))
    return None

def IsVisualStudioVersionOrGreater(desiredVersion):
    if not Windows():
        return False

    msvcCompilerAndVersion = GetVisualStudioCompilerAndVersion()
    if msvcCompilerAndVersion:
        _, version = msvcCompilerAndVersion
        return version >= desiredVersion
    return False

# Helpers to determine the version of "Visual Studio" (also support the Build Tools) based
# on the version of the MSVC compiler.
# See MSVC++ versions table on https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B
def IsVisualStudio2022OrGreater():
    VISUAL_STUDIO_2022_VERSION = (14, 30)
    return IsVisualStudioVersionOrGreater(VISUAL_STUDIO_2022_VERSION)
def IsVisualStudio2019OrGreater():
    VISUAL_STUDIO_2019_VERSION = (14, 20)
    return IsVisualStudioVersionOrGreater(VISUAL_STUDIO_2019_VERSION)
def IsVisualStudio2017OrGreater():
    VISUAL_STUDIO_2017_VERSION = (14, 1)
    return IsVisualStudioVersionOrGreater(VISUAL_STUDIO_2017_VERSION)

def GetPythonInfo(context):
    """Returns a tuple containing the path to the Python executable, shared
    library, and include directory corresponding to the version of Python
    currently running. Returns None if any path could not be determined.

    This function is used to extract build information from the Python 
    interpreter used to launch this script. This information is used
    in the Boost and USD builds. By taking this approach we can support
    having USD builds for different Python versions built on the same
    machine. This is very useful, especially when developers have multiple
    versions installed on their machine.
    """

    # If we were given build python info then just use it.
    if context.build_python_info:
        return (context.build_python_info['PYTHON_EXECUTABLE'],
                context.build_python_info['PYTHON_LIBRARY'],
                context.build_python_info['PYTHON_INCLUDE_DIR'],
                context.build_python_info['PYTHON_VERSION'])

    # First we extract the information that can be uniformly dealt with across
    # the platforms:
    pythonExecPath = sys.executable
    pythonVersion = sysconfig.get_config_var("py_version_short")  # "3.7"

    # Lib path is unfortunately special for each platform and there is no
    # config_var for it. But we can deduce it for each platform, and this
    # logic works for any Python version.
    def _GetPythonLibraryFilename(context):
        if Windows():
            return "python{version}{suffix}.lib".format(
                version=sysconfig.get_config_var("py_version_nodot"),
                suffix=('_d' if context.buildDebug and context.debugPython
                        else ''))
        elif Linux():
            return sysconfig.get_config_var("LDLIBRARY")
        elif MacOS():
            return "libpython{version}.dylib".format(
                version=(sysconfig.get_config_var('LDVERSION') or
                         sysconfig.get_config_var('VERSION') or
                         pythonVersion))
        else:
            raise RuntimeError("Platform not supported")

    pythonIncludeDir = sysconfig.get_path("include")
    if not pythonIncludeDir or not os.path.isdir(pythonIncludeDir):
        # as a backup, and for legacy reasons - not preferred because
        # it may be baked at build time
        pythonIncludeDir = sysconfig.get_config_var("INCLUDEPY")

    # if in a venv, installed_base will be the "original" python,
    # which is where the libs are ("base" will be the venv dir)
    pythonBaseDir = sysconfig.get_config_var("installed_base")
    if Windows():
        pythonLibPath = os.path.join(pythonBaseDir, "libs",
                                     _GetPythonLibraryFilename(context))
    elif Linux():
        pythonMultiarchSubdir = sysconfig.get_config_var("multiarchsubdir")
        # Try multiple ways to get the python lib dir
        for pythonLibDir in (sysconfig.get_config_var("LIBDIR"),
                             os.path.join(pythonBaseDir, "lib")):
            if pythonMultiarchSubdir:
                pythonLibPath = \
                    os.path.join(pythonLibDir + pythonMultiarchSubdir,
                                 _GetPythonLibraryFilename(context))
                if os.path.isfile(pythonLibPath):
                    break
            pythonLibPath = os.path.join(pythonLibDir,
                                         _GetPythonLibraryFilename(context))
            if os.path.isfile(pythonLibPath):
                break
    elif MacOS():
        pythonLibPath = os.path.join(pythonBaseDir, "lib",
                                     _GetPythonLibraryFilename(context))
    else:
        raise RuntimeError("Platform not supported")

    return (pythonExecPath, pythonLibPath, pythonIncludeDir, pythonVersion)

def GetCPUCount():
    try:
        return multiprocessing.cpu_count()
    except NotImplementedError:
        return 1

def Run(cmd, logCommandOutput = True, env = None):
    """Run the specified command in a subprocess."""
    PrintInfo('Running "{cmd}"'.format(cmd=cmd))

    with codecs.open("log.txt", "a", "utf-8") as logfile:
        logfile.write(datetime.datetime.now().strftime("%Y-%m-%d %H:%M"))
        logfile.write("\n")
        logfile.write(cmd)
        logfile.write("\n")

        # Let exceptions escape from subprocess calls -- higher level
        # code will handle them.
        if logCommandOutput:
            p = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE, 
                                 stderr=subprocess.STDOUT, env=env)
            while True:
                l = p.stdout.readline().decode(GetLocale(), 'replace')
                if l:
                    logfile.write(l)
                    PrintCommandOutput(l)
                elif p.poll() is not None:
                    break
        else:
            p = subprocess.Popen(shlex.split(cmd), env=env)
            p.wait()

    if p.returncode != 0:
        # If verbosity >= 3, we'll have already been printing out command output
        # so no reason to print the log file again.
        if verbosity < 3:
            with open("log.txt", "r") as logfile:
                Print(logfile.read())
        raise RuntimeError("Failed to run '{cmd}' in {path}.\nSee {log} for more details."
                           .format(cmd=cmd, path=os.getcwd(), log=os.path.abspath("log.txt")))

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
    if not os.path.isdir(instDestDir):
        try:
            os.mkdir(instDestDir)
        except Exception as e:
            raise RuntimeError(
                "Unable to create {destDir}".format(destDir=instDestDir)) from e

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

def AppendCXX11ABIArg(buildFlag, context, buildArgs):
    """Append a build argument that defines _GLIBCXX_USE_CXX11_ABI
    based on the settings in the context. This may either do nothing
    or append an entry to buildArgs like:

      <buildFlag>="-D_GLIBCXX_USE_CXX11_ABI={0, 1}"

    If buildArgs contains settings for buildFlag, those settings will
    be merged with the above define."""
    if context.useCXX11ABI is None:
        return

    cxxFlags = ["-D_GLIBCXX_USE_CXX11_ABI={}".format(context.useCXX11ABI)]
    
    # buildArgs might look like:
    # ["-DFOO=1", "-DBAR=2", ...] or ["-DFOO=1 -DBAR=2 ...", ...]
    #
    # See if any of the arguments in buildArgs start with the given
    # buildFlag. If so, we want to take whatever that buildFlag has
    # been set to and merge it in with the cxxFlags above.
    #
    # For example, if buildArgs = ['-DCMAKE_CXX_FLAGS="-w"', ...]
    # we want to add "-w" to cxxFlags.
    splitArgs = [shlex.split(a) for a in buildArgs]
    for p in [item for arg in splitArgs for item in arg]:
        if p.startswith(buildFlag):
            (_, _, flags) = p.partition("=")
            cxxFlags.append(flags)

    buildArgs.append('{flag}="{flags}"'.format(
        flag=buildFlag, flags=" ".join(cxxFlags)))

def FormatMultiProcs(numJobs, generator):
    tag = "-j"
    if generator:
        if "Visual Studio" in generator:
            tag = "/M:" # This will build multiple projects at once.
        elif "Xcode" in generator:
            tag = "-j "

    return "{tag}{procs}".format(tag=tag, procs=numJobs)

def RunCMake(context, force, extraArgs = None):
    """Invoke CMake to configure, build, and install a library whose 
    source code is located in the current working directory."""
    # Create a directory for out-of-source builds in the build directory
    # using the name of the current working directory.
    if extraArgs is None:
        extraArgs = []
    else:
        # ensure we can freely modify our extraArgs without affecting caller
        extraArgs = list(extraArgs)

    if context.cmakeBuildArgs:
        extraArgs.insert(0, context.cmakeBuildArgs)
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
    # TODO: figure out exactly what "vcvarsall.bat x64" sets to force x64
    if generator is None and Windows():
        if IsVisualStudio2022OrGreater():
            generator = "Visual Studio 17 2022"
        elif IsVisualStudio2019OrGreater():
            generator = "Visual Studio 16 2019"
        elif IsVisualStudio2017OrGreater():
            generator = "Visual Studio 15 2017 Win64"

    if generator is not None:
        generator = '-G "{gen}"'.format(gen=generator)

    # Note - don't want to add -A (architecture flag) if generator is, ie, Ninja
    if IsVisualStudio2019OrGreater() and "Visual Studio" in generator:
        generator = generator + " -A x64"

    toolset = context.cmakeToolset
    if toolset is not None:
        toolset = '-T "{toolset}"'.format(toolset=toolset)

    # On MacOS, enable the use of @rpath for relocatable builds.
    osx_rpath = None
    if MacOS():
        osx_rpath = "-DCMAKE_MACOSX_RPATH=ON"

        # For macOS cross compilation, set the Xcode architecture flags.
        targetArch = apple_utils.GetTargetArch(context)

        if context.targetNative or targetArch == apple_utils.GetHostArch():
            extraArgs.append('-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=YES')
        else:
            extraArgs.append('-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO')

        extraArgs.append('-DCMAKE_OSX_ARCHITECTURES={0}'.format(targetArch))
        extraArgs = apple_utils.ConfigureCMakeExtraArgs(context, extraArgs)

    if context.ignorePaths:
        ignoredPaths = ";".join(context.ignorePaths)
        extraArgs.append("-DCMAKE_IGNORE_PATH={0}".format(ignoredPaths))
        extraArgs.append("-DCMAKE_IGNORE_PREFIX_PATH={0}".format(ignoredPaths))

    # We use -DCMAKE_BUILD_TYPE for single-configuration generators 
    # (Ninja, make), and --config for multi-configuration generators 
    # (Visual Studio); technically we don't need BOTH at the same
    # time, but specifying both is simpler than branching
    config = "Release"
    if context.buildDebug:
        config = "Debug"
    elif context.buildRelease:
        config = "Release"
    elif context.buildRelWithDebug:
        config = "RelWithDebInfo"

    # Append extra argument controlling libstdc++ ABI if specified.
    AppendCXX11ABIArg("-DCMAKE_CXX_FLAGS", context, extraArgs)

    with CurrentWorkingDirectory(buildDir):
        Run('cmake '
            '-DCMAKE_INSTALL_PREFIX="{instDir}" '
            '-DCMAKE_PREFIX_PATH="{depsInstDir}" '
            '-DCMAKE_BUILD_TYPE={config} '
            '{osx_rpath} '
            '{generator} '
            '{toolset} '
            '{extraArgs} '
            '"{srcDir}"'
            .format(instDir=instDir,
                    depsInstDir=context.instDir,
                    config=config,
                    srcDir=srcDir,
                    osx_rpath=(osx_rpath or ""),
                    generator=(generator or ""),
                    toolset=(toolset or ""),
                    extraArgs=(" ".join(extraArgs) if extraArgs else "")))
        Run("cmake --build . --config {config} --target install -- {multiproc}"
            .format(config=config,
                    multiproc=FormatMultiProcs(context.numJobs, generator)))

def GetCMakeVersion():
    """
    Returns the CMake version as tuple of integers (major, minor) or
    (major, minor, patch) or None if an error occured while launching cmake and
    parsing its output.
    """

    output_string = GetCommandOutput("cmake --version")
    if not output_string:
        PrintWarning("Could not determine cmake version -- please install it "
                     "and adjust your PATH")
        return None

    # cmake reports, e.g., "... version 3.14.3"
    match = re.search(r"version (\d+)\.(\d+)(\.(\d+))?", output_string)
    if not match:
        PrintWarning("Could not determine cmake version")
        return None

    major, minor, patch_group, patch = match.groups()
    if patch_group is None:
        return (int(major), int(minor))
    else:
        return (int(major), int(minor), int(patch))

def PatchFile(filename, patches, multiLineMatches=False):
    """Applies patches to the specified file. patches is a list of tuples
    (old string, new string)."""
    if multiLineMatches:
        oldLines = [open(filename, 'r').read()]
    else:
        oldLines = open(filename, 'r').readlines()
    newLines = oldLines
    for (oldString, newString) in patches:
        newLines = [s.replace(oldString, newString) for s in newLines]
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
    r = urlopen(url)
    with open(outputFilename, "wb") as outfile:
        outfile.write(r.read())

def DownloadURL(url, context, force, extractDir = None, 
        dontExtract = None):
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

            for i in range(maxRetries):
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
        # of the contents beneath it, unless a specific extractDir is specified,
        # which is to be used.
        archive = None
        rootDir = None
        members = None
        try:
            if zipfile.is_zipfile(filename):
                archive = zipfile.ZipFile(filename)
                if extractDir:
                    rootDir = extractDir
                else:
                    rootDir = archive.namelist()[0].split('/')[0]
                if dontExtract != None:
                    members = (m for m in archive.namelist() 
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
        return any([os.path.isfile(os.path.join(context.instDir, f))
                    for f in self.filesToCheck])

class PythonDependency(object):
    def __init__(self, name, getInstructions, moduleNames):
        self.name = name
        self.getInstructions = getInstructions
        self.moduleNames = moduleNames

    def Exists(self, context):
        # If one of the modules in our list imports successfully, we are good.
        for moduleName in self.moduleNames:
            try:
                pyModule = __import__(moduleName)
                return True
            except:
                pass

        return False

def AnyPythonDependencies(deps):
    return any([type(d) is PythonDependency for d in deps])

############################################################
# zlib

ZLIB_URL = "https://github.com/madler/zlib/archive/v1.2.13.zip"

def InstallZlib(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(ZLIB_URL, context, force)):
        # The following test files aren't portable to embedded platforms.
        # They're not required for use on any platforms, so we elide them
        # for efficiency
        PatchFile("CMakeLists.txt",
                  [("add_executable(example test/example.c)",
                    ""),
                   ("add_executable(minigzip test/minigzip.c)",
                    ""),
                   ("target_link_libraries(example zlib)",
                    ""),
                   ("target_link_libraries(minigzip zlib)",
                    ""),
                   ("add_test(example example)",
                    "")])
        RunCMake(context, force, buildArgs)

ZLIB = Dependency("zlib", InstallZlib, "include/zlib.h")
        
############################################################
# boost

# The default installation of boost on Windows puts headers in a versioned 
# subdirectory, which we have to account for here. Specifying "layout=system" 
# would cause the Windows header install to match Linux/MacOS, but the 
# "layout=system" flag also changes the naming of the boost dlls in a 
# manner that causes problems for dependent libraries that rely on boost's
# trick of automatically linking the boost libraries via pragmas in boost's
# standard include files. Dependencies that use boost's pragma linking
# facility in general don't have enough configuration switches to also coerce 
# the naming of the dlls and so it is best to rely on boost's most default
# settings for maximum compatibility.
#
# It's tricky to determine which version of boost we'll be using at this
# point in the script, so we simplify the logic by checking for any of the
# possible boost header locations that are possible outcomes from running
# this script.
BOOST_VERSION_FILES = [
    "include/boost/version.hpp",
    "include/boost-1_76/boost/version.hpp",
    "include/boost-1_78/boost/version.hpp",
    "include/boost-1_82/boost/version.hpp"
]

def InstallBoost_Helper(context, force, buildArgs):
    # In general we use boost 1.76.0 to adhere to VFX Reference Platform CY2022.
    # However, there are some cases where a newer version is required.
    # - Building with Python 3.11 requires boost 1.82.0 or newer
    #   (https://github.com/boostorg/python/commit/a218ba)
    # - Building on MacOS requires v1.82.0 or later for C++17 support starting 
    #   with Xcode 15. We choose to use this version for all MacOS builds for 
    #   simplicity."
    # - Building with Python 3.10 requires boost 1.76.0 or newer
    #   (https://github.com/boostorg/python/commit/cbd2d9)
    #   XXX: Due to a typo we've been using 1.78.0 in this case for a while.
    #        We're leaving it that way to minimize potential disruption.
    # - Building with Visual Studio 2022 requires boost 1.78.0 or newer.
    #   (https://github.com/boostorg/build/issues/735)
    # - Building on MacOS requires boost 1.78.0 or newer to resolve Python 3
    #   compatibility issues on Big Sur and Monterey.
    pyInfo = GetPythonInfo(context)
    pyVer = (int(pyInfo[3].split('.')[0]), int(pyInfo[3].split('.')[1]))
    if MacOS() or (context.buildBoostPython and pyVer >= (3,11)):
        BOOST_URL = "https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.zip"
    elif context.buildBoostPython and pyVer >= (3, 10):
        BOOST_URL = "https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.zip"
    elif IsVisualStudio2022OrGreater():
        BOOST_URL = "https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.zip"
    else:
        BOOST_URL = "https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.zip"

    # Documentation files in the boost archive can have exceptionally
    # long paths. This can lead to errors when extracting boost on Windows,
    # since paths are limited to 260 characters by default on that platform.
    # To avoid this, we skip extracting all documentation.
    #
    # For some examples, see: https://svn.boost.org/trac10/ticket/11677
    dontExtract = [
        "*/doc/*",
        "*/libs/*/doc/*",
        "*/libs/wave/test/testwave/testfiles/utf8-test-*"
    ]

    with CurrentWorkingDirectory(DownloadURL(BOOST_URL, context, force, 
                                             dontExtract=dontExtract)):
        if Windows():
            bootstrap = "bootstrap.bat"
        else:
            bootstrap = "./bootstrap.sh"
            # zip doesn't preserve file attributes, so force +x manually.
            Run('chmod +x ' + bootstrap)
            Run('chmod +x ./tools/build/src/engine/build.sh')

        # For cross-compilation on macOS we need to specify the architecture
        # for both the bootstrap and the b2 phase of building boost.
        bootstrapCmd = '{bootstrap} --prefix="{instDir}"'.format(
            bootstrap=bootstrap, instDir=context.instDir)

        macOSArch = ""

        if MacOS():
            if apple_utils.GetTargetArch(context) == \
                        apple_utils.TARGET_X86:
                macOSArch = "-arch {0}".format(apple_utils.TARGET_X86)
            elif apple_utils.GetTargetArch(context) == \
                        apple_utils.GetTargetArmArch():
                macOSArch = "-arch {0}".format(
                        apple_utils.GetTargetArmArch())
            elif context.targetUniversal:
                (primaryArch, secondaryArch) = \
                        apple_utils.GetTargetArchPair(context)
                macOSArch="-arch {0} -arch {1}".format(
                        primaryArch, secondaryArch)

            if macOSArch:
                bootstrapCmd += " cxxflags=\"{0} -std=c++17 -stdlib=libc++\" " \
                                " cflags=\"{0}\" " \
                                " linkflags=\"{0}\"".format(macOSArch)
            bootstrapCmd += " --with-toolset=clang"

        Run(bootstrapCmd)

        # b2 supports at most -j64 and will error if given a higher value.
        num_procs = min(64, context.numJobs)

        # boost only accepts three variants: debug, release, profile
        boostBuildVariant = "profile"
        if context.buildDebug:
            boostBuildVariant= "debug"
        elif context.buildRelease:
            boostBuildVariant= "release"
        elif context.buildRelWithDebug:
            boostBuildVariant= "profile"

        b2_settings = [
            '--prefix="{instDir}"'.format(instDir=context.instDir),
            '--build-dir="{buildDir}"'.format(buildDir=context.buildDir),
            '-j{procs}'.format(procs=num_procs),
            'address-model=64',
            'link=shared',
            'runtime-link=shared',
            'threading=multi', 
            'variant={variant}'.format(variant=boostBuildVariant),
            '--with-atomic',
            '--with-regex'
        ]

        if context.buildBoostPython:
            b2_settings.append("--with-python")
            pythonInfo = GetPythonInfo(context)
            # This is the only platform-independent way to configure these
            # settings correctly and robustly for the Boost jam build system.
            # There are Python config arguments that can be passed to bootstrap 
            # but those are not available in boostrap.bat (Windows) so we must 
            # take the following approach:
            projectPath = 'python-config.jam'
            with open(projectPath, 'w') as projectFile:
                # Note that we must escape any special characters, like 
                # backslashes for jam, hence the mods below for the path 
                # arguments. Also, if the path contains spaces jam will not
                # handle them well. Surround the path parameters in quotes.
                projectFile.write('using python : %s\n' % pythonInfo[3])
                projectFile.write('  : "%s"\n' % pythonInfo[0].replace("\\","/"))
                projectFile.write('  : "%s"\n' % pythonInfo[2].replace("\\","/"))
                projectFile.write('  : "%s"\n' % os.path.dirname(pythonInfo[1]).replace("\\","/"))
                if context.buildDebug and context.debugPython:
                    projectFile.write('  : <python-debugging>on\n')
                projectFile.write('  ;\n')
            b2_settings.append("--user-config=python-config.jam")

            if context.buildDebug and context.debugPython:
                b2_settings.append("python-debugging=on")

        if context.buildOIIO:
            b2_settings.append("--with-date_time")

        if context.buildOIIO or context.enableOpenVDB:
            b2_settings.append("--with-system")
            b2_settings.append("--with-thread")

        if context.enableOpenVDB:
            b2_settings.append("--with-iostreams")

            # b2 with -sNO_COMPRESSION=1 fails with the following error message:
            #     error: at [...]/boost_1_61_0/tools/build/src/kernel/modules.jam:107
            #     error: Unable to find file or target named
            #     error:     '/zlib//zlib'
            #     error: referred to from project at
            #     error:     'libs/iostreams/build'
            #     error: could not resolve project reference '/zlib'

            # But to avoid an extra library dependency, we can still explicitly
            # exclude the bzip2 compression from boost_iostreams (note that
            # OpenVDB uses blosc compression).
            b2_settings.append("-sNO_BZIP2=1")

        if context.buildOIIO:
            b2_settings.append("--with-filesystem")

        if force:
            b2_settings.append("-a")

        if Windows():
            # toolset parameter for Visual Studio documented here:
            # https://github.com/boostorg/build/blob/develop/src/tools/msvc.jam
            if context.cmakeToolset == "v143":
                b2_settings.append("toolset=msvc-14.3")
            elif context.cmakeToolset == "v142":
                b2_settings.append("toolset=msvc-14.2")
            elif context.cmakeToolset == "v141":
                b2_settings.append("toolset=msvc-14.1")
            elif IsVisualStudio2022OrGreater():
                b2_settings.append("toolset=msvc-14.3")
            elif IsVisualStudio2019OrGreater():
                b2_settings.append("toolset=msvc-14.2")
            elif IsVisualStudio2017OrGreater():
                b2_settings.append("toolset=msvc-14.1")

        if MacOS():
            # Must specify toolset=clang to ensure install_name for boost
            # libraries includes @rpath
            b2_settings.append("toolset=clang")
            #
            # Xcode 15.3 (and hence Apple Clang 15) removed the global
            # declaration of std::piecewise_construct which causes boost build
            # to fail.
            # https://developer.apple.com/documentation/xcode-release-notes/xcode-15_3-release-notes.
            # A fix for the same is also available in boost 1.84:
            # https://github.com/boostorg/container/commit/79a75f470e75f35f5f2a91e10fcc67d03b0a2160
            b2_settings.append(f"define=BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT=0")
            if macOSArch:
                b2_settings.append("cxxflags=\"{0} -std=c++17 -stdlib=libc++\"".format(macOSArch))
                b2_settings.append("cflags=\"{0}\"".format(macOSArch))
                b2_settings.append("linkflags=\"{0}\"".format(macOSArch))

        if context.buildDebug:
            b2_settings.append("--debug-configuration")

        # Add on any user-specified extra arguments.
        b2_settings += buildArgs

        # Append extra argument controlling libstdc++ ABI if specified.
        AppendCXX11ABIArg("cxxflags", context, b2_settings)

        b2 = "b2" if Windows() else "./b2"
        Run('{b2} {options} install'
            .format(b2=b2, options=" ".join(b2_settings)))

def InstallBoost(context, force, buildArgs):
    # Boost's build system will install the version.hpp header before
    # building its libraries. We make sure to remove it in case of
    # any failure to ensure that the build script detects boost as a 
    # dependency to build the next time it's run.
    try:
        InstallBoost_Helper(context, force, buildArgs)
    except:
        for versionHeader in [
            os.path.join(context.instDir, f) for f in BOOST_VERSION_FILES
        ]:
            if os.path.isfile(versionHeader):
                try: os.remove(versionHeader)
                except: pass
        raise

BOOST = Dependency("boost", InstallBoost, *BOOST_VERSION_FILES)

############################################################
# Intel oneTBB

ONETBB_URL = "https://github.com/oneapi-src/oneTBB/archive/refs/tags/v2021.9.0.zip"

def InstallOneTBB(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(ONETBB_URL, context, force)):
        RunCMake(context, force, 
                 ['-DTBB_TEST=OFF',
                  '-DTBB_STRICT=OFF'] + buildArgs)

ONETBB = Dependency("oneTBB", InstallOneTBB, "include/oneapi/tbb.h")

############################################################
# Intel TBB

if Windows():
    TBB_URL = "https://github.com/oneapi-src/oneTBB/releases/download/v2020.3/tbb-2020.3-win.zip"
    TBB_ROOT_DIR_NAME = "tbb"
elif MacOS():
    # On MacOS Intel systems we experience various crashes in tests during
    # teardown starting with 2018 Update 2. Until we figure that out, we use
    # 2018 Update 1 on this platform.
    TBB_URL = "https://github.com/oneapi-src/oneTBB/archive/refs/tags/v2020.3.zip"
    TBB_INTEL_URL = "https://github.com/oneapi-src/oneTBB/archive/refs/tags/2018_U1.zip"
else:
    # Use point release with fix https://github.com/oneapi-src/oneTBB/pull/833
    TBB_URL = "https://github.com/oneapi-src/oneTBB/archive/refs/tags/v2020.3.1.zip"

def InstallTBB(context, force, buildArgs):
    if Windows():
        InstallTBB_Windows(context, force, buildArgs)
    elif MacOS():
        InstallTBB_MacOS(context, force, buildArgs)
    else:
        InstallTBB_Linux(context, force, buildArgs)

def InstallTBB_Windows(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(TBB_URL, context, force, 
        TBB_ROOT_DIR_NAME)):
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

def InstallTBB_MacOS(context, force, buildArgs):
    tbb_url = TBB_URL if apple_utils.IsTargetArm(context) else TBB_INTEL_URL
    with CurrentWorkingDirectory(DownloadURL(tbb_url, context, force)):
        # Ensure that the tbb build system picks the proper architecture.
        PatchFile("build/macos.clang.inc",
                [("-m64",
                  "-m64 -arch {0}".format(apple_utils.TARGET_X86)),
                 ("ifeq ($(arch),$(filter $(arch),armv7 armv7s arm64))",
                  "ifeq ($(arch),$(filter $(arch),armv7 armv7s {0}))"
                        .format(apple_utils.GetTargetArmArch()))])

        if context.buildTarget == apple_utils.TARGET_VISIONOS:
            # Create visionOS config from iOS config
            shutil.copy(
                src="build/ios.macos.inc",
                dst="build/visionos.macos.inc")

            PatchFile("build/visionos.macos.inc",
                      [("ios","visionos"),
                       ("iOS", "visionOS"),
                       ("iPhone", "XR"),
                       ("IPHONEOS","XROS"),
                       ("?= 8.0", "?= 1.0")])

            # iOS clang just reuses the macOS one,
            # so it's easier to copy it directly.
            shutil.copy(src="build/macos.clang.inc",
                        dst="build/visionos.clang.inc")

            PatchFile("build/visionos.clang.inc",
                      [("ios","visionos"),
                       ("-miphoneos-version-min=", "-target arm64-apple-xros"),
                       ("iOS", "visionOS"),
                       ("iPhone", "XR"),
                       ("IPHONEOS","XROS")])

        (primaryArch, secondaryArch) = apple_utils.GetTargetArchPair(context)

        # tbb uses different arch names
        if (primaryArch == apple_utils.TARGET_X86):
            primaryArch = "intel64"
        if (secondaryArch == apple_utils.TARGET_X86):
            secondaryArch = "intel64"

        # Install both release and debug builds.
        # See comments in InstallTBB_Linux.
        def _RunBuild(arch):
            if not arch:
                return
            env = os.environ.copy()
            if MacOSTargetEmbedded(context):
                env["SDKROOT"] = apple_utils.GetSDKRoot(context)
                buildArgs.append(f' compiler=clang arch=arm64 extra_inc=big_iron.inc target={context.buildTarget.lower()}')
            makeTBBCmd = 'make -j{procs} arch={arch} {buildArgs}'.format(
                arch=arch, procs=context.numJobs,
                buildArgs=" ".join(buildArgs))
            Run(makeTBBCmd, env=env)

        _RunBuild(primaryArch)
        _RunBuild(secondaryArch)

        # See comments in InstallTBB_Linux about why we patch the Makefile
        # and rerun builds. This is only required for TBB 2020; 2019 and
        # earlier build both release and debug, and 2021 has moved to CMake.
        if "2020" in tbb_url:
            PatchFile("Makefile", [("release", "debug")])
            _RunBuild(primaryArch)
            _RunBuild(secondaryArch)

        if context.targetUniversal:
            x86Files = glob.glob(os.getcwd() +
                "/build/*intel64*_release/libtbb*.*")
            armFiles = glob.glob(os.getcwd() +
                "/build/*{0}*_release/libtbb*.*".format(
                        apple_utils.GetTargetArmArch()))
            libNames = [os.path.basename(x) for x in x86Files]
            x86Dir = os.path.dirname(x86Files[0])
            armDir = os.path.dirname(armFiles[0])

            apple_utils.CreateUniversalBinaries(context, libNames, x86Dir, armDir)

            x86Files = glob.glob(
                os.getcwd() + "/build/*intel64*_debug/libtbb*.*")
            armFiles = glob.glob(
                os.getcwd() + "/build/*{0}*_debug/libtbb*.*".format(
                        apple_utils.GetTargetArmArch()))
            if x86Files and armFiles:
                libNames = [os.path.basename(x) for x in x86Files]
                x86Dir = os.path.dirname(x86Files[0])
                armDir = os.path.dirname(armFiles[0])

                apple_utils.CreateUniversalBinaries(context, libNames, x86Dir, armDir)
        else:
            CopyFiles(context, "build/*_release/libtbb*.*", "lib")
            CopyFiles(context, "build/*_debug/libtbb*.*", "lib")

        CopyDirectory(context, "include/serial", "include/serial")
        CopyDirectory(context, "include/tbb", "include/tbb")

def InstallTBB_Linux(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(TBB_URL, context, force)):
        # Append extra argument controlling libstdc++ ABI if specified.
        AppendCXX11ABIArg("CXXFLAGS", context, buildArgs)

        # TBB does not support out-of-source builds in a custom location.
        makeTBBCmd = 'make -j{procs} {buildArgs}'.format(
            procs=context.numJobs, 
            buildArgs=" ".join(buildArgs))
        Run(makeTBBCmd)

        # Install both release and debug builds. USD requires the debug
        # libraries when building in debug mode, and installing both
        # makes it easier for users to install dependencies in some
        # location that can be shared by both release and debug USD
        # builds.
        #
        # As of TBB 2020 the build no longer produces a debug build along
        # with the release build. There is also no way to specify a debug
        # build when running make, even though the internals of the build
        # still support it.
        #
        # To workaround this, we patch the Makefile to change "release" to
        # "debug" and re-run the build, copying the debug libraries
        # afterwards. 
        #
        # See https://github.com/oneapi-src/oneTBB/issues/207/
        PatchFile("Makefile", [("release", "debug")])
        Run(makeTBBCmd)

        CopyFiles(context, "build/*_release/libtbb*.*", "lib")
        CopyFiles(context, "build/*_debug/libtbb*.*", "lib")
        CopyDirectory(context, "include/serial", "include/serial")
        CopyDirectory(context, "include/tbb", "include/tbb")

TBB = Dependency("TBB", InstallTBB, "include/tbb/tbb.h")

############################################################
# JPEG

JPEG_URL = "https://github.com/libjpeg-turbo/libjpeg-turbo/archive/2.0.1.zip"

def InstallJPEG(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(JPEG_URL, context, force)):
        extraJPEGArgs = buildArgs
        if not which("nasm"):
            extraJPEGArgs.append("-DWITH_SIMD=FALSE")

        RunCMake(context, force, extraJPEGArgs)
        return os.getcwd()

JPEG = Dependency("JPEG", InstallJPEG, "include/jpeglib.h")
        
############################################################
# TIFF

TIFF_URL = "https://gitlab.com/libtiff/libtiff/-/archive/v4.0.7/libtiff-v4.0.7.zip"

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

        # The libTIFF CMakeScript says the ld-version-script 
        # functionality is only for compilers using GNU ld on 
        # ELF systems or systems which provide an emulation; therefore
        # skipping it completely on mac and windows.
        if MacOS() or Windows():
            extraArgs = ["-Dld-version-script=OFF"]
        else:
            extraArgs = []
        extraArgs += buildArgs
        RunCMake(context, force, extraArgs)

TIFF = Dependency("TIFF", InstallTIFF, "include/tiff.h")

############################################################
# PNG

PNG_URL = "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.38.zip"

def InstallPNG(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(PNG_URL, context, force)):
        macArgs = []
        if MacOS() and apple_utils.IsTargetArm(context):
            # Ensure libpng's build doesn't erroneously activate inappropriate
            # Neon extensions
            macArgs = ["-DCMAKE_C_FLAGS=\"-DPNG_ARM_NEON_OPT=0\""]

            if context.targetUniversal:
                PatchFile("scripts/genout.cmake.in",
                [("CMAKE_OSX_ARCHITECTURES",
                  "CMAKE_OSX_INTERNAL_ARCHITECTURES")])

        RunCMake(context, force, buildArgs + macArgs)

PNG = Dependency("PNG", InstallPNG, "include/png.h")

############################################################
# IlmBase/OpenEXR

OPENEXR_URL = "https://github.com/AcademySoftwareFoundation/openexr/archive/refs/tags/v3.1.11.zip"

def InstallOpenEXR(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(OPENEXR_URL, context, force)):
        RunCMake(context, force, 
                 ['-DOPENEXR_INSTALL_TOOLS=OFF',
                  '-DOPENEXR_INSTALL_EXAMPLES=OFF',

                  # Force OpenEXR to build and use a separate Imath library
                  # instead of looking for one externally. This ensures that
                  # OpenEXR and other dependencies use the Imath library
                  # built via this script.
                  '-DOPENEXR_FORCE_INTERNAL_IMATH=ON',
                  '-DBUILD_TESTING=OFF'] + buildArgs)

OPENEXR = Dependency("OpenEXR", InstallOpenEXR, "include/OpenEXR/ImfVersion.h")

############################################################
# Ptex

PTEX_URL = "https://github.com/wdas/ptex/archive/refs/tags/v2.4.2.zip"
PTEX_VERSION = "v2.4.2"

def InstallPtex(context, force, buildArgs):
    cmakeOptions = [
        '-DBUILD_TESTING=OFF',
        '-DPTEX_BUILD_STATIC_LIBS=OFF',
        # We must tell the Ptex build system what version we're building
        # otherwise we get errors when running CMake.
        '-DPTEX_VER={v}'.format(v=PTEX_VERSION)
    ]
    cmakeOptions += buildArgs

    with CurrentWorkingDirectory(DownloadURL(PTEX_URL, context, force)):
        RunCMake(context, force, cmakeOptions)

PTEX = Dependency("Ptex", InstallPtex, "include/PtexVersion.h")

############################################################
# BLOSC (Compression used by OpenVDB)

# Using blosc v1.20.1 to avoid build errors on macOS Catalina (10.15)
# related to implicit declaration of functions in zlib. See:
# https://github.com/Blosc/python-blosc/issues/229
BLOSC_URL = "https://github.com/Blosc/c-blosc/archive/v1.20.1.zip"

def InstallBLOSC(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(BLOSC_URL, context, force)):
        macArgs = []
        if MacOS() and apple_utils.IsTargetArm(context):
            # Need to disable SSE for macOS ARM targets.
            macArgs = ["-DDEACTIVATE_SSE2=ON"]
        RunCMake(context, force, buildArgs + macArgs)

BLOSC = Dependency("Blosc", InstallBLOSC, "include/blosc.h")

############################################################
# OpenVDB

OPENVDB_URL = "https://github.com/AcademySoftwareFoundation/openvdb/archive/refs/tags/v9.1.0.zip"

# OpenVDB v9.1.0 requires TBB 2019.0 or above, but this script installs
# TBB 2018 on macOS Intel systems for reasons documented above. So we
# keep OpenVDB at the version specified for the VFX Reference Platform
# CY2021, which is the last version that supported 2018.
OPENVDB_INTEL_URL = "https://github.com/AcademySoftwareFoundation/openvdb/archive/refs/tags/v8.2.0.zip"

def InstallOpenVDB(context, force, buildArgs):
    openvdb_url = OPENVDB_URL
    if MacOS() and not apple_utils.IsTargetArm(context):
        openvdb_url = OPENVDB_INTEL_URL

    with CurrentWorkingDirectory(DownloadURL(openvdb_url, context, force)):
        extraArgs = [
            '-DOPENVDB_BUILD_PYTHON_MODULE=OFF',
            '-DOPENVDB_BUILD_BINARIES=OFF',
            '-DOPENVDB_BUILD_UNITTESTS=OFF'
        ]

        # Make sure to use boost installed by the build script and not any
        # system installed boost
        extraArgs.append('-DBoost_NO_BOOST_CMAKE=On')
        extraArgs.append('-DBoost_NO_SYSTEM_PATHS=True')

        extraArgs.append('-DBLOSC_ROOT="{instDir}"'
                         .format(instDir=context.instDir))
        extraArgs.append('-DTBB_ROOT="{instDir}"'
                         .format(instDir=context.instDir))
        # OpenVDB needs Half type from IlmBase
        extraArgs.append('-DILMBASE_ROOT="{instDir}"'
                         .format(instDir=context.instDir))

        # Add on any user-specified extra arguments.
        extraArgs += buildArgs

        RunCMake(context, force, extraArgs)

OPENVDB = Dependency("OpenVDB", InstallOpenVDB, "include/openvdb/openvdb.h")

############################################################
# OpenImageIO

OIIO_URL = "https://github.com/OpenImageIO/oiio/archive/refs/tags/v2.3.21.0.zip"

def InstallOpenImageIO(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(OIIO_URL, context, force)):
        # The only time that we want to build tools with OIIO is for testing
        # purposes. Libraries such as usdImagingGL might need to use tools like
        # idiff to compare the output images from their tests
        buildOIIOTools = 'ON' if (context.buildUsdImaging
                                  and context.buildTests) else 'OFF'
        extraArgs = ['-DOIIO_BUILD_TOOLS={}'.format(buildOIIOTools),
                     '-DOIIO_BUILD_TESTS=OFF',
                     '-DBUILD_DOCS=OFF',
                     '-DUSE_PYTHON=OFF',
                     '-DSTOP_ON_WARNING=OFF']

        # OIIO's FindOpenEXR module circumvents CMake's normal library 
        # search order, which causes versions of OpenEXR installed in
        # /usr/local or other hard-coded locations in the module to
        # take precedence over the version we've built, which would 
        # normally be picked up when we specify CMAKE_PREFIX_PATH. 
        # This may lead to undefined symbol errors at build or runtime. 
        # So, we explicitly specify the OpenEXR we want to use here.
        extraArgs.append('-DOPENEXR_ROOT="{instDir}"'
                         .format(instDir=context.instDir))

        # If Ptex support is disabled in USD, disable support in OpenImageIO
        # as well. This ensures OIIO doesn't accidentally pick up a Ptex
        # library outside of our build.
        if not context.enablePtex:
            extraArgs.append('-DUSE_PTEX=OFF')

        # Make sure to use boost installed by the build script and not any
        # system installed boost
        extraArgs.append('-DBoost_NO_BOOST_CMAKE=On')
        extraArgs.append('-DBoost_NO_SYSTEM_PATHS=True')

        # OpenImageIO 2.3.5 changed the default postfix for debug library
        # names from "" to "_d". USD's build system currently does not support
        # finding the library under this name, so as an interim workaround
        # we reset it back to its old value.
        extraArgs.append('-DCMAKE_DEBUG_POSTFIX=""')

        # Add on any user-specified extra arguments.
        extraArgs += buildArgs

        RunCMake(context, force, extraArgs)

OPENIMAGEIO = Dependency("OpenImageIO", InstallOpenImageIO,
                         "include/OpenImageIO/oiioversion.h")

############################################################
# OpenColorIO

OCIO_URL = "https://github.com/AcademySoftwareFoundation/OpenColorIO/archive/refs/tags/v2.1.3.zip"

def InstallOpenColorIO(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(OCIO_URL, context, force)):
        extraArgs = ['-DOCIO_BUILD_APPS=OFF',
                     '-DOCIO_BUILD_NUKE=OFF',
                     '-DOCIO_BUILD_DOCS=OFF',
                     '-DOCIO_BUILD_TESTS=OFF',
                     '-DOCIO_BUILD_GPU_TESTS=OFF',
                     '-DOCIO_BUILD_PYTHON=OFF']

        if MacOS():
            if apple_utils.IsTargetArm(context):
                extraArgs.append('-DOCIO_USE_SSE=OFF')

        # Add on any user-specified extra arguments.
        extraArgs += buildArgs

        RunCMake(context, force, extraArgs)

OPENCOLORIO = Dependency("OpenColorIO", InstallOpenColorIO,
                         "include/OpenColorIO/OpenColorABI.h")

############################################################
# OpenSubdiv

OPENSUBDIV_URL = "https://github.com/PixarAnimationStudios/OpenSubdiv/archive/v3_6_0.zip"

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
            '-DNO_GLEW=ON',
            '-DNO_GLFW=ON',
            '-DNO_PTEX=ON',
            '-DNO_TBB=ON',
        ]

        # Use Metal for macOS and all Apple embedded systems.
        if MacOS():
            extraArgs.append('-DNO_OPENGL=ON')

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
        # PySide6 is required for Apple Silicon support, so is the default
        # across all macOS hardware platforms.
        return ('PySide6 is not installed. If you have pip '
                'installed, run "pip install PySide6" '
                'to install it, then re-run this script.\n'
                'If PySide6 is already installed, you may need to '
                'update your PYTHONPATH to indicate where it is '
                'located.')
    else:
        return ('PySide2 or PySide6 are not installed. If you have pip '
                'installed, run "pip install PySide2" '
                'to install it, then re-run this script.\n'
                'If PySide2 is already installed, you may need to '
                'update your PYTHONPATH to indicate where it is '
                'located.')

PYSIDE = PythonDependency("PySide", GetPySideInstructions,
                          moduleNames=["PySide2", "PySide6"])

############################################################
# HDF5

HDF5_URL = "https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.0-patch1/src/hdf5-1.10.0-patch1.zip"

def InstallHDF5(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(HDF5_URL, context, force)):
        if MacOS():
            PatchFile("config/cmake_ext_mod/ConfigureChecks.cmake",
                    [("if (ARCH_LENGTH GREATER 1)", "if (FALSE)")])
            if context.targetUniversal:
                PatchFile("config/cmake/H5pubconf.h.in",
                        [(" #define H5_SIZEOF_LONG_LONG 8",
                        " #define H5_SIZEOF_LONG_LONG 8\n" +
                        " #define H5_SIZEOF_LONG_DOUBLE 16")])
        RunCMake(context, force,
                 ['-DBUILD_TESTING=OFF',
                  '-DHDF5_BUILD_TOOLS=OFF',
                  '-DHDF5_BUILD_EXAMPLES=OFF'] + buildArgs)
                 
HDF5 = Dependency("HDF5", InstallHDF5, "include/hdf5.h")

############################################################
# Alembic

ALEMBIC_URL = "https://github.com/alembic/alembic/archive/refs/tags/1.8.5.zip"

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
        else:
           cmakeOptions += ['-DUSE_HDF5=OFF']
                 
        cmakeOptions += buildArgs

        RunCMake(context, force, cmakeOptions)

ALEMBIC = Dependency("Alembic", InstallAlembic, "include/Alembic/Abc/Base.h")

############################################################
# Draco

DRACO_URL = "https://github.com/google/draco/archive/refs/tags/1.3.6.zip"

def InstallDraco(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(DRACO_URL, context, force)):
        cmakeOptions = [
            '-DBUILD_USD_PLUGIN=ON',
        ]
        cmakeOptions += buildArgs
        RunCMake(context, force, cmakeOptions)

DRACO = Dependency("Draco", InstallDraco, "include/draco/compression/decode.h")

############################################################
# MaterialX

MATERIALX_URL = "https://github.com/materialx/MaterialX/archive/v1.38.10.zip"

def InstallMaterialX(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(MATERIALX_URL, context, force)):
        cmakeOptions = ['-DMATERIALX_BUILD_SHARED_LIBS=ON',
                        '-DMATERIALX_BUILD_TESTS=OFF'
        ]

        if MacOSTargetEmbedded(context):
            # The materialXShaderGen in hdSt assumes the GLSL shadergen is
            # available but MaterialX intertwines GLSL shadergen support with
            # also requiring rendering support.
            cmakeOptions.extend([
                '-DMATERIALX_BUILD_GEN_MSL=ON',
                '-DMATERIALX_BUILD_GEN_GLSL=ON',
                '-DMATERIALX_BUILD_IOS=ON'])
            PatchFile("CMakeLists.txt",
                      [('    set(MATERIALX_BUILD_GEN_GLSL OFF)',
                        '    set(MATERIALX_BUILD_GEN_GLSL ON)'),
                       ('    if (MATERIALX_BUILD_GEN_GLSL)\n' +
                        '        add_subdirectory(source/MaterialXRenderGlsl)\n' +
                        '    endif()',
                        '    if (MATERIALX_BUILD_GEN_GLSL AND NOT MATERIALX_BUILD_IOS)\n' +
                        '        add_subdirectory(source/MaterialXRenderGlsl)\n' +
                        '    endif()')
                       ], multiLineMatches=True)

        cmakeOptions += buildArgs
        RunCMake(context, force, cmakeOptions)

MATERIALX = Dependency("MaterialX", InstallMaterialX, "include/MaterialXCore/Library.h")

############################################################
# Embree

EMBREE_DEFAULT_MAJOR_VERSION = 3

def InstallEmbree(context, force, buildArgs):
    if context.embreeMajorVersion >= 4:
        EMBREE_URL = "https://github.com/RenderKit/embree/archive/refs/tags/v4.3.3.zip"
    elif MacOS():
        # For MacOS we use version 3.13.3 to include a fix from Intel
        # to build on Apple Silicon.
        EMBREE_URL = "https://github.com/RenderKit/embree/archive/refs/tags/v3.13.3.zip"
    else:
        EMBREE_URL = "https://github.com/RenderKit/embree/archive/refs/tags/v3.2.2.zip"

    with CurrentWorkingDirectory(DownloadURL(EMBREE_URL, context, force)):
        extraArgs = [
            '-DTBB_ROOT="{instDir}"'.format(instDir=context.instDir),
            '-DEMBREE_TUTORIALS=OFF',
            '-DEMBREE_ISPC_SUPPORT=OFF'
        ]

        if MacOS() and context.targetUniversal:
            extraArgs += [
                '-DEMBREE_MAX_ISA=NEON',
                '-DEMBREE_ISA_NEON=ON']

        extraArgs += buildArgs

        RunCMake(context, force, extraArgs)

EMBREE = Dependency("Embree", InstallEmbree,
                    "include/embree3/rtcore.h",
                    "include/embree4/rtcore.h")

############################################################
# AnimX

# This GitHub project has no releases, so we fixed on the latest commit as of
# 2024-02-06 - 5db8ee4, which was committed on 2018-11-05
ANIMX_URL = "https://github.com/Autodesk/animx/archive/5db8ee416d5fa7050357f498d4dcfaa6ff3f7738.zip"

def InstallAnimX(context, force, buildArgs):
    with CurrentWorkingDirectory(DownloadURL(ANIMX_URL, context, force)):
        # AnimX strangely installs its output to the inst root, rather than the
        # lib subdirectory.  Fix.
        PatchFile("src/CMakeLists.txt",
                  [("LIBRARY DESTINATION .", "LIBRARY DESTINATION lib")])

        extraArgs = [
            '-DANIMX_BUILD_MAYA_TESTSUITE=OFF',
            '-DMAYA_64BIT_TIME_PRECISION=ON',
            '-DANIMX_BUILD_SHARED=ON',
            '-DANIMX_BUILD_STATIC=OFF'
        ]
        RunCMake(context, force, extraArgs)

ANIMX = Dependency("AnimX", InstallAnimX, "include/animx.h")


############################################################
# USD

def InstallUSD(context, force, buildArgs):
    with CurrentWorkingDirectory(context.usdSrcDir):
        extraArgs = []

        extraArgs.append('-DPXR_PREFER_SAFETY_OVER_SPEED={}'
                         .format('ON' if context.safetyFirst else 'OFF'))

        if context.buildPython:
            extraArgs.append('-DPXR_ENABLE_PYTHON_SUPPORT=ON')

            # Many people on Windows may not have Python libraries with debug
            # symbols (denoted by a '_d') installed. This is the common
            # case when a user installs Python from the official download
            # website. Therefore we can still let people decide to build USD
            # with the release version of Python if debugging into Python land
            # is not what they want which can be done by setting the debugPython
            # argument.
            if context.buildDebug and context.debugPython:
                extraArgs.append('-DPXR_USE_DEBUG_PYTHON=ON')
            else:
                extraArgs.append('-DPXR_USE_DEBUG_PYTHON=OFF')

            if context.buildBoostPython:
                extraArgs.append('-DPXR_USE_BOOST_PYTHON=ON')
            else:
                extraArgs.append('-DPXR_USE_BOOST_PYTHON=OFF')

            # CMake has trouble finding the executable, library, and include
            # directories when there are multiple versions of Python installed.
            # This can lead to crashes due to USD being linked against one
            # version of Python but running through some other Python
            # interpreter version. This primarily shows up on macOS, as it's
            # common to have a Python install that's separate from the one
            # included with the system.
            #
            # To avoid this, we try to determine these paths from Python
            # itself rather than rely on CMake's heuristics.
            pythonInfo = GetPythonInfo(context)
            if pythonInfo:
                extraArgs.append('-DPython3_EXECUTABLE="{pyExecPath}"'
                                 .format(pyExecPath=pythonInfo[0]))
                extraArgs.append('-DPython3_LIBRARY="{pyLibPath}"'
                                 .format(pyLibPath=pythonInfo[1]))
                extraArgs.append('-DPython3_INCLUDE_DIR="{pyIncPath}"'
                                 .format(pyIncPath=pythonInfo[2]))
        else:
            extraArgs.append('-DPXR_ENABLE_PYTHON_SUPPORT=OFF')

        if context.buildShared:
            extraArgs.append('-DBUILD_SHARED_LIBS=ON')
        elif context.buildMonolithic:
            extraArgs.append('-DPXR_BUILD_MONOLITHIC=ON')

        if context.buildDebug:
            extraArgs.append('-DTBB_USE_DEBUG_BUILD=ON')
        else:
            extraArgs.append('-DTBB_USE_DEBUG_BUILD=OFF')

        if context.buildDocs:
            extraArgs.append('-DPXR_BUILD_DOCUMENTATION=ON')
        else:
            extraArgs.append('-DPXR_BUILD_DOCUMENTATION=OFF')

        if context.buildHtmlDocs:
            extraArgs.append('-DPXR_BUILD_HTML_DOCUMENTATION=ON')
        else:
            extraArgs.append('-DPXR_BUILD_HTML_DOCUMENTATION=OFF')

        if context.buildPythonDocs:
            extraArgs.append('-DPXR_BUILD_PYTHON_DOCUMENTATION=ON')
        else:
            extraArgs.append('-DPXR_BUILD_PYTHON_DOCUMENTATION=OFF')

        if context.buildTests:
            extraArgs.append('-DPXR_BUILD_TESTS=ON')
        else:
            extraArgs.append('-DPXR_BUILD_TESTS=OFF')

        if context.buildExamples:
            extraArgs.append('-DPXR_BUILD_EXAMPLES=ON')
        else:
            extraArgs.append('-DPXR_BUILD_EXAMPLES=OFF')

        if context.buildTutorials:
            extraArgs.append('-DPXR_BUILD_TUTORIALS=ON')
        else:
            extraArgs.append('-DPXR_BUILD_TUTORIALS=OFF')

        if context.buildTools:
            extraArgs.append('-DPXR_BUILD_USD_TOOLS=ON')
        else:
            extraArgs.append('-DPXR_BUILD_USD_TOOLS=OFF')
            
        if context.buildImaging:
            extraArgs.append('-DPXR_BUILD_IMAGING=ON')
            if context.enablePtex:
                extraArgs.append('-DPXR_ENABLE_PTEX_SUPPORT=ON')
            else:
                extraArgs.append('-DPXR_ENABLE_PTEX_SUPPORT=OFF')

            if context.enableOpenVDB:
                extraArgs.append('-DPXR_ENABLE_OPENVDB_SUPPORT=ON')
            else:
                extraArgs.append('-DPXR_ENABLE_OPENVDB_SUPPORT=OFF')

            if context.buildEmbree:
                extraArgs.append('-DPXR_BUILD_EMBREE_PLUGIN=ON')
            else:
                extraArgs.append('-DPXR_BUILD_EMBREE_PLUGIN=OFF')

            if context.buildPrman:
                if context.prmanLocation:
                    extraArgs.append('-DRENDERMAN_LOCATION="{location}"'
                                     .format(location=context.prmanLocation))
                extraArgs.append('-DPXR_BUILD_PRMAN_PLUGIN=ON')
            else:
                extraArgs.append('-DPXR_BUILD_PRMAN_PLUGIN=OFF')                
            
            if context.buildOIIO:
                extraArgs.append('-DPXR_BUILD_OPENIMAGEIO_PLUGIN=ON')
            else:
                extraArgs.append('-DPXR_BUILD_OPENIMAGEIO_PLUGIN=OFF')
                
            if context.buildOCIO:
                extraArgs.append('-DPXR_BUILD_OPENCOLORIO_PLUGIN=ON')
            else:
                extraArgs.append('-DPXR_BUILD_OPENCOLORIO_PLUGIN=OFF')

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

        if context.buildDraco:
            extraArgs.append('-DPXR_BUILD_DRACO_PLUGIN=ON')
            draco_root = (context.dracoLocation
                          if context.dracoLocation else context.instDir)
            extraArgs.append('-DDRACO_ROOT="{}"'.format(draco_root))
        else:
            extraArgs.append('-DPXR_BUILD_DRACO_PLUGIN=OFF')

        if context.buildMaterialX:
            extraArgs.append('-DPXR_ENABLE_MATERIALX_SUPPORT=ON')
        else:
            extraArgs.append('-DPXR_ENABLE_MATERIALX_SUPPORT=OFF')

        if context.buildMayapyTests:
            extraArgs.append('-DPXR_BUILD_MAYAPY_TESTS=ON')
            extraArgs.append('-DMAYAPY_LOCATION="{mayapyLocation}"'
                             .format(mayapyLocation=context.mayapyLocation))
        else:
            extraArgs.append('-DPXR_BUILD_MAYAPY_TESTS=OFF')

        if context.buildAnimXTests:
            extraArgs.append('-DPXR_BUILD_ANIMX_TESTS=ON')
        else:
            extraArgs.append('-DPXR_BUILD_ANIMX_TESTS=OFF')

        if Windows():
            # Increase the precompiled header buffer limit.
            extraArgs.append('-DCMAKE_CXX_FLAGS="/Zm150"')

        # Make sure to use boost installed by the build script and not any
        # system installed boost
        extraArgs.append('-DBoost_NO_BOOST_CMAKE=On')
        extraArgs.append('-DBoost_NO_SYSTEM_PATHS=True')
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

- Embedded Build Targets
When cross compiling for an embedded target operating system, e.g. iOS, the
following components are disabled: python, tools, tests, examples, tutorials,
opencolorio, openimageio, openvdb.

- Python Versions and DCC Plugins:
Some DCCs may ship with and run using their own version of Python. In that case,
it is important that USD and the plugins for that DCC are built using the DCC's
version of Python and not the system version. This can be done by running
%(prog)s using the DCC's version of Python.

If %(prog)s does not automatically detect the necessary information, the flag
--build-python-info can be used to explicitly pass in the Python that you want
USD to use to build the Python bindings with. This flag takes 4 arguments: 
Python executable, Python include directory Python library and Python version.

Note that this is primarily an issue on MacOS, where a DCC's version of Python
is likely to conflict with the version provided by the system.

- C++11 ABI Compatibility:
On Linux, the --use-cxx11-abi parameter can be used to specify whether to use
the C++11 ABI for libstdc++ when building USD and any dependencies. The value
given to this parameter will be used to define _GLIBCXX_USE_CXX11_ABI for
all builds.

If this parameter is not specified, the compiler's default ABI will be used.

For more details see:
https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_dual_abi.html
""".format(
    libraryList=" ".join(sorted([d.name for d in AllDependencies])))

parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
    allow_abbrev=False, description=programDescription)

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

BUILD_DEBUG = "debug"
BUILD_RELEASE = "release"
BUILD_RELWITHDEBUG = "relwithdebuginfo"
group.add_argument("--build-variant", default=BUILD_RELEASE,
                   choices=[BUILD_DEBUG, BUILD_RELEASE, BUILD_RELWITHDEBUG],
                   help=("Build variant for USD and 3rd-party dependencies. "
                         "(default: {})".format(BUILD_RELEASE)))

group.add_argument("--ignore-paths", type=str, nargs="*", default=[],
                   help="Paths for CMake to ignore when configuring projects.")
if MacOS():
    group.add_argument("--build-target",
                       default=apple_utils.GetBuildTargetDefault(),
                       choices=apple_utils.GetBuildTargets(),
                       help=("Build target for macOS cross compilation. "
                             "(default: {})".format(
                                apple_utils.GetBuildTargetDefault())))
    if apple_utils.IsHostArm():
        # Intel Homebrew stores packages in /usr/local which unfortunately can
        # be where a lot of other things are too. So we only add this flag on arm macs.
        group.add_argument("--ignore-homebrew", action="store_true",
                           help="Specify that CMake should ignore Homebrew packages.")

group.add_argument("--build-args", type=str, nargs="*", default=[],
                   help=("Custom arguments to pass to build system when "
                         "building libraries (see docs above)"))
group.add_argument("--cmake-build-args", type=str,
                   help=("Custom arguments to pass to all builds that use "
                         "cmake; a single string, similar to the args passed "
                         "for a single dependency in --build-args"))
group.add_argument("--build-python-info", type=str, nargs=4, default=[],
                   metavar=('PYTHON_EXECUTABLE', 'PYTHON_INCLUDE_DIR', 
                            'PYTHON_LIBRARY', 'PYTHON_VERSION'),
                   help=("Specify a custom python to use during build"))
group.add_argument("--force", type=str, action="append", dest="force_build",
                   default=[],
                   help=("Force download and build of specified library "
                         "(see docs above)"))
group.add_argument("--force-all", action="store_true",
                   help="Force download and build of all libraries")
group.add_argument("--generator", type=str,
                   help=("CMake generator to use when building libraries with "
                         "cmake"))
group.add_argument("--toolset", type=str,
                   help=("CMake toolset to use when building libraries with "
                         "cmake"))
if MacOS():
    codesignDefault = True if apple_utils.IsHostArm() else False
    group.add_argument("--codesign", dest="macos_codesign",
                       default=codesignDefault, action="store_true",
                       help=("Enable code signing for macOS builds "
                             "(defaults to enabled on Apple Silicon)"))

if Linux():
    group.add_argument("--use-cxx11-abi", type=int, choices=[0, 1],
                       help=("Use C++11 ABI for libstdc++. (see docs above)"))

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
subgroup.add_argument("--examples", dest="build_examples", action="store_true",
                      default=True, help="Build examples (default)")
subgroup.add_argument("--no-examples", dest="build_examples", action="store_false",
                      help="Do not build examples")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--tutorials", dest="build_tutorials", action="store_true",
                      default=True, help="Build tutorials (default)")
subgroup.add_argument("--no-tutorials", dest="build_tutorials", action="store_false",
                      help="Do not build tutorials")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--tools", dest="build_tools", action="store_true",
                     default=True, help="Build USD tools (default)")
subgroup.add_argument("--no-tools", dest="build_tools", action="store_false",
                      help="Do not build USD tools")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--docs", dest="build_docs", action="store_true",
                      default=False, help="Build C++ documentation")
subgroup.add_argument("--no-docs", dest="build_docs", action="store_false",
                      help="Do not build C++ documentation (default)")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--python-docs", dest="build_python_docs", action="store_true",
                      default=False, help="Build Python docs")
subgroup.add_argument("--no-python-docs", dest="build_python_docs", action="store_false",
                      help="Do not build Python documentation (default)")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--python", dest="build_python", action="store_true",
                      default=True, help="Build python based components "
                                         "(default)")
subgroup.add_argument("--no-python", dest="build_python", action="store_false",
                      help="Do not build python based components")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--prefer-safety-over-speed", dest="safety_first",
                      action="store_true", default=True, help=
                      "Enable extra safety checks (which may negatively "
                      "impact performance) against malformed input files "
                      "(default)")
subgroup.add_argument("--prefer-speed-over-safety", dest="safety_first",
                      action="store_false", help=
                      "Disable performance-impacting safety checks against "
                      "malformed input files")

group.add_argument("--boost-python", dest="build_boost_python",
                   action="store_true", default=False,
                   help="Build Python bindings with boost::python (deprecated)")

subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--debug-python", dest="debug_python", 
                      action="store_true", help=
                      "Define Boost Python Debug if your Python library "
                      "comes with Debugging symbols.")
subgroup.add_argument("--no-debug-python", dest="debug_python",
                      action="store_false", help=
                      "Don't define Boost Python Debug if your Python "
                      "library comes with Debugging symbols.")

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
subgroup.add_argument("--openvdb", dest="enable_openvdb", action="store_true", 
                      default=False, 
                      help="Enable OpenVDB support in imaging")
subgroup.add_argument("--no-openvdb", dest="enable_openvdb", 
                      action="store_false",
                      help="Disable OpenVDB support in imaging (default)")
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
group.add_argument("--embree-major-version",
                   default=EMBREE_DEFAULT_MAJOR_VERSION, type=int,
                   choices=[3, 4],
                   help=(
                       "The major version of Embree to build "
                       "(default: {})").format(EMBREE_DEFAULT_MAJOR_VERSION))
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--prman", dest="build_prman", action="store_true",
                      default=False,
                      help="Build Pixar's RenderMan imaging plugin")
subgroup.add_argument("--no-prman", dest="build_prman", action="store_false",
                      help="Do not build Pixar's RenderMan imaging plugin (default)")
group.add_argument("--prman-location", type=str,
                   help="Directory where Pixar's RenderMan is installed.")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--openimageio", dest="build_oiio", action="store_true", 
                      default=False,
                      help="Build OpenImageIO plugin for USD")
subgroup.add_argument("--no-openimageio", dest="build_oiio", action="store_false",
                      help="Do not build OpenImageIO plugin for USD (default)")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--opencolorio", dest="build_ocio", action="store_true", 
                      default=False,
                      help="Build OpenColorIO plugin for USD")
subgroup.add_argument("--no-opencolorio", dest="build_ocio", action="store_false",
                      help="Do not build OpenColorIO plugin for USD (default)")

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

group = parser.add_argument_group(title="Draco Plugin Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--draco", dest="build_draco", action="store_true", 
                      default=False,
                      help="Build Draco plugin for USD")
subgroup.add_argument("--no-draco", dest="build_draco", action="store_false",
                      help="Do not build Draco plugin for USD (default)")
group.add_argument("--draco-location", type=str,
                   help="Directory where Draco is installed.")

group = parser.add_argument_group(title="MaterialX Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--materialx", dest="build_materialx", action="store_true", 
                      default=True,
                      help="Enable MaterialX support (default)")
subgroup.add_argument("--no-materialx", dest="build_materialx", action="store_false",
                      help="Disable MaterialX support")

group = parser.add_argument_group(title="TBB Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--onetbb", dest="build_onetbb", action="store_true",
                      default=False,
                      help="Build using oneTBB instead of TBB")
subgroup.add_argument("--no-onetbb", dest="build_onetbb", action="store_false",
                      help="Build using TBB (default)")

group = parser.add_argument_group(title="Spline Test Options")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--mayapy-tests",
                      dest="build_mayapy_tests", action="store_true",
                      default=False,
                      help="Build mayapy spline tests")
subgroup.add_argument("--no-mayapy-tests",
                      dest="build_mayapy_tests", action="store_false",
                      help="Do not build mayapy spline tests (default)")
group.add_argument("--mayapy-location", type=str,
                   help="Directory where mayapy is installed")
subgroup = group.add_mutually_exclusive_group()
subgroup.add_argument("--animx-tests",
                      dest="build_animx_tests", action="store_true",
                      default=False,
                      help="Build AnimX spline tests")
subgroup.add_argument("--no-animx-tests",
                      dest="build_animx_tests", action="store_false",
                      help="Do not build AnimX spline tests (default)")

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
        if which("curl"):
            self.downloader = DownloadFileWithCurl
            self.downloaderName = "curl"
        elif Windows() and which("powershell"):
            self.downloader = DownloadFileWithPowershell
            self.downloaderName = "powershell"
        else:
            self.downloader = DownloadFileWithUrllib
            self.downloaderName = "built-in"

        # CMake generator and toolset
        self.cmakeGenerator = args.generator
        self.cmakeToolset = args.toolset
        self.cmakeBuildArgs = args.cmake_build_args

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

        # Build python info
        self.build_python_info = dict()
        if args.build_python_info:
            self.build_python_info['PYTHON_EXECUTABLE'] = args.build_python_info[0]
            self.build_python_info['PYTHON_INCLUDE_DIR'] = args.build_python_info[1]
            self.build_python_info['PYTHON_LIBRARY'] = args.build_python_info[2]
            self.build_python_info['PYTHON_VERSION'] = args.build_python_info[3]

        # Build type
        self.buildDebug = (args.build_variant == BUILD_DEBUG)
        self.buildRelease = (args.build_variant == BUILD_RELEASE)
        self.buildRelWithDebug = (args.build_variant == BUILD_RELWITHDEBUG)

        self.debugPython = args.debug_python

        self.buildShared = (args.build_type == SHARED_LIBS)
        self.buildMonolithic = (args.build_type == MONOLITHIC_LIB)

        self.ignorePaths = args.ignore_paths or []
        # Build target and code signing
        if MacOS():
            self.buildTarget = args.build_target
            apple_utils.SetTarget(self, self.buildTarget)

            self.macOSCodesign = \
                (args.macos_codesign if hasattr(args, "macos_codesign")
                 else False)
            if apple_utils.IsHostArm() and args.ignore_homebrew:
                self.ignorePaths.append("/opt/homebrew")
        else:
            self.buildTarget = ""

        self.useCXX11ABI = \
            (args.use_cxx11_abi if hasattr(args, "use_cxx11_abi") else None)
        self.safetyFirst = args.safety_first

        # Dependencies that are forced to be built
        self.forceBuildAll = args.force_all
        self.forceBuild = [dep.lower() for dep in args.force_build]

        # Some components are disabled for embedded build targets
        embedded = MacOSTargetEmbedded(self)

        # Optional components
        self.buildTests = args.build_tests and not embedded
        self.buildPython = args.build_python and not embedded
        self.buildBoostPython = self.buildPython and args.build_boost_python
        self.buildExamples = args.build_examples and not embedded
        self.buildTutorials = args.build_tutorials and not embedded
        self.buildTools = args.build_tools and not embedded

        # - Documentation
        self.buildDocs = args.build_docs or args.build_python_docs
        self.buildHtmlDocs = args.build_docs
        self.buildPythonDocs = args.build_python_docs

        # - Imaging
        self.buildImaging = (args.build_imaging == IMAGING or
                             args.build_imaging == USD_IMAGING)
        self.enablePtex = self.buildImaging and args.enable_ptex
        self.enableOpenVDB = (self.buildImaging
                              and args.enable_openvdb
                              and not embedded)

        # - USD Imaging
        self.buildUsdImaging = (args.build_imaging == USD_IMAGING)

        # - usdview
        self.buildUsdview = (self.buildUsdImaging and 
                             self.buildPython and 
                             args.build_usdview)

        # - Imaging plugins
        self.buildEmbree = self.buildImaging and args.build_embree
        self.embreeMajorVersion = args.embree_major_version
        self.buildPrman = self.buildImaging and args.build_prman
        self.prmanLocation = (os.path.abspath(args.prman_location)
                               if args.prman_location else None)                               
        self.buildOIIO = ((args.build_oiio or (self.buildUsdImaging
                                               and self.buildTests))
                          and not embedded)
        self.buildOCIO = args.build_ocio and not embedded

        # - Alembic Plugin
        self.buildAlembic = args.build_alembic
        self.enableHDF5 = self.buildAlembic and args.enable_hdf5

        # - Draco Plugin
        self.buildDraco = args.build_draco
        self.dracoLocation = (os.path.abspath(args.draco_location)
                                if args.draco_location else None)

        # - MaterialX
        self.buildMaterialX = args.build_materialx

        # - TBB
        self.buildOneTBB = args.build_onetbb

        # - Spline Tests
        self.buildMayapyTests = args.build_mayapy_tests
        self.mayapyLocation = args.mayapy_location
        self.buildAnimXTests = args.build_animx_tests

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
if context.buildOneTBB:
    TBB = ONETBB

requiredDependencies = [ZLIB, TBB]

if context.buildBoostPython:
    requiredDependencies += [BOOST]

if context.buildAlembic:
    if context.enableHDF5:
        requiredDependencies += [HDF5]
    requiredDependencies += [OPENEXR, ALEMBIC]

if context.buildDraco:
    requiredDependencies += [DRACO]

if context.buildMaterialX:
    requiredDependencies += [MATERIALX]

if context.buildImaging:
    if context.enablePtex:
        requiredDependencies += [PTEX]

    requiredDependencies += [OPENSUBDIV]

    if context.enableOpenVDB:
        requiredDependencies += [BLOSC, BOOST, OPENEXR, OPENVDB, TBB]
    
    if context.buildOIIO:
        requiredDependencies += [BOOST, JPEG, TIFF, PNG, OPENEXR, OPENIMAGEIO]

    if context.buildOCIO:
        requiredDependencies += [OPENCOLORIO]

    if context.buildEmbree:
        requiredDependencies += [TBB, EMBREE]
                             
if context.buildUsdview:
    requiredDependencies += [PYOPENGL, PYSIDE]

if context.buildAnimXTests:
    requiredDependencies += [ANIMX]

# Assume zlib already exists on Linux platforms and don't build
# our own. This avoids potential issues where a host application
# loads an older version of zlib than the one we'd build and link
# our libraries against.
if Linux():
    requiredDependencies.remove(ZLIB)

# Error out if user is building monolithic library on windows with draco plugin
# enabled. This currently results in missing symbols.
if context.buildDraco and context.buildMonolithic and Windows():
    PrintError("Draco plugin can not be enabled for monolithic build on Windows")
    sys.exit(1)

# When building with both oneTBB and Embree, a 4.x version of Embree must be
# used.
if context.buildOneTBB and (context.buildEmbree and context.embreeMajorVersion < 4):
    PrintError("Embree 4.x or later must be selected when building against oneTBB")
    sys.exit(1)

# Error out if user explicitly enabled components which aren't
# supported for embedded build targets.
if MacOSTargetEmbedded(context):
    if "--tests" in sys.argv:
        PrintError("Cannot build tests for embedded build targets")
        sys.exit(1)
    if "--python" in sys.argv:
        PrintError("Cannot build python components for embedded build targets")
        sys.exit(1)
    if "--examples" in sys.argv:
        PrintError("Cannot build examples for embedded build targets")
        sys.exit(1)
    if "--tutorials" in sys.argv:
        PrintError("Cannot build tutorials for embedded build targets")
        sys.exit(1)
    if "--tools" in sys.argv:
        PrintError("Cannot build tools for embedded build targets")
        sys.exit(1)
    if "--openimageio" in sys.argv:
        PrintError("Cannot build openimageio for embedded build targets")
        sys.exit(1)
    if "--opencolorio" in sys.argv:
        PrintError("Cannot build opencolorio for embedded build targets")
        sys.exit(1)
    if "--openvdb" in sys.argv:
        PrintError("Cannot build openvdb for embedded build targets")
        sys.exit(1)

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

dependenciesToBuild = []
for dep in requiredDependencies:
    if context.ForceBuildDependency(dep) or not dep.Exists(context):
        if dep not in dependenciesToBuild:
            dependenciesToBuild.append(dep)

# Verify toolchain needed to build required dependencies
if (not which("g++") and
    not which("clang") and
    not GetXcodeDeveloperDirectory() and
    not GetVisualStudioCompilerAndVersion()):
    PrintError("C++ compiler not found -- please install a compiler")
    sys.exit(1)

# Error out if a 64bit version of python interpreter is not being used
isPython64Bit = (ctypes.sizeof(ctypes.c_voidp) == 8)
if not isPython64Bit:
    PrintError("64bit python not found -- please install it and adjust your"
               "PATH")
    sys.exit(1)

if which("cmake"):
    # Check cmake minimum version requirements
    pyInfo = GetPythonInfo(context)
    pyVer = (int(pyInfo[3].split('.')[0]), int(pyInfo[3].split('.')[1]))
    if context.buildPython and pyVer >= (3, 11):
        # Python 3.11 requires boost 1.82.0, which is not supported prior
        # to 3.27
        cmake_required_version = (3, 27)
    elif context.buildPython and pyVer >= (3, 10):
        # Python 3.10 is not supported prior to 3.24
        cmake_required_version = (3, 24)
    elif IsVisualStudio2022OrGreater():
        # Visual Studio 2022 is not supported prior to 3.24
        cmake_required_version = (3, 24)
    elif Windows():
        # Visual Studio 2017 and 2019 are verified to work correctly with 3.14
        cmake_required_version = (3, 14)
    elif MacOS():
        # Apple Silicon is not supported prior to 3.19
        cmake_required_version = (3, 19)

        # visionOS support was added in CMake 3.28
        if context.buildTarget == apple_utils.TARGET_VISIONOS:
            cmake_required_version = (3, 28)
    else:
        # Linux, and vfx platform CY2020, are verified to work correctly with 3.14
        cmake_required_version = (3, 14)

    cmake_version = GetCMakeVersion()
    if not cmake_version:
        PrintError("Failed to determine CMake version")
        sys.exit(1)

    if cmake_version < cmake_required_version:
        def _JoinVersion(v):
            return ".".join(str(n) for n in v)
        PrintError("CMake version {req} or later required to build USD, "
                   "but version found was {found}".format(
                       req=_JoinVersion(cmake_required_version),
                       found=_JoinVersion(cmake_version)))
        sys.exit(1)
else:
    PrintError("CMake not found -- please install it and adjust your PATH")
    sys.exit(1)

if context.buildDocs:
    if not which("doxygen"):
        PrintError("doxygen not found -- please install it and adjust your PATH")
        sys.exit(1)

if context.buildHtmlDocs:
    if not which("dot"):
        PrintError("dot not found -- please install graphviz and adjust your "
                   "PATH")
        sys.exit(1)

# Require having built both Python support and Doxygen docs before we can 
# build Python docs
if context.buildPythonDocs:
    if not context.buildDocs:
        PrintError("Cannot build Python docs when doxygen docs are disabled.")
        sys.exit(1)
    if not context.buildPython:
        PrintError("Cannot build Python docs when Python support is disabled.")
        sys.exit(1)        

if PYSIDE in requiredDependencies:
    # Special case - we are given the PYSIDEUICBINARY as cmake arg.
    usdBuildArgs = context.GetBuildArguments(USD)
    given_pysideUic = 'PYSIDEUICBINARY' in " ".join(usdBuildArgs)

    # The USD build will skip building usdview if pyside6-uic or pyside2-uic is
    # not found, so check for it here to avoid confusing users. This list of 
    # PySide executable names comes from cmake/modules/FindPySide.cmake
    pyside6Uic = ["pyside6-uic"]
    found_pyside6Uic = any([which(p) for p in pyside6Uic])
    pyside2Uic = ["pyside2-uic"]
    found_pyside2Uic = any([which(p) for p in pyside2Uic])
    if not given_pysideUic and not found_pyside2Uic and not found_pyside6Uic:
        PrintError("PySide's user interface compiler was not found -- please"
                   " install PySide2 or PySide6 and adjust your PATH. (Note"
                   " that this program may be named {0} depending on your"
                   " platform)"
                   .format(" or ".join(set(pyside2Uic+pyside6Uic))))
        sys.exit(1)

if context.buildMayapyTests:
    if not context.buildPython:
        PrintError("--mayapy-tests requires --python")
        sys.exit(1)
    if not context.buildTests:
        PrintError("--mayapy-tests requires --tests")
        sys.exit(1)
    if not context.mayapyLocation:
        PrintError("--mayapy-tests requires --mayapy-location")
        sys.exit(1)

if context.buildAnimXTests:
    if not context.buildTests:
        PrintError("--animx-tests requires --tests")
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
  CMake toolset                 {cmakeToolset}
  Downloader                    {downloader}

  Building                      {buildType}
""" 

if context.useCXX11ABI is not None:
    summaryMsg += """\
    Use C++11 ABI               {useCXX11ABI}
"""

summaryMsg += """\
    Variant                     {buildVariant}
    Target                      {buildTarget}
    Imaging                     {buildImaging}
      Ptex support:             {enablePtex}
      OpenVDB support:          {enableOpenVDB}
      OpenImageIO support:      {buildOIIO} 
      OpenColorIO support:      {buildOCIO} 
      Embree support:           {buildEmbree}
        Embree major version:   {embreeMajorVersion}
      PRMan support:            {buildPrman}
    UsdImaging                  {buildUsdImaging}
      usdview:                  {buildUsdview}
    MaterialX support           {buildMaterialX}
    Python support              {buildPython}
      Python Debug:             {debugPython}
      Python docs:              {buildPythonDocs}
    Documentation               {buildHtmlDocs}
    Tests                       {buildTests}
      Mayapy Tests:             {buildMayapyTests}
      AnimX Tests:              {buildAnimXTests}
    Examples                    {buildExamples}
    Tutorials                   {buildTutorials}
    Tools                       {buildTools}
    Alembic Plugin              {buildAlembic}
      HDF5 support:             {enableHDF5}
    Draco Plugin                {buildDraco}

  Dependencies                  {dependencies}"""

if context.buildArgs:
    summaryMsg += """
  Build arguments               {buildArgs}"""

def FormatBuildArguments(buildArgs):
    s = ""
    for depName in sorted(buildArgs.keys()):
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
    cmakeToolset=("Default" if not context.cmakeToolset
                  else context.cmakeToolset),
    downloader=(context.downloaderName),
    dependencies=("None" if not dependenciesToBuild else 
                  ", ".join([d.name for d in dependenciesToBuild])),
    buildArgs=FormatBuildArguments(context.buildArgs),
    useCXX11ABI=("On" if context.useCXX11ABI else "Off"),
    buildType=("Shared libraries" if context.buildShared
               else "Monolithic shared library" if context.buildMonolithic
               else ""),
    buildVariant=("Release" if context.buildRelease
                  else "Debug" if context.buildDebug
                  else "Release w/ Debug Info" if context.buildRelWithDebug
                  else ""),
    buildTarget=(apple_utils.GetTargetName(context) if context.buildTarget
                 else ""),
    buildImaging=("On" if context.buildImaging else "Off"),
    enablePtex=("On" if context.enablePtex else "Off"),
    enableOpenVDB=("On" if context.enableOpenVDB else "Off"),
    buildOIIO=("On" if context.buildOIIO else "Off"),
    buildOCIO=("On" if context.buildOCIO else "Off"),
    buildEmbree=("On" if context.buildEmbree else "Off"),
    embreeMajorVersion=(context.embreeMajorVersion if context.buildEmbree
                        else "N/A"),
    buildPrman=("On" if context.buildPrman else "Off"),
    buildUsdImaging=("On" if context.buildUsdImaging else "Off"),
    buildUsdview=("On" if context.buildUsdview else "Off"),
    buildPython=("On" if context.buildPython else "Off"),
    debugPython=("On" if context.debugPython else "Off"),
    buildPythonDocs=("On" if context.buildPythonDocs else "Off"),
    buildHtmlDocs=("On" if context.buildHtmlDocs else "Off"),
    buildTests=("On" if context.buildTests else "Off"),
    buildExamples=("On" if context.buildExamples else "Off"),
    buildTutorials=("On" if context.buildTutorials else "Off"),
    buildTools=("On" if context.buildTools else "Off"),
    buildAlembic=("On" if context.buildAlembic else "Off"),
    buildDraco=("On" if context.buildDraco else "Off"),
    buildMaterialX=("On" if context.buildMaterialX else "Off"),
    buildMayapyTests=("On" if context.buildMayapyTests else "Off"),
    buildAnimXTests=("On" if context.buildAnimXTests else "Off"),
    enableHDF5=("On" if context.enableHDF5 else "Off"))

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

if MacOS():
    if context.macOSCodesign:
        apple_utils.Codesign(context.usdInstDir, verbosity > 1)

additionalInstructions = any([context.buildPython, context.buildTools, context.buildPrman])
if additionalInstructions:
    Print("\nSuccess! To use USD, please ensure that you have:")
else:
    Print("\nSuccess! USD libraries were built.")

if context.buildPython:
    Print("""
    The following in your PYTHONPATH environment variable:
    {requiredInPythonPath}""".format(
        requiredInPythonPath="\n    ".join(sorted(requiredInPythonPath))))

if context.buildPython or context.buildTools:
    Print("""
    The following in your PATH environment variable:
    {requiredInPath}""".format(
        requiredInPath="\n    ".join(sorted(requiredInPath))))
    
if context.buildPrman:
    Print("See documentation at http://openusd.org/docs/RenderMan-USD-Imaging-Plugin.html "
          "for setting up the RenderMan plugin.\n")
