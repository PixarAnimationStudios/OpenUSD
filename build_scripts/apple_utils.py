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
import re
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
TARGET_IOS_SIMULATOR = "iOSSimulator"
TARGET_VISIONOS_SIMULATOR = "visionOSSimulator"

EMBEDDED_PLATFORMS = [TARGET_IOS, TARGET_IOS_SIMULATOR,
                      TARGET_VISIONOS, TARGET_VISIONOS_SIMULATOR]

def GetBuildTargets():
    return [TARGET_NATIVE,
            TARGET_X86,
            TARGET_ARM64,
            TARGET_UNIVERSAL,
            TARGET_IOS,
            TARGET_IOS_SIMULATOR,
            TARGET_VISIONOS,
            TARGET_VISIONOS_SIMULATOR]

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


def GetSDKName(context) -> str:
    sdk = "macosx"
    if context.buildTarget == TARGET_IOS:
        sdk = "iPhoneOS"
    elif context.buildTarget == TARGET_IOS_SIMULATOR:
        sdk = "iPhoneSimulator"
    elif context.buildTarget == TARGET_VISIONOS:
        sdk = "xrOS"
    elif context.buildTarget == TARGET_VISIONOS_SIMULATOR:
        sdk = "xrSimulator"

    return sdk

def GetSDKRoot(context) -> Optional[str]:
    sdk = GetSDKName(context).lower()

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
    context.targetIOS = (targetName in (TARGET_IOS, TARGET_IOS_SIMULATOR))
    context.targetVisionOS = (targetName in (TARGET_VISIONOS, TARGET_VISIONOS_SIMULATOR))
    context.targetSimulator = (targetName in (TARGET_IOS_SIMULATOR, TARGET_VISIONOS_SIMULATOR))
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

def GetTargetPlatform(context):
    return GetTargetName(context).replace("Simulator", "")

devout = open(os.devnull, 'w')

def ExtractFilesRecursive(path, cond):
    files = []
    for r, d, f in os.walk(path):
        for file in f:
            if cond(os.path.join(r,file)):
                files.append(os.path.join(r, file))
    return files

def _GetCodeSignStringFromTerminal():
    codeSignIDs = GetCommandOutput(['security', 'find-identity', '-vp', 'codesigning'])
    return codeSignIDs

def GetCodeSignID():
    if os.environ.get('CODE_SIGN_ID'):
        return os.environ.get('CODE_SIGN_ID')

    codeSignIDs = _GetCodeSignStringFromTerminal()
    if not codeSignIDs:
        return "-"
    for codeSignID in codeSignIDs.splitlines():
        if "CSSMERR_TP_CERT_REVOKED" in codeSignID:
            continue
        if ")" not in codeSignID:
            continue
        codeSignID = codeSignID.split()[1]
        break
    else:
        raise RuntimeError("Could not find a valid codesigning ID")

    return codeSignID or "-"

def GetCodeSignIDHash():
    codeSignIDs = _GetCodeSignStringFromTerminal()
    try:
        return re.findall(r'\(.*?\)', codeSignIDs)[0][1:-1]
    except:
        raise Exception("Unable to parse codesign ID hash")

def GetDevelopmentTeamID():
    if os.environ.get("DEVELOPMENT_TEAM"):
        return os.environ.get("DEVELOPMENT_TEAM")
    codesignID = GetCodeSignIDHash()

    certs = subprocess.check_output(["security", "find-certificate", "-c", codesignID, "-p"])
    subject = GetCommandOutput(["openssl", "x509", "-subject"], input=certs)
    subject = subject.splitlines()[0]
    match = re.search("OU\s*=\s*(?P<team>([A-Za-z0-9_])+)",  subject)
    if not match:
        raise RuntimeError("Could not parse the output certificate to find the team ID")

    groups = match.groupdict()
    team = groups.get("team")

    if not team:
        raise RuntimeError("Could not extract team id from certificate")

    return team


def CodesignFiles(files):
    codeSignID = GetCodeSignID()

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
        outputName = os.path.join(context.instDir, "lib", libName)
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
        system_name = GetTargetPlatform(context)

    if system_name:
        args.append(f"-DCMAKE_SYSTEM_NAME={system_name}")
        args.append(f"-DCMAKE_OSX_SYSROOT={GetSDKRoot(context)}")

        # Required to find locally built libs not from the sysroot.
        args.append(f"-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH")
        args.append(f"-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH")
        args.append(f"-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH")


    # Signing needs to happen on all systems
    if context.macOSCodesign:
        args.append(f"-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY={GetCodeSignID()}")
        args.append(f"-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM={GetDevelopmentTeamID()}")

    return args

def BuildMultipleTargets(args, context_class):
    if not args.no_build_apple_framework:
        embedded_targets = any(t for t in args.build_target if t in EMBEDDED_PLATFORMS)
        args.build_apple_framework = embedded_targets

    root = args.install_dir
    raise RuntimeError("Testing")