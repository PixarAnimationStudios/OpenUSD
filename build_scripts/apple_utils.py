#!/usr/bin/python
import sys
import locale
import os
import platform
import shlex
import subprocess

TARGET_NATIVE = "native"
TARGET_X86 = "x86_64"
TARGET_ARM64 = "arm64"

def MacOS():
    return platform.system() == "Darwin"

def GetLocale():
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

def GetMacArmArch():
    # Allows the arm architecture string to be overridden by setting MACOS_ARM_ARCHITECTURE
    return os.environ.get('MACOS_ARM_ARCHITECTURE') or TARGET_ARM64

def GetMacArch():
    macArch = GetCommandOutput('arch').strip()
    if macArch == "i386" or macArch == TARGET_X86:
        macArch = TARGET_X86
    else:
        macArch = GetMacArmArch()
    return macArch

def GetMacTargetArch(context):
    if context.targetNative:
        macTargets = GetMacArch()
    else:
        if context.targetX86:
            macTargets = TARGET_X86
        if context.targetARM64:
            macTargets = GetMacArmArch()
    return macTargets

def IsMacTargetIntel(context):
    return GetMacTargetArch(context) == TARGET_X86

devout = open(os.devnull, 'w')

def ExtractFilesRecursive(path, cond):
    files = []
    for r, d, f in os.walk(path):
        for file in f:
            if cond(os.path.join(r,file)):
                files.append(os.path.join(r, file))
    return files

def CodesignFiles(files):
    SDKVersion  = subprocess.check_output(['xcodebuild', '-version']).strip()[6:10]
    codeSignIDs = subprocess.check_output(['security', 'find-identity', '-vp', 'codesigning'])

    codeSignID = "-"
    if os.environ.get('CODE_SIGN_ID'):
        codeSignID = os.environ.get('CODE_SIGN_ID')
    elif float(SDKVersion) >= 11.0 and codeSignIDs.find(b'Apple Development') != -1:
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

