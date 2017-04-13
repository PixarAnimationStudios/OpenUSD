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
# Generate doxygen based docs for USD.

import sys, os, argparse, shutil, subprocess, tempfile, platform, stat

# This finds all modules in the pxr/ source area, such as ar, usdGeom etc.
def _getModules(pxrSourceRoot):
    modules = []
    topLevel = []
    for p in os.listdir(pxrSourceRoot):
        # Ignore any hidden directories
        if os.path.basename(p).startswith('.'):
            continue
        path = os.path.join(os.path.join(pxrSourceRoot, p), 'lib/')
        if os.path.isdir(path):
            topLevel.append(path)
    for t in topLevel:
        for p in os.listdir(t):
            if os.path.basename(p).startswith('.'):
                continue
            # Ignore any irrelevant directories
            elif os.path.basename(p).startswith('CMakeFiles'):
                continue
            elif os.path.basename(p).startswith('testenv'):
                continue
            path = os.path.join(os.path.join(pxrSourceRoot, t), p)
            if os.path.isdir(path):
                modules.append(path)
    return modules

def _generateDocs(pxrSourceRoot, installLoc, doxygenBin, doxyConfig, dotBin):
    platformIsWindows = platform.system() == 'Windows'

    docsLoc = os.path.join(installLoc, 'src/modules')
    if not os.path.exists(docsLoc):
        os.makedirs(docsLoc)

    for mod in _getModules(pxrSourceRoot):
        target = os.path.join(docsLoc, os.path.basename(mod))
        # This ensures that we get a fresh view of the source
        # on each build invocation.
        if os.path.exists(target):
            def _removeReadOnly(func, path, exc):
                try:
                    os.chmod(path, stat.S_IWRITE)
                    func(path)
                except Exception as exc:
                    print >>sys.stderr, "Failed to remove %s: %s" % (path, str(exc))
            shutil.rmtree(target, onerror=_removeReadOnly)
        shutil.copytree(mod, target) 
    
    os.chdir(installLoc)

    # Generate a new doxyConfig containing the dot executable path
    with open(doxyConfig, 'r') as doxyConfigHandle:
        doxyConfigContents = doxyConfigHandle.read()
        doxyConfigContents += ('DOT_PATH\t\t = %s' % str(dotBin))
    
    outputDoxyConfig = os.path.join(docsLoc, 'Doxyfile')
    with open(outputDoxyConfig, 'w') as outputDoxyConfigHandle:
        outputDoxyConfigHandle.write(doxyConfigContents)

    cmd = [doxygenBin, outputDoxyConfig] 
    with tempfile.TemporaryFile() as tmpFile:
        if subprocess.call(cmd, stdout=tmpFile, stderr=tmpFile) != 0:
            sys.stderr.write(tmpFile.read())
            sys.exit('Error: doxygen failed to complete.')

def _generateHeaderImpl(targetPath, targetContent):
    if os.path.exists(targetPath):
        os.remove(targetPath)

    with open(targetPath, 'w') as targetHandle:
        targetHandle.write(targetContent)

def _generateHeaders(installLoc):
    headerLoc = os.path.join(installLoc, 'src/')
    if not os.path.exists(headerLoc):
        os.mkdir(headerLoc)

    clientContent = ("/*!\n\page Usd_Separator_Header_Primary Primary "
                     "Client-Facing Modules\n*/") 
    clientFilePath = os.path.join(installLoc, "clientModuleHeader.dox")
    _generateHeaderImpl(clientFilePath, clientContent)    

    moduleContent = ("/*!\n\page Usd_Separator_Header_Foundation "
                     "Foundation/Support Modules\n*/")
    moduleFilePath = os.path.join(installLoc, "foundationModuleHeader.dox")
    _generateHeaderImpl(moduleFilePath, moduleContent)

def _checkPath(path, ident, perm):
    if not os.path.exists(path):
        sys.exit('Error: file path %s (arg=%s) '
                 'does not exist, exiting.' % (path,ident))
    elif not os.access(path, perm):
        permString = '-permission'
        if perm == os.W_OK:
            permString = 'write'+permString
        elif perm == os.R_OK:
            permString = 'read'+permString
        elif perm == os.X_OK:
            permString = 'execute'+permString
        sys.exit('Error: insufficient permission for path %s, '
                 '%s required.' % (path, permString))

def _generateInstallLoc(installRoot):
    installPath = os.path.join(installRoot, 'docs')
    if not os.path.exists(installPath):
        os.mkdir(installPath)
    return installPath

if __name__ == "__main__":
    p = argparse.ArgumentParser(description="Generate USD documentation.") 
    p.add_argument("source",  help="The path to the pxr source root.")
    p.add_argument("install", help="The install root for generated docs.")
    p.add_argument("doxygen", help="The path to the doxygen executable.")
    p.add_argument("config",  help="The path to the doxygen config.")
    p.add_argument("dot",     help="The path to the dot executable.")
    args = p.parse_args()

    # Ensure all paths exist first
    _checkPath(args.doxygen, 'doxygen', os.X_OK)
    _checkPath(args.install, 'install', os.W_OK)
    _checkPath(args.source, 'source', os.R_OK)
    _checkPath(args.config, 'config', os.R_OK)
    _checkPath(args.dot, 'dot', os.X_OK)
     
    installPath = _generateInstallLoc(args.install)
    _generateHeaders(installPath)
    _generateDocs(args.source, installPath, args.doxygen, args.config, args.dot) 
