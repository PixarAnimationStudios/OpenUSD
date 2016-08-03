#!/pxrpythonsubst

import shutil, os

from pxr import Sdf, Usd, Tf

import Mentor.Runtime
from Mentor.Runtime import (AssertEqual, AssertTrue, AssertFalse,
                            FindDataFile, ExpectedErrors, RequiredException)

def DiffLayers(layer1, layer2):
    AssertEqual(layer1.ExportToString(), layer2.ExportToString())

############################################################
# Utility functions for determining the underlying file format of
# a .usd layer.
#
# XXX: These all rely on the given layer being saved on disk. WBN and would
#      simplify some test code a bit if that weren't the case.

def VerifyUsdFileFormat(usdFileName, underlyingFormatId):
    """In order to verify the underlying file format of the given .usd file,
    we're going to try to load the file but use an extension override to force
    the use of the expected underlying file format. If the file fails to load,
    we'll know it wasn't the right format.
    """

    # Copy the file to a temporary location and replace the extension with
    # the expected format, then try to open it.
    tmpFileName = '/tmp/%s.%s' % \
        (os.path.splitext(os.path.basename(usdFileName))[0], underlyingFormatId)

    shutil.copyfile(usdFileName, tmpFileName)

    AssertFalse(Sdf.Layer.Find(tmpFileName))
    AssertFalse(Sdf.Layer.Find(usdFileName))
    AssertTrue(Sdf.Layer.FindOrOpen(tmpFileName))

    os.unlink(tmpFileName)

def VerifyUsdFileIsASCII(usdFileName):
    VerifyUsdFileFormat(usdFileName, 'usda')

def VerifyUsdFileIsBinary(usdFileName):
    VerifyUsdFileFormat(usdFileName, 'usdb')

def GetUnderlyingUsdFileFormat(layer):
    if Sdf.FileFormat.FindById('usda').CanRead(layer.identifier):
        return 'usda'
    elif Sdf.FileFormat.FindById('usdc').CanRead(layer.identifier):
        return 'usdc'
    elif Sdf.FileFormat.FindById('usdb').CanRead(layer.identifier):
        return 'usdb'
    return ''

############################################################

def TestNewUsdLayer():
    def _TestNewLayer(layer, expectedFileFormat):
        """Check that the new layer was created successfully and its file
        format is what we expect."""
        AssertTrue(layer)
        AssertEqual(layer.GetFileFormat(), Sdf.FileFormat.FindById('usd'))

        # Write something to the layer to ensure it's functional.
        primSpec = Sdf.PrimSpec(layer, 'Scope', Sdf.SpecifierDef, 'Scope')
        AssertTrue(primSpec)

        # Ensure the layer is saved to disk, then check that its
        # underlying file format matches what we expect.
        AssertTrue(layer.Save())
        AssertEqual(GetUnderlyingUsdFileFormat(layer), expectedFileFormat)

    usdFileFormat = Sdf.FileFormat.FindById('usd')

    # Newly-created .usd layers default to the USD_DEFAULT_FILE_FORMAT format.
    _TestNewLayer(Sdf.Layer.CreateNew('testNewUsdLayer.usd'),
                  Tf.GetEnvSetting('USD_DEFAULT_FILE_FORMAT'))
    _TestNewLayer(Sdf.Layer.New(usdFileFormat, 'testNewUsdLayer_2.usd'),
                  Tf.GetEnvSetting('USD_DEFAULT_FILE_FORMAT'))

    # Verify that file format arguments can be used to control the
    # underlying file format for new .usd layers.
    _TestNewLayer(Sdf.Layer.CreateNew('testNewUsdLayer_text.usd',
                                     args={'format':'usda'}), 'usda')
    _TestNewLayer(Sdf.Layer.New(usdFileFormat, 'testNewUsdLayer_text_2.usd',
                               args={'format':'usda'}), 'usda')

    _TestNewLayer(Sdf.Layer.CreateNew('testNewUsdLayer_binary.usd',
                                     args={'format':'usdb'}), 'usdb')
    _TestNewLayer(Sdf.Layer.New(usdFileFormat, 'testNewUsdLayer_binary_2.usd',
                               args={'format':'usdb'}), 'usdb')

    _TestNewLayer(Sdf.Layer.CreateNew('testNewUsdLayer_crate.usd',
                                     args={'format':'usdc'}), 'usdc')
    _TestNewLayer(Sdf.Layer.New(usdFileFormat, 'testNewUsdLayer_crate_2.usd',
                               args={'format':'usdc'}), 'usdc')
    
def TestUsdLayerSave():
    # Verify that opening a .usd file, authoring changes, and saving it
    # maintains the same underlying file format.
    def _TestLayerOpenAndSave(srcFilename, expectedFileFormat):
        dstFilename = 'testUsdLayerSave_%s.usd' % expectedFileFormat
        shutil.copyfile(srcFilename, dstFilename)
        layer = Sdf.Layer.FindOrOpen(dstFilename)
        AssertEqual(GetUnderlyingUsdFileFormat(layer), expectedFileFormat)

        Sdf.PrimSpec(layer, 'TestUsdLayerSave', Sdf.SpecifierDef)
        AssertTrue(layer.Save())

        AssertEqual(GetUnderlyingUsdFileFormat(layer), expectedFileFormat)

    asciiFile = FindDataFile('testUsdFileFormats/ascii.usd')
    _TestLayerOpenAndSave(asciiFile, 'usda')

    binaryFile = FindDataFile('testUsdFileFormats/binary.usd')
    _TestLayerOpenAndSave(binaryFile, 'usdb')

    crateFile = FindDataFile('testUsdFileFormats/crate.usd')
    _TestLayerOpenAndSave(crateFile, 'usdc')

