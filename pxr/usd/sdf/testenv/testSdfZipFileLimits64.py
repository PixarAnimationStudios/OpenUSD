#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os
import struct
import tempfile
import unittest
import zipfile

from pxr import Sdf, Tf



MAX_ZIP32_ENTRY_SIZE = Tf.GetEnvSetting('SDF_MAX_ZIP32_ENTRY_SIZE')
MAX_ZIP32_ZIPFILE_SIZE = Tf.GetEnvSetting('SDF_MAX_ZIP32_ZIPFILE_SIZE')
MAX_ZIP32_ENTRY_COUNT = Tf.GetEnvSetting('SDF_MAX_ZIP32_ENTRY_COUNT')

class TestSdfZipFileLimits64(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        tmp = tempfile.TemporaryDirectory(prefix="testFiles")
        cls.addClassCleanup(tmp.cleanup)

        def CreateTestFile(size):
            fd, path = tempfile.mkstemp(dir=tmp.name)
            with os.fdopen(fd, "wb") as f:
                f.truncate(size)

            return path

        cls.smallFile = CreateTestFile(16)
        cls.mediumFile = CreateTestFile(MAX_ZIP32_ENTRY_SIZE - 1)
        cls.largeFile = CreateTestFile(MAX_ZIP32_ENTRY_SIZE + 1)
        cls.xlargeFile = CreateTestFile(MAX_ZIP32_ZIPFILE_SIZE - 1)

    def ReadEntryCounts(self, path):
        """Returns the (entries on disk, total entries) counts from the end of
        central directory record and from the Zip64 end of central directory
        record. The Zip64 counts are None if the Zip64 records are not
        present. Assumes no archive comment and no Zip64 extensible data."""
        with open(path, "rb") as f:
            data = f.read()

        # End of central directory record (4.3.16), 22 bytes
        eocdStart = len(data) - 22
        (sig, _, _, entriesOnDisk, entries, _, _, _) = \
            struct.unpack_from("<IHHHHIIH", data, eocdStart)
        self.assertEqual(sig, 0x06054b50)
        eocdCounts = (entriesOnDisk, entries)

        # Zip64 end of central directory locator (4.3.15), 20 bytes
        locatorStart = eocdStart - 20
        if locatorStart < 0:
            return eocdCounts, None
        (sig, _, zip64RecordOffset, _) = \
            struct.unpack_from("<IIQI", data, locatorStart)
        if sig != 0x07064b50:
            return eocdCounts, None

        # Zip64 end of central directory record (4.3.14), 56 bytes
        (sig, _, _, _, _, _, entriesOnDisk64, entries64, _, _) = \
            struct.unpack_from("<IQHHIIQQQQ", data, zip64RecordOffset)
        self.assertEqual(sig, 0x06064b50)
        return eocdCounts, (entriesOnDisk64, entries64)

    def ReadCentralDirectoryHeaders(self, path):
        """Extracts and returns (list(CentralDirectoryHeader), data) from
        the usdz archive at the given path"""
        with open(path, "rb") as f:
            data = f.read()

        # End of central directory record (4.3.16), 22 bytes
        eocdStart = len(data) - 22
        (sig, _, _, _, numEntries, _, cdOffset, _) = \
            struct.unpack_from("<IHHHHIIH", data, eocdStart)
        self.assertEqual(sig, 0x06054b50)

        # If present, the Zip64 end of central directory record (4.3.14)
        # holds the actual entry count and central directory offset.
        locatorStart = eocdStart - 20
        if locatorStart >= 0:
            (sig, _, zip64RecordOffset, _) = \
                struct.unpack_from("<IIQI", data, locatorStart)
            if sig == 0x07064b50:
                (sig, _, _, _, _, _, _, numEntries, _, cdOffset) = \
                    struct.unpack_from("<IQHHIIQQQQ", data, zip64RecordOffset)
                self.assertEqual(sig, 0x06064b50)

        # Central directory headers (4.3.12), 46 bytes plus variable fields
        headers = []
        offset = cdOffset
        for _ in range(numEntries):
            (sig, _, _, _, _, _, _, _, compressedSize, uncompressedSize,
             filenameLength, extraFieldLength, commentLength, _, _, _,
             localHeaderOffset) = \
                struct.unpack_from("<IHHHHHHIIIHHHHHII", data, offset)
            self.assertEqual(sig, 0x02014b50)
            offset += 46

            filename = data[offset:offset + filenameLength].decode()
            offset += filenameLength

            # Walk the extra field blocks looking for the Zip64 extended
            # information extra field (4.5.3).
            zip64Data = None
            extraEnd = offset + extraFieldLength
            while offset < extraEnd:
                headerId, dataSize = struct.unpack_from("<HH", data, offset)
                offset += 4
                if headerId == 0x0001:
                    zip64Data = data[offset:offset + dataSize]
                offset += dataSize
            self.assertEqual(offset, extraEnd)

            offset += commentLength

            headers.append({
                "filename": filename,
                "compressedSize": compressedSize,
                "uncompressedSize": uncompressedSize,
                "localHeaderOffset": localHeaderOffset,
                "zip64Data": zip64Data,
            })

        return headers, data

    def _AssertLocalFileHeaderFileName(self, data, localHeaderOffset, filename):
        """Checks that the local file header starting at localHeaderOffset
        has the expected filename. Tests that the correct offset can be
        read from the end of central directory header."""
        # The offset must point at the local file header for this entry.
        (sig,) = struct.unpack_from("<I", data, localHeaderOffset)
        self.assertEqual(sig, 0x04034b50)
        (filenameLength,) = \
            struct.unpack_from("<H", data, localHeaderOffset + 26)
        self.assertEqual(
            data[localHeaderOffset + 30:
                    localHeaderOffset + 30 + filenameLength].decode(),
            filename)

    def _AssertZip32Record(self, data, record, filename, size):
        """Checks that size and offset values in the central directory
        record are expected values in the case where sentinel values
        are not needed"""
        self.assertEqual(record["compressedSize"], size)
        self.assertEqual(record["uncompressedSize"], size)
        localHeaderOffset = record["localHeaderOffset"]
        self.assertLess(size, MAX_ZIP32_ENTRY_SIZE)
        self.assertLess(localHeaderOffset, MAX_ZIP32_ZIPFILE_SIZE)
        self._AssertLocalFileHeaderFileName(data, localHeaderOffset, filename)

    def _AssertZip64Record(self, data, record, filename, size):
        """Checks that size and offset values in the central directory
        record are expected values in the case where sentinel values
        are needed"""
        self.assertEqual(record["compressedSize"], 0xFFFFFFFF)
        self.assertEqual(record["uncompressedSize"], 0xFFFFFFFF)
        self.assertEqual(record["localHeaderOffset"], 0xFFFFFFFF)
        self.assertEqual(len(record["zip64Data"]), 24)
        (uncompressedSize64, compressedSize64, localHeaderOffset) = \
            struct.unpack("<QQQ", record["zip64Data"])
        self.assertEqual(uncompressedSize64, size)
        self.assertEqual(compressedSize64, size)
        self._AssertLocalFileHeaderFileName(data, localHeaderOffset, filename)

    def test_add64BitEntry(self):
        """Tests that adding an entry larger than MAX_ZIP32_ENTRY_SIZE properly
        sets the version needed to extract and correct file sizes are decoded
        from the local file header"""

        usdzFile = "test64BitEntry.usdz"

        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)

            zfw.AddFile(type(self).smallFile, "small")
            zfw.AddFile(type(self).largeFile, "large")

        self.assertTrue(os.path.isfile(usdzFile))
        zf = Sdf.ZipFile.Open(usdzFile)

        self.assertEqual(zf.GetFileInfo("small").versionNeededToExtract, 10)
        self.assertEqual(zf.GetFileInfo("large").versionNeededToExtract, 45)

    def test_64bitEntryFileSizes(self):
        """Tests that when a entry that is greater than MAX_ZIP32_ENTRY_SIZE
        is added to the archive that the actual size values are read and the
        true size is reflected in the file info"""

        usdzFile = "test64BitFileSizes.usdz"

        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)

            zfw.AddFile(type(self).largeFile, "large")

        self.assertTrue(os.path.isfile(usdzFile))
        zf = Sdf.ZipFile.Open(usdzFile)

        info = zf.GetFileInfo("large")
        self.assertIsNotNone(info)

        self.assertEqual(info.size, MAX_ZIP32_ENTRY_SIZE + 1)
        self.assertEqual(info.uncompressedSize, MAX_ZIP32_ENTRY_SIZE + 1)
        self.assertEqual(info.versionNeededToExtract, 45)

    def test_archiveGrowsLargerThanZip32(self):
        """Tests that adding enough entries which are smaller than 
        MAX_ZIP32_ENTRY_SIZE that push the archive over MAX_ZIP32_ZIPFILE_SIZE
        result in 64 bit extensions being used"""

        usdzFile = "test64BitArchive.usdz"

        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)

            zfw.AddFile(type(self).mediumFile, "file1")
            zfw.AddFile(type(self).mediumFile, "file2")
            zfw.AddFile(type(self).mediumFile, "file3")

        self.assertTrue(os.path.isfile(usdzFile))
        zf = Sdf.ZipFile.Open(usdzFile)

        self.assertEqual(zf.GetFileInfo("file1").versionNeededToExtract, 10)
        self.assertEqual(zf.GetFileInfo("file2").versionNeededToExtract, 10)
        self.assertEqual(zf.GetFileInfo("file3").versionNeededToExtract, 45)

    def test_entryCountFitsInZip32(self):
        """Tests that an archive with fewer than MAX_ZIP32_ENTRY_COUNT entries
        does not write the Zip64 end of central directory records"""

        usdzFile = "testEntryCountBelowZip32.usdz"

        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)

            zfw.AddFile(type(self).smallFile, "file1")
            zfw.AddFile(type(self).smallFile, "file2")

        self.assertTrue(os.path.isfile(usdzFile))

        eocdCounts, zip64Counts = self.ReadEntryCounts(usdzFile)
        self.assertEqual(eocdCounts, (2, 2))
        self.assertIsNone(zip64Counts)

    def test_entryCountExceedsZip32(self):
        """Tests that an archive with MAX_ZIP32_ENTRY_COUNT or more entries
        stores the sentinel value in the end of central directory record
        entry counts and the actual counts in the Zip64 end of central
        directory record"""

        usdzFile = "testEntryCountExceedsZip32.usdz"

        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)

            for i in range(MAX_ZIP32_ENTRY_COUNT):
                zfw.AddFile(type(self).smallFile, f"file{i}")

        self.assertTrue(os.path.isfile(usdzFile))

        eocdCounts, zip64Counts = self.ReadEntryCounts(usdzFile)
        self.assertEqual(eocdCounts, (0xFFFF, 0xFFFF))
        self.assertEqual(
            zip64Counts, (MAX_ZIP32_ENTRY_COUNT, MAX_ZIP32_ENTRY_COUNT))

    def test_LargeEntriesMaintainAlignment(self):
        """Tests that files that store their uncompressed sizes in the 
        local file header's extra data fields maintain the proper alignment
        mandated by the usdz spec"""

        usdzFile = "testLargeFileAlignment.usdz"

        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)

            for i in range(MAX_ZIP32_ENTRY_COUNT):
                zfw.AddFile(type(self).largeFile, f"file{i}")

        self.assertTrue(os.path.isfile(usdzFile))
        zf = Sdf.ZipFile.Open(usdzFile)

        for fileName in zf.GetFileNames():
            info = zf.GetFileInfo(fileName)
            self.assertIsNotNone(info)
            self.assertEqual(info.versionNeededToExtract, 45)
            self.assertEqual(info.dataOffset % 64, 0,
                f"Data for '{fileName}' at offset {info.dataOffset} is not "
                f"64 byte aligned")

    def test_centralDirectoryZip64ExtraFieldSize(self):
        """Tests that central directory headers are correctly written for
        records which require Zip64 extensions due to file / archive size"""

        usdzFile = "testCentralDirectoryZip64FieldSize.usdz"

        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)

            zfw.AddFile(type(self).smallFile, "small1")
            zfw.AddFile(type(self).largeFile, "large")
            zfw.AddFile(type(self).xlargeFile, "xlarge")
            zfw.AddFile(type(self).smallFile, "small2")

        self.assertTrue(os.path.isfile(usdzFile))

        headers, data = self.ReadCentralDirectoryHeaders(usdzFile)

        self.assertLess(len(headers), MAX_ZIP32_ENTRY_COUNT)

        self._AssertZip32Record(data, headers[0], "small1", 
                          os.path.getsize(type(self).smallFile))
        # file larger than the max 32 bit entry size
        self._AssertZip64Record(data, headers[1], "large", 
                          os.path.getsize(type(self).largeFile))
        # pushes the archive over the max 32 bit size
        self._AssertZip64Record(data, headers[2], "xlarge", 
                          os.path.getsize(type(self).xlargeFile))
        # offset is larger than the max 32 bit size
        self._AssertZip64Record(data, headers[3], "small2", 
                          os.path.getsize(type(self).smallFile))

    def test_SanityPythonZipfileLargeFile(self):
        """Tests that an archive created which contains a file that uses 
        Zip64 sentinel values with actual size stored in the extra fields
        correctly can be extracted by python's zip utility"""

        usdzFile = "testPythonLargeFile.usdz"

        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)
            zfw.AddFile(type(self).largeFile, "large_extracted")

        self.assertTrue(os.path.isfile(usdzFile))

        with zipfile.ZipFile(usdzFile) as zf:
            zf.extract("large_extracted")

        self.assertEqual(
            os.path.getsize(type(self).largeFile),
            os.path.getsize("large_extracted"))

    def test_SanityPythonZipfileLargeOffset(self):
        """Tests that an archive containing small files whose local file
        headers begin beyond the maxiumum 32 bit archive size, can be extracted
        by python's zip utility"""

        usdzFile = "testPythonLargeOffset.usdz"

        fileNames = ["offset_extracted_file{}".format(i) for i in range(3)]
        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)

            for fileName in fileNames:
                zfw.AddFile(type(self).mediumFile, fileName)

        self.assertTrue(os.path.isfile(usdzFile))

        with zipfile.ZipFile(usdzFile) as zf:
            for fileName in fileNames:
                zf.extract(fileName)

                self.assertEqual(
                    os.path.getsize(type(self).mediumFile),
                    os.path.getsize(fileName))

    def test_SanityPythonZipfileLotsOfEntries(self):
        """Tests that an archive created which contains file counts in
        the zip64 EndOfCentralDirectoryHeader can correctly be extracted
        by python's zip utility."""

        usdzFile = "testPythonLotsOfEntries.usdz"

        fileNames = ["entries_extracted_file{}".format(i) 
                     for i in range(MAX_ZIP32_ENTRY_COUNT)]
        with Sdf.ZipFileWriter.CreateNew(usdzFile) as zfw:
            self.assertTrue(zfw)

            for fileName in fileNames:
                zfw.AddFile(type(self).smallFile, fileName)

        self.assertTrue(os.path.isfile(usdzFile))

        with zipfile.ZipFile(usdzFile) as zf:
            self.assertEqual(len(zf.namelist()), len(fileNames))
            for fileName in fileNames:
                zf.extract(fileName)

                self.assertEqual(
                    os.path.getsize(type(self).smallFile),
                    os.path.getsize(fileName))

if __name__ == "__main__":
    unittest.main()
