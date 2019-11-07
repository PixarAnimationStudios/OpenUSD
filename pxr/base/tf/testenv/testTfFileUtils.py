#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

import datetime
import logging
import os
import shutil
import sys
import time
import unittest

from pxr import Tf

def Lstat(path):
    return int(os.lstat(path).st_mtime)

def Strftime(t):
    return datetime.datetime.fromtimestamp(float(str(t))).strftime('%c')

class TestFileUtils(unittest.TestCase):
    """
    Test Tf File Utils (The python wrapped porting of the utility functions).
    """

    EmptyDirStructure = [
    ('empty', ['sub1','sub2','sub3'], []),
    ('empty/sub1', ['sub1sub1','sub1sub2'], []),
    ('empty/sub2', ['sub2sub1','sub2sub2'], []),
    ('empty/sub3', ['sub3sub1','sub3sub2'], []),
    ]

    DirStructure = [
    ('notEmpty', ['sub1','sub2','sub3'], ['a','b','c']),
    ('notEmpty/sub1', ['sub1sub1','sub1sub2'], ['a','b','c']),
    ('notEmpty/sub2', ['sub2sub1','sub2sub2'], ['a','b','c']),
    ('notEmpty/sub3', ['sub3sub1','sub3sub2'], ['a','b','c']),
    ]

    CollideDirStructure = [
    ('alreadyThere', ['sub1','sub2','sub3'], ['a','b','c']),
    ('alreadyThere/sub1', ['sub1sub1','sub1sub2'], ['a','b','c']),
    ('alreadyThere/sub2', ['sub2sub1','sub2sub2'], ['a','b','c']),
    ('alreadyThere/sub3', ['sub3sub1','sub3sub2'], ['a','b','c']),
    ]

    def CreateFile(self, filename, data):
        """ Create file and register for cleanup"""
        f = open(filename,"w")
        f.write(data)
        f.close()   
        self.files.append(filename)

    def CreateSymlink(self, filename, linkname):
        """ Create symlink, remove old link if it exists."""
        if os.path.islink(linkname):
            os.unlink(linkname)

        if hasattr(os, 'symlink'):
            os.symlink(filename, linkname)

    def SetupDirStructure(self, structure, withSymlink=False):
        """Create a dir structure for testing"""
        self.log.info("Creating test directory structure...")
        for path, dirs, files in structure:
            if not os.path.isdir(path):
                print 'os.makedirs(%s)' % path
                os.makedirs(path)
        
            for d in [os.path.join(path, d) for d in dirs]:
                if not os.path.isdir(d):
                    print 'os.makedirs(%s)' % d
                    os.makedirs(d)
        
            for f in [os.path.join(path, f) for f in files]:
                if not os.path.isfile(f):
                    Tf.TouchFile(f, True)

        if withSymlink:
            (path, testDir, testFile) = structure[0]
            testFile = os.path.join(os.getcwd(), "linkTarget")
            link = os.path.join(path, "symlink")
            self.CreateFile(testFile, "link")
            self.CreateSymlink(testFile, link)
        
    def RemoveDirs(self):
        """ Remove all dirs which have been created/moved"""
        if os.path.isdir('movedEmpty'):
            shutil.rmtree('movedEmpty', True)
        if os.path.isdir('movedNotEmpty'):
            shutil.rmtree('movedNotEmpty', True)
        if os.path.isdir('alreadyThere'):
            shutil.rmtree('alreadyThere', True)

    def RemoveLinks(self):
        """ Remove all links which have been created/moved"""
        if os.path.islink(self.link):
            os.unlink(self.link)
        if os.path.islink(self.copyLink):
            os.unlink(self.copyLink)

    def VerifyDirStructure(self, rootDir):
        '''
        Verifies if given rootdir and all subdirs created by SetupDirStructure
        exist.
        '''

        self.assertTrue(os.path.isdir(rootDir))
        self.assertTrue(os.path.isdir("%s/sub1" % rootDir))
        self.assertTrue(os.path.isdir("%s/sub2" % rootDir))
        self.assertTrue(os.path.isdir("%s/sub3" % rootDir))
        self.assertTrue(os.path.isdir("%s/sub1/sub1sub1" % rootDir))
        self.assertTrue(os.path.isdir("%s/sub1/sub1sub2" % rootDir))
        self.assertTrue(os.path.isdir("%s/sub2/sub2sub1" % rootDir))
        self.assertTrue(os.path.isdir("%s/sub2/sub2sub2" % rootDir))
        self.assertTrue(os.path.isdir("%s/sub3/sub3sub1" % rootDir))
        self.assertTrue(os.path.isdir("%s/sub3/sub3sub2" % rootDir))

    def VerifyDirAndFileStructure(self, rootDir):
        '''
        Verifies if given rootdir and all subdirs created by SetupDirStructure
        exist.
        '''

        self.VerifyDirStructure(rootDir)

        self.assertTrue(os.path.isfile("%s/a" % rootDir))
        self.assertTrue(os.path.isfile("%s/b" % rootDir))
        self.assertTrue(os.path.isfile("%s/c" % rootDir))

        self.assertTrue(os.path.isfile("%s/sub1/a" % rootDir))
        self.assertTrue(os.path.isfile("%s/sub1/b" % rootDir))
        self.assertTrue(os.path.isfile("%s/sub1/c" % rootDir))

        self.assertTrue(os.path.isfile("%s/sub2/a" % rootDir))
        self.assertTrue(os.path.isfile("%s/sub2/b" % rootDir))
        self.assertTrue(os.path.isfile("%s/sub2/c" % rootDir))

        self.assertTrue(os.path.isfile("%s/sub3/a" % rootDir))
        self.assertTrue(os.path.isfile("%s/sub3/b" % rootDir))
        self.assertTrue(os.path.isfile("%s/sub3/c" % rootDir))

    def setUp(self):
        """ Setup of test directories and symlinks"""
        self.log = logging.getLogger()
        self.files = []

        self.file     = os.path.join(os.getcwd(), "copyTarget")
        self.link     = os.path.join(os.getcwd(), "symlink")
        self.copyLink = os.path.join(os.getcwd(), "copiedLink")

        #  Potentially cleanup liks and test dirs...
        self.RemoveDirs()
        self.RemoveLinks()

        self.SetupDirStructure(self.EmptyDirStructure)
        self.SetupDirStructure(self.DirStructure, True)
        self.SetupDirStructure(self.CollideDirStructure)

        # create a file and a symlink to the file and prepare a path
        # for a symlink copy, used by the CopySymlink test...
        self.CreateFile(self.file, "content")
        self.CreateSymlink(self.file, self.link)


    def tearDown(self):
        """Remove test directories and symlinks"""

        self.RemoveDirs()
        self.RemoveLinks()

        for f in self.files:
            os.remove(f)


    def test_Touch(self):
        """Testing Touch() function"""

        self.log.info("Touch non-existent file")
        self.assertFalse(Tf.TouchFile("touchFile", False))
        self.assertFalse(os.path.isfile("touchFile"))

        self.log.info("Touch non-existent file with create flag")
        self.assertTrue(Tf.TouchFile("touchFile", True))
        self.assertTrue(os.path.isfile("touchFile"))

        self.log.info("Test if touch updates the mod time")

        st = os.stat("touchFile")
        oldTime = st.st_mtime
    
        time.sleep(1)
    
        self.assertTrue(Tf.TouchFile("touchFile", False))
    
        st = os.stat("touchFile")
        newTime = st.st_mtime
    
        # Mod time should have been updated by the Has() call.
        self.assertTrue(newTime > oldTime)
    
        self.files.append("touchFile")

    Links = [
        ('file', 'a'),
        ('a', 'b'),
        ('b', 'c'),
        ('c', 'd'),
        ]

    def PrintTestLinks(self):
        mtime = Lstat(self.Links[0][0])
        self.log.info('%s %s %s' % (
            mtime, Strftime(mtime), self.Links[0][0]))
        for sp,dp in self.Links:
            mtime = Lstat(dp)
            self.log.info('%s %s %s -> %s' % (
                mtime, Strftime(mtime), dp, sp))

    def RemoveTestLinks(self):
        allFiles = set()
        [allFiles.update(list(i)) for i in self.Links]
        for f in allFiles:
            try: os.unlink(f) 
            except OSError:
                pass

    def CreateTestLinks(self):
        self.RemoveTestLinks()
        filePath = self.Links[0][0]
        open(filePath, 'w').write('The real file\n')
        linkPaths = []
        for s,d in self.Links:
            self.CreateSymlink(s, d)
        self.PrintTestLinks()


if __name__ == '__main__':
    unittest.main()

