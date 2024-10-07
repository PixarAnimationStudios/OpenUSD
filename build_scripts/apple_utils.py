#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import shutil
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
        subprocess.call(['codesign', '-f', '-s', str(codeSignID), f],
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
    lipoBinary = f"{xcodeRoot}/Toolchains/XcodeDefault.xctoolchain/usr/bin/lipo"
    for libName in libNames:
        outputName = os.path.join(context.instDir, "lib", libName)
        if not os.path.islink(f"{x86Dir}/{libName}"):
            if os.path.exists(outputName):
                os.remove(outputName)
            lipoCmd = f"{lipoBinary} -create {x86Dir}/{libName} {armDir}/{libName} -output {outputName}"
            lipoCommands.append(lipoCmd)
            p = subprocess.Popen(shlex.split(lipoCmd))
            p.wait()
    for libName in libNames:
        if os.path.islink(f"{x86Dir}/{libName}"):
            outputName = os.path.join(context.instDir, "lib", libName)
            if os.path.exists(outputName):
                os.unlink(outputName)
            targetName = os.readlink(f"{x86Dir}/{libName}")
            targetName = os.path.basename(targetName)
            os.symlink(f"{context.instDir}/lib/{targetName}", outputName)
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

def GetTBBPatches(context):
    if context.buildTarget not in EMBEDDED_PLATFORMS or context.buildTarget == TARGET_IOS:
        # TBB already handles these so we don't patch them out
        return [], []

    sdk_name = GetSDKName(context)

    # Standard Target based names
    target_config_patches = [("ios", context.buildTarget.lower()),
                             ("iOS", context.buildTarget),
                             ("IPHONEOS", sdk_name.upper())]

    clang_config_patches = [("ios",context.buildTarget.lower()),
                            ("iOS", context.buildTarget),
                            ("IPHONEOS",sdk_name.upper())]

    # SDK Name's root and minimum visionOS version change
    if context.buildTarget in (TARGET_VISIONOS, TARGET_VISIONOS_SIMULATOR):
        target_config_patches.extend([("iPhone", "XR"),
                                      ("?= 8.0", "?= 1.0")])

        clang_config_patches.append(("iPhone", "XR"),)

    if context.buildTarget == TARGET_VISIONOS:
        clang_config_patches.append(("-miphoneos-version-min=", "-target arm64-apple-xros"))
    else:
        sdk_root = GetSDKRoot(context)
        version=os.path.basename(sdk_root).split("Simulator")[-1].replace(".sdk","")

        if context.buildTarget == TARGET_VISIONOS_SIMULATOR:
            clang_config_patches.append(("-miphoneos-version-min=", f"-target arm64-apple-xros{version}-simulator"))
        elif context.buildTarget == TARGET_IOS_SIMULATOR:
            clang_config_patches.append(("-miphoneos-version-min=", f"-target arm64-apple-ios{version}-simulator"))

    return target_config_patches, clang_config_patches



def BuildXCFramework(root, targets, args):
    if TARGET_UNIVERSAL in targets:
        targets.remove(TARGET_UNIVERSAL)
        targets.extend(TARGET_ARM64, TARGET_X86) # Static builds of TBB can't be built as universal

    print(f"Building {len(targets)} targets...")
    shared_sources = os.path.join(root, "shared_sources")
    os.makedirs(shared_sources, exist_ok=True)

    build_command = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build_usd.py")
    framework_list = []
    for target in targets:

        print(f"Building {target}...")
        install_dir = os.path.join(root, target)
        target_src_dir = os.path.join(install_dir, "src")
        os.makedirs(target_src_dir, exist_ok=True)
        framework = os.path.join(install_dir, "frameworks/OpenUSD.framework")
        framework_list.append(framework)

        # Copy the shared sources over to save time
        for src in os.listdir(shared_sources):
            shared_src = os.path.join(shared_sources, src)
            target_src = os.path.join(target_src_dir, src)
            shutil.copy2(shared_src, target_src)

        target_args = [sys.executable, build_command, install_dir, "--build-target", target, "--build-apple-framework"]
        target_args.extend(args)
        try:
            subprocess.check_call(target_args)
        except:
            raise RuntimeError(f"Failed to build {target} using {' '.join(target_args)}")

        # Copy the unshared sources back as needed
        # We copy the zips in case there are any patches involved
        for src in os.listdir(target_src_dir):
            target_src_path = os.path.join(target_src_dir, src)
            shared_src_path = os.path.join(shared_sources, src)
            if not os.path.exists(shared_src_path) and os.path.isfile(target_src_path):
                shutil.copy2(target_src_path, shared_src_path)


        assert os.path.exists(framework)

    print("Creating XCFramework")

    xcframework_dir = os.path.join(root, "xcframework")
    if os.path.exists(xcframework_dir):
        shutil.rmtree(xcframework_dir)
    os.makedirs(xcframework_dir, exist_ok=True)
    xcframework_path = os.path.join(xcframework_dir, "OpenUSD.xcframework")
    command = ["xcodebuild","-create-xcframework", "-output", xcframework_path]
    for framework in framework_list:
        command.extend(["-framework", framework])

    try:
        subprocess.check_call(command)
    except:
        raise RuntimeError(f"Failed to create XCFramework using {' '.join(command)}")

    subprocess.check_call(["codesign", "--timestamp", "-s", GetCodeSignID(), xcframework_path])


    print("""Success! Add the OpenUSD.xcframework to your Xcode Project.""")

def main():
    import argparse
    parser = argparse.ArgumentParser(description="A set of command line utilities for building on Apple Platforms")
    subparsers = parser.add_subparsers(dest="command", required=True)

    xcframework = subparsers.add_parser("xcframework",
                                        description="Build multiple framework targets together as a single xcframework")
    xcframework.add_argument("install_dir",  type=str,
                             help="Directory where the XCFramework will be installed")
    xcframework.add_argument("--build-targets", nargs="+", help="The list of targets to build.",
                             default=[TARGET_ARM64, TARGET_X86,
                                      TARGET_IOS, TARGET_IOS_SIMULATOR,
                                      TARGET_VISIONOS, TARGET_VISIONOS_SIMULATOR])

    args, unknown = parser.parse_known_args()
    command = args.command
    if command == "xcframework":
        BuildXCFramework(args.install_dir, args.build_targets, unknown)
    else:
        raise RuntimeError(f"Unknown command: {command}")

if __name__ == '__main__':
    main()