#
# Copyright 2020 Pixar
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

import tempfile
import shutil
import os
import zipfile

from pxr import Usd


class ExpandedUSDZ(object):
    """
    A context library for expanding a given usdz file in to separate
    files on disk so that they can be operated on.

    Args:
       source (str): A path to the usdz file
       destination (str): An optional location to recompress the usdz to. Overwrites the source usd if not given.
       rezip (bool): Whether or not you want to rezip the file on closing this context. Defaults true if Destination is provided.
       cleanup (bool): Whether or not you want this context to clean up temporary files after itself
    """

    def __init__(self, source, destination=None, rezip=None, cleanup=True):
        # Check that source is a valid usdz
        self.__source = os.path.abspath(source)
        if not os.path.exists(self.__source):
            raise IOError("Could not find source file")

        _, ext = os.path.splitext(self.__source)
        if ext != ".usdz":
            raise ValueError("Source path must be a .usdz file")

        self.destination = destination or source

        self.__path = None
        self.__defaultLayer = None

        # If a destination is provided, then it makes sense to default rezip to True unless the user specifies otherwise
        # Note: It doesn't make sense to have a destination provided and rezip set to False, but I think it's better to respect the users wishes.
        self.rezip = bool(destination) if rezip is None else rezip
        self.cleanup = cleanup

        self.__extract()

    def __extract(self):
        """
        Extracts the files to a temp location and finds the default layer within it
        """
        base, ext = os.path.splitext(os.path.basename(self.source))
        self.__path = tempfile.mkdtemp(
            prefix="{}_".format(base), suffix=ext.replace(".", "_")
        )

        with zipfile.ZipFile(self.source, "r") as zh:
            zh.extractall(self.path)

            # This is the only way I could find to programmatically find the default layer.
            # The usdz spec says the first file must be a usd and is treated as a default layer.
            for name in zh.namelist():
                _, ext = os.path.splitext(name)
                if ext.startswith(".usd"):
                    self.__defaultLayer = name
                    break

        return self

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.rezip:
            self.package()

        if self.cleanup:
            self.cleanupFiles()

    def package(self):
        """
        Packages up the temp directory in to a usdz file and copies it to a destination
        """
        with tempfile.NamedTemporaryFile(suffix=".usdz", delete=self.cleanup) as tmp:
            self.__package(tmp.name)
            shutil.copy(tmp.name, self.destination)

    def __package(self, temp):
        # Find all the paths we need to add to the zip
        paths = []
        for root, dirs, files in os.walk(self.path):
            for name in files:
                path = os.path.join(root, name)
                paths.append(path)

        # Try and find the default layer if it exists or fall back
        if os.path.exists(self.defaultLayerPath):
            paths.sort(key=lambda x: x == self.defaultLayerPath, reverse=True)
        else:
            print(
                "Warning: Could not find the Default Layer file. Using first valid usd file instead."
            )
            paths.sort(
                key=lambda x: os.path.splitext(x)[-1].startswith(".usd"), reverse=True
            )

        # Create the usdz file
        with Usd.ZipFileWriter.CreateNew(temp) as usdz:
            for path in paths:
                relpath = os.path.relpath(path, self.path)
                usdz.AddFile(path, relpath)

    def cleanupFiles(self):
        """Cleans up the temporary extraction path"""
        try:
            shutil.rmtree(self.path)
        except FileNotFoundError:
            pass

    @property
    def source(self):
        """The source usdz file for this expansion"""
        return self.__source

    @property
    def destination(self):
        """The destination usdz path to repackage up the files to"""
        return self.__destination

    @destination.setter
    def destination(self, path):
        _, ext = os.path.splitext(path)
        if ext != ".usdz":
            raise ValueError("Destination path must be a .usdz file")

        self.__destination = path

    @property
    def path(self):
        """The temporary path where the source usdz file is expanded to"""
        return self.__path

    @property
    def defaultLayer(self):
        """The default layer file for the usdz"""
        return self.__defaultLayer

    @defaultLayer.setter
    def defaultLayer(self, path):
        _, ext = os.path.splitext(path)
        if not ext.startswith(".usd"):
            raise ValueError("Default Layer must be a usd file")

        if not os.path.abspath(path).startswith(self.path):
            raise ValueError("Default Layer must be a child of {}".format(self.path))

        self.__defaultLayer = os.path.relpath(path, self.path)

    @property
    def defaultLayerPath(self):
        """The full path to the default layer file within the temporary directory"""
        return os.path.join(self.path, self.defaultLayer)
