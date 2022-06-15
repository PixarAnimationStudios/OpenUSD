#!/usr/bin/python
import sys
import locale
import os
import shlex
import subprocess

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
    return os.environ.get('MACOS_ARM_ARCHITECTURE') or "arm64"

def GetMacArch():
    macArch = GetCommandOutput('arch').strip()
    if macArch == "i386" or macArch == "x86_64":
        macArch = "x86_64"
    else:
        macArch = GetMacArmArch()
    return macArch

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
    if verbose_output:
        global devout
        devout = sys.stdout

    files = ExtractFilesRecursive(install_path, 
                 (lambda file: '.so' in file or '.dylib' in file))
    CodesignFiles(files)
