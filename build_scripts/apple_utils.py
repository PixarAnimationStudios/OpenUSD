#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Utilities for managing Apple OS build concerns.
#
# NOTE: This file and its contents may change significantly as we continue
# working to make the build scripts more modular. We anticipate providing
# a clearer and more extensible way of expressing platform specific concerns
# as we add support for additional platforms.

import sys
import locale
import os
import platform
import shlex
import subprocess
from typing import Optional, List

TARGET_NATIVE = "native"
TARGET_X86 = "x86_64"
TARGET_ARM64 = "arm64"
TARGET_UNIVERSAL = "universal"
TARGET_IOS = "iOS"
TARGET_VISIONOS = "visionOS"

EMBEDDED_PLATFORMS = [TARGET_IOS, TARGET_VISIONOS]

def GetBuildTargets():
    return [TARGET_NATIVE,
            TARGET_X86,
            TARGET_ARM64,
            TARGET_UNIVERSAL,
            TARGET_IOS,
            TARGET_VISIONOS]

def GetBuildTargetDefault():
    return TARGET_NATIVE

def MacOS():
    return platform.system() == "Darwin"

def TargetEmbeddedOS(context):
    return context.buildTarget in EMBEDDED_PLATFORMS

def GetLocale():
    return sys.stdout.encoding or locale.getdefaultlocale()[1] or "UTF-8"

def GetCommandOutput(command, **kwargs):
    """Executes the specified command and returns output or None."""
    try:
        return subprocess.check_output(
            command, stderr=subprocess.STDOUT, **kwargs).decode(
                                        GetLocale(), 'replace').strip()
    except:
        return None

def GetTargetArmArch():
    # Allows the arm architecture string to be overridden by
    # setting MACOS_ARM_ARCHITECTURE
    return os.environ.get('MACOS_ARM_ARCHITECTURE') or TARGET_ARM64

def GetHostArch():
    macArch = GetCommandOutput(["arch"])
    if macArch == "i386" or macArch == TARGET_X86:
        macArch = TARGET_X86
    else:
        macArch = GetTargetArmArch()
    return macArch

def GetTargetArch(context):
    if TargetEmbeddedOS(context):
        return GetTargetArmArch()

    if context.targetNative:
        macTargets = GetHostArch()
    else:
        if context.targetX86:
            macTargets = TARGET_X86
        if context.targetARM64:
            macTargets = GetTargetArmArch()
        if context.targetUniversal:
            macTargets = TARGET_X86 + ";" + GetTargetArmArch()
    return macTargets

def IsHostArm():
    return GetHostArch() != TARGET_X86

def IsTargetArm(context):
    return GetTargetArch(context) != TARGET_X86

def GetTargetArchPair(context):
    secondaryArch = None

    if context.targetNative:
        primaryArch = GetHostArch()
    if context.targetX86:
        primaryArch = TARGET_X86
    if context.targetARM64:
        primaryArch = GetTargetArmArch()
    if context.buildTarget in EMBEDDED_PLATFORMS:
        primaryArch = GetTargetArmArch()
    if context.targetUniversal:
        primaryArch = GetHostArch()
        if (primaryArch == TARGET_X86):
            secondaryArch = GetTargetArmArch()
        else:
            secondaryArch = TARGET_X86

    return (primaryArch, secondaryArch)

def SupportsMacOSUniversalBinaries():
    if not MacOS():
        return False
    XcodeOutput = GetCommandOutput(["/usr/bin/xcodebuild", "-version"])
    XcodeFind = XcodeOutput.rfind('Xcode ', 0, len(XcodeOutput))
    XcodeVersion = XcodeOutput[XcodeFind:].split(' ')[1]
    return (XcodeVersion > '11.0')

def GetSDKRoot(context) -> Optional[str]:
    sdk = "macosx"
    if context.buildTarget == TARGET_IOS:
        sdk = "iphoneos"
    elif context.buildTarget == TARGET_VISIONOS:
        sdk = "xros"

    for arg in (context.cmakeBuildArgs or '').split():
        if "CMAKE_OSX_SYSROOT" in arg:
            override = arg.split('=')[1].strip('"').strip()
            if override:
                sdk = override
    return GetCommandOutput(["xcrun", "--sdk", sdk, "--show-sdk-path"])

