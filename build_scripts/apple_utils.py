#
# Copyright 2022 Pixar
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
import re

TARGET_NATIVE = "native"
TARGET_X86 = "x86_64"
TARGET_ARM64 = "arm64"
TARGET_UNIVERSAL = "universal"
TARGET_IOS = "ios"

def GetBuildTargets():
    return [TARGET_NATIVE,
            TARGET_X86,
            TARGET_ARM64,
            TARGET_UNIVERSAL,
            TARGET_IOS]

def GetBuildTargetDefault():
    return TARGET_NATIVE;

def MacOS():
    return platform.system() == "Darwin"

def GetLocale():
    return sys.stdout.encoding or locale.getdefaultlocale()[1] or "UTF-8"

def GetCommandOutput(command):
    """Executes the specified command and returns output or None.
    If command contains pipes (i.e '|'s), creates a subprocess for
    each pipe in command, returning the output from the last subcommand
    or None if any of the subcommands result in a CalledProcessError"""

    result = None

    args = shlex.split(command)
    commands = []
    cmd_args = []
    while args:
        arg = args.pop(0)
        if arg == '|':
            commands.append((cmd_args))
            cmd_args = []
        else:
            cmd_args.append(arg)
    commands.append((cmd_args))

    pipes = []
    while len(commands) > 1:
        # We have some pipes
        command = commands.pop(0)
        stdin = pipes[-1].stdout if pipes else None
        try:
            pipe = subprocess.Popen(command, stdin=stdin, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            pipes.append(pipe)
        except subprocess.CalledProcessError:
            return None

    # The last command actually returns a result
    command = commands[0]
    try:
        stdin = pipes[-1].stdout if pipes else None
        result = subprocess.check_output(
            command,
            stdin = stdin,
            stderr=subprocess.STDOUT).decode('utf-8').strip()
    except subprocess.CalledProcessError:
        pass

    # clean-up
    for pipe in pipes:
        pipe.wait()

    return result

def GetTargetArmArch():
    # Allows the arm architecture string to be overridden by
    # setting MACOS_ARM_ARCHITECTURE
    return os.environ.get('MACOS_ARM_ARCHITECTURE') or TARGET_ARM64

def GetHostArch():
    macArch = GetCommandOutput('arch').strip()
    if macArch == "i386" or macArch == TARGET_X86:
        macArch = TARGET_X86
    else:
        macArch = GetTargetArmArch()
    return macArch

def GetTargetArch(context):
    if context.targetNative:
        macTargets = GetHostArch()
    else:
        if context.targetX86:
            macTargets = TARGET_X86
        if context.targetARM64 or context.targetIos:
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
    if context.targetIos:
        primaryArch = TARGET_IOS
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
    XcodeOutput = GetCommandOutput('/usr/bin/xcodebuild -version')
    XcodeFind = XcodeOutput.rfind('Xcode ', 0, len(XcodeOutput))
    XcodeVersion = XcodeOutput[XcodeFind:].split(' ')[1]
    return (XcodeVersion > '11.0')

def SetTarget(context, targetName):
    context.targetNative = (targetName == TARGET_NATIVE)
    context.targetX86 = (targetName == TARGET_X86)
    context.targetARM64 = (targetName == GetTargetArmArch())
    context.targetUniversal = (targetName == TARGET_UNIVERSAL)
    context.targetIos = (targetName == TARGET_IOS)
    if context.targetIos:
        sdkStr = GetCommandOutput('xcodebuild -showsdks')
        sdkStrSpl = sdkStr.split()
        for s in sdkStrSpl:
            if s.startswith('iphoneos'):
                s = s.replace('iphoneos', "")
                context.iosVersion = s
                break
    else:
        context.iosVersion = None
    if context.targetUniversal and not SupportsMacOSUniversalBinaries():
        self.targetUniversal = False
        raise ValueError(
                "Universal binaries only supported in macOS 11.0 and later.")

def GetTargetName(context):
    return (TARGET_NATIVE if context.targetNative else
            TARGET_X86 if context.targetX86 else
            GetTargetArmArch() if context.targetARM64 else
            TARGET_UNIVERSAL if context.targetUniversal else
            TARGET_IOS if context.targetIos else
            "")

devout = open(os.devnull, 'w')

def ExtractFilesRecursive(path, cond):
    files = []
    for r, d, f in os.walk(path):
        for file in f:
            if cond(os.path.join(r,file)):
                files.append(os.path.join(r, file))
    return files

def _GetCodeSignStringFromTerminal():
    codeSignIDs = subprocess.check_output(['security',
                                           'find-identity', '-vp', 'codesigning'])
    return codeSignIDs

def GetCodeSignID():
    codeSignIDs = _GetCodeSignStringFromTerminal()
    codeSignID = "-"
    if os.environ.get('CODE_SIGN_ID'):
        codeSignID = os.environ.get('CODE_SIGN_ID')
    else:
        try:
            codeSignID = codeSignIDs.decode("utf-8").split()[1]
        except:
            raise Exception("Unable to parse codesign ID")
    return codeSignID

def GetCodeSignIDHash():
    codeSignIDs = _GetCodeSignStringFromTerminal()
    try:
        return re.findall(r'\(.*?\)', codeSignIDs.decode("utf-8"))[0][1:-1]
    except:
        raise Exception("Unable to parse codesign ID hash")

def GetDevelopmentTeamID():
    if os.environ.get("DEVELOPMENT_TEAM"):
        return os.environ.get("DEVELOPMENT_TEAM")
    codesignID = GetCodeSignIDHash()
    x509subject = GetCommandOutput('security find-certificate -c {}'
                                   ' -p | openssl x509 -subject | head -1'.format(codesignID)).strip()
    # Extract the Organizational Unit (OU field) from the cert
    try:
        team = [elm for elm in x509subject.split(
            '/') if elm.startswith('OU')][0].split('=')[1]
        if team is not None and team != "":
            return team
    except Exception as ex:
        raise Exception("No development team found with exception " + ex)

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