def TestUsdLayerExport():
    def _TestExport(layer):
        AssertTrue(layer)

        exportedLayerId = os.path.basename(
            layer.identifier).replace('.usd', '_exported.usdc')
        AssertTrue(layer.Export(exportedLayerId))

        exportedLayer = Sdf.Layer.FindOrOpen(exportedLayerId)
        AssertTrue(exportedLayer)

        # Exporting to a new .usd file should produce a binary file,
        # since that's the default format for .usd.
        AssertEqual(GetUnderlyingUsdFileFormat(exportedLayer), "usdc")

        # The contents of the exported layer should equal to the
        # contents of the original layer.
        DiffLayers(layer, exportedLayer)

    # Write some content to the layer for testing purposes.
    newLayer = Sdf.Layer.CreateNew('testUsdLayerExport.usd')
    primSpec = Sdf.PrimSpec(newLayer, 'Scope', Sdf.SpecifierDef, 'Scope')
    newLayer.Save()
    _TestExport(newLayer)

    asciiFile = FindDataFile('testUsdFileFormats/ascii.usd')
    _TestExport(Sdf.Layer.FindOrOpen(asciiFile))

    binaryFile = FindDataFile('testUsdFileFormats/binary.usd')
    _TestExport(Sdf.Layer.FindOrOpen(binaryFile))

    print 'doing FindOrOpen on crateFile'
    crateFile = FindDataFile('testUsdFileFormats/crate.usd')
    _TestExport(Sdf.Layer.FindOrOpen(crateFile))

def TestLoadASCIIUsdLayer():
    asciiFile = FindDataFile('testUsdFileFormats/ascii.usd')
    VerifyUsdFileIsASCII(asciiFile)

    l = Sdf.Layer.FindOrOpen(asciiFile)
    AssertTrue(l)

def TestLoadBinaryUsdLayer():
    binaryFile = FindDataFile('testUsdFileFormats/binary.usd')
    VerifyUsdFileIsBinary(binaryFile)

    l = Sdf.Layer.FindOrOpen(binaryFile)
    AssertTrue(l)

def TestLoadCrateUsdLayer():
    crateFile = FindDataFile('testUsdFileFormats/crate.usd')
    VerifyUsdFileIsCrate(crateFile)

    l = Sdf.Layer.FindOrOpen(crateFile)
    AssertTrue(l)

def TestUsdLayerString():
    # Verify that we get non-empty string representations for binary, text, and
    # crate .usd layers.
    asciiFile = FindDataFile('testUsdFileFormats/ascii.usd')
    asciiLayer = Sdf.Layer.FindOrOpen(asciiFile)
    asciiLayerAsString = asciiLayer.ExportToString()
    AssertTrue(len(asciiLayerAsString) > 0)

    binaryFile = FindDataFile('testUsdFileFormats/binary.usd')
    binaryLayer = Sdf.Layer.FindOrOpen(binaryFile)
    binaryLayerAsString = binaryLayer.ExportToString()
    AssertTrue(len(binaryLayerAsString) > 0)

    crateFile = FindDataFile('testUsdFileFormats/crate.usd')
    crateLayer = Sdf.Layer.FindOrOpen(crateFile)
    crateLayerAsString = crateLayer.ExportToString()
    AssertTrue(len(crateLayerAsString) > 0)

    # All .usd layers use the same string representation as a text .usd layer,
    # so these should match.
    AssertEqual(asciiLayerAsString, binaryLayerAsString)
    AssertEqual(binaryLayerAsString, crateLayerAsString)
    AssertEqual(crateLayerAsString, asciiLayerAsString)

    # Verify that we can import the string representation of a .usd
    # layer into another .usd layer.
    importLayer = Sdf.Layer.CreateNew('testUsdLayerString.usdb')
    AssertTrue(importLayer.ImportFromString(asciiLayerAsString))
    DiffLayers(asciiLayer, importLayer)

    importLayer = Sdf.Layer.CreateNew('testUsdLayerString.usdc')
    AssertTrue(importLayer.ImportFromString(asciiLayerAsString))
    DiffLayers(asciiLayer, importLayer)

def TestBadFileName():
    """Previously, failing to open a layer would assert-fail in C++/Usd, this
    test makes sure it is a recoverable error."""

    with RequiredException(Tf.ErrorException):
        stage = Usd.Stage.Open("badFileName.usd")

def TestUsdcRepeatedSaves():
    """Repeated saves used to fail due to an fopen mode bug."""
    s = Usd.Stage.CreateNew('testUsdcRepeatedSaves.usdc')
    s.DefinePrim('/foo')
    s.GetRootLayer().Save()
    s.DefinePrim('/foo/bar/baz')
    assert s.GetPrimAtPath('/foo')
    assert s.GetPrimAtPath('/foo/bar')
    assert s.GetPrimAtPath('/foo/bar/baz')
    s.GetRootLayer().Save()
    assert s.GetPrimAtPath('/foo')
    assert s.GetPrimAtPath('/foo/bar')
    assert s.GetPrimAtPath('/foo/bar/baz')

if __name__ == "__main__":
    Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)

    TestNewUsdLayer()
    TestUsdLayerSave()
    TestUsdLayerExport()
    TestUsdLayerString()
    TestLoadASCIIUsdLayer()
    TestLoadBinaryUsdLayer()
    TestBadFileName()
    TestUsdcRepeatedSaves()

    Mentor.Runtime.ExitTest()