def SetTarget(context, targetName):
    context.targetNative = (targetName == TARGET_NATIVE)
    context.targetX86 = (targetName == TARGET_X86)
    context.targetARM64 = (targetName == GetTargetArmArch())
    context.targetUniversal = (targetName == TARGET_UNIVERSAL)
    context.targetIOS = (targetName == TARGET_IOS)
    context.targetVisionOS = (targetName == TARGET_VISIONOS)
    if context.targetUniversal and not SupportsMacOSUniversalBinaries():
        context.targetUniversal = False
        raise ValueError(
                "Universal binaries only supported in macOS 11.0 and later.")

def GetTargetName(context):
    return (TARGET_NATIVE if context.targetNative else
            TARGET_X86 if context.targetX86 else
            GetTargetArmArch() if context.targetARM64 else
            TARGET_UNIVERSAL if context.targetUniversal else
            context.buildTarget)

devout = open(os.devnull, 'w')

def ExtractFilesRecursive(path, cond):
    files = []
    for r, d, f in os.walk(path):
        for file in f:
            if cond(os.path.join(r,file)):
                files.append(os.path.join(r, file))
    return files

def CodesignFiles(files):
    SDKVersion  = subprocess.check_output(
        ['xcodebuild', '-version']).strip()[6:10]
    codeSignIDs = subprocess.check_output(
        ['security', 'find-identity', '-vp', 'codesigning'])

    codeSignID = "-"
    if os.environ.get('CODE_SIGN_ID'):
        codeSignID = os.environ.get('CODE_SIGN_ID')
    elif float(SDKVersion) >= 11.0 and \
                codeSignIDs.find(b'Apple Development') != -1:
        codeSignID = "Apple Development"
    elif codeSignIDs.find(b'Mac Developer') != -1:
        codeSignID = "Mac Developer"

    for f in files:
        subprocess.call(['codesign', '-f', '-s', '{codesignid}'
                              .format(codesignid=codeSignID), f],
                        stdout=devout, stderr=devout)

def Codesign(install_path, verbose_output=False):
    if not MacOS():
        return False
    if verbose_output:
        global devout
        devout = sys.stdout

    files = ExtractFilesRecursive(install_path,
                 (lambda file: '.so' in file or '.dylib' in file))
    CodesignFiles(files)

def CreateUniversalBinaries(context, libNames, x86Dir, armDir):
    if not MacOS():
        return False
    lipoCommands = []
    xcodeRoot = subprocess.check_output(
        ["xcode-select", "--print-path"]).decode('utf-8').strip()
    lipoBinary = \
        "{XCODE_ROOT}/Toolchains/XcodeDefault.xctoolchain/usr/bin/lipo".format(
                XCODE_ROOT=xcodeRoot)
    for libName in libNames:
        outputDir = os.path.join(context.instDir, "lib")
        if not os.path.isdir(outputDir):
            os.mkdir(outputDir)

        outputName = os.path.join(outputDir, libName)
        if not os.path.islink("{x86Dir}/{libName}".format(
                                x86Dir=x86Dir, libName=libName)):
            if os.path.exists(outputName):
                os.remove(outputName)
            lipoCmd = "{lipo} -create {x86Dir}/{libName} {armDir}/{libName} " \
                      "-output {outputName}".format(
                                lipo=lipoBinary,
                                x86Dir=x86Dir, armDir=armDir,
                                libName=libName, outputName=outputName)
            lipoCommands.append(lipoCmd)
            p = subprocess.Popen(shlex.split(lipoCmd))
            p.wait()
    for libName in libNames:
        if os.path.islink("{x86Dir}/{libName}".format(
                                x86Dir=x86Dir, libName=libName)):
            outputName = os.path.join(context.instDir, "lib", libName)
            if os.path.exists(outputName):
                os.unlink(outputName)
            targetName = os.readlink("{x86Dir}/{libName}".format(
                                x86Dir=x86Dir, libName=libName))
            targetName = os.path.basename(targetName)
            os.symlink("{instDir}/lib/{libName}".format(
                                instDir=context.instDir, libName=targetName),
                       outputName)
    return lipoCommands

def ConfigureCMakeExtraArgs(context, args:List[str]) -> List[str]:
    system_name = None
    if TargetEmbeddedOS(context):
        system_name = context.buildTarget

    if system_name:
        args.append(f"-DCMAKE_SYSTEM_NAME={system_name}")
        args.append(f"-DCMAKE_OSX_SYSROOT={GetSDKRoot(context)}")

        # Required to find locally built libs not from the sysroot.
        args.append(f"-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH")
        args.append(f"-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH")
        args.append(f"-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH")

    return args
