#!/usr/bin/python
import sys
import os
from subprocess import check_output, call

SDKVersion  = check_output(['xcodebuild', '-version']).strip()[6:10]
codeSignIDs = check_output(['security', 'find-identity', '-vp', 'codesigning'])

codeSignID = "-"
if os.environ.get('CODE_SIGN_ID'):
    codeSignID = os.environ.get('CODE_SIGN_ID')
elif float(SDKVersion) >= 11.0 and codeSignIDs.find(b'Apple Development') != -1:
    codeSignID = "Apple Development"
elif codeSignIDs.find(b'Mac Developer') != -1:
    codeSignID = "Mac Developer"

devout = open(os.devnull, 'w')

def ExtractFilesRecursive(path, cond):
    files = []
    for r, d, f in os.walk(path):
        for file in f:
            if cond(os.path.join(r,file)):
                files.append(os.path.join(r, file))
    return files

def CodesignFiles(files):
    for f in files:
        call(['codesign', '-f', '-s', '{codesignid}'
              .format(codesignid=codeSignID), f],
              stdout=devout, stderr=devout)

def MacOSCodesign(install_path, verbose_output=False):
    if verbose_output:
        global devout
        devout = sys.stdout

    files = ExtractFilesRecursive(install_path, 
                 (lambda file: '.so' in file or '.dylib' in file))
    CodesignFiles(files)
