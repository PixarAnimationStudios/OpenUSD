#!/usr/bin/python
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
TARGET_IOS = "ios"

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
    elif context.targetIos:
        macTargets = TARGET_ARM64
    else:
        if context.targetX86:
            macTargets = TARGET_X86
        if context.targetARM64:
            macTargets = GetMacArmArch()
    return macTargets

def IsMacTargetIntel(context):
    return GetMacTargetArch(context) == TARGET_X86

devout = open(os.devnull, 'w')

def GetFrameworkRoot():
    pathWithFramework = next(path for path in sys.path if (
            "Python.framework" in path or "Python3.framework" in path))
    splitPath = pathWithFramework.split('/')
    index = 0
    for p in range(0, len(splitPath)):
        if splitPath[index] == "Python.framework" or \
                splitPath[index] == "Python3.framework":
            break
        index = index+1
    if index == len(splitPath):
        raise Exception("Could not find Python framework")
    #This should get Python.framework/Version/3.x
    frameworkSplitPath = splitPath[0:index+3]
    frameworkPath = "/".join(frameworkSplitPath)
    return frameworkPath

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