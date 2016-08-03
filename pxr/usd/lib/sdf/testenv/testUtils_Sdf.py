import sys, os
from Mentor.Runtime import Assert, AssertEqual

from pxr import Sdf

__all__ = [
    'verbose',
    'superVerbose',
    'removeFiles',
    'doMenvaFileIOCheck',
    'PathElemsToPrefixes',
    ]

# Set up a global variables verbose and superVerbose based on the presence of -v or -vv...
# uses this in a lot of tests
verbose = False
superVerbose = False

if len(sys.argv) > 1:
    if sys.argv[1] == "-v":
        verbose = True
        superVerbose = False
    elif sys.argv[1] == "-vv":
        verbose = True
        superVerbose = True


def removeFiles(*filenames):
    """Removes the given files (if one of the args is itself a tuple or list, "unwrap" it.)"""
    for f in filenames:
        if isinstance(f, (tuple, list)):
            removeFiles(*f)
        else:
            try:
                os.remove(f)
            except OSError:
                pass



def doMenvaFileIOCheck(layer, layerFilename, rewriteFilename):
    """Writes out the given layer (or csd) to a layerFilename, reads it back in to a new layer, writes the new layer out again to rewriteFilename, and compares the two files.  For this function to work properly, layerFilename and rewriteFilename must be different paths where it is possible to write.  They must also both be different from the fileName/realPath of the layer to avoid instance uniquing getting in the way of actual file parsing."""
    
    if (layerFilename == rewriteFilename):
        print "WARNING: doMenvaFileIOCheck: rewriteFilename should not be the same as layerFilename or this test function won't work correctly... fixing up the name to be different, but you should change the test code."
        rewriteFilename = rewriteFilename + "REWRITE.menva"
    if Sdf.Find(layerFilename):
        print "WARNING: doMenvaFileIOCheck: layerFilename should not be the same as the actual fileName of the layer or this test function won't work correctly... fixing up the name to be different, but you should change the test code."
        layerFilename = layerFilename + "DISAMBIG.menva"
    if Sdf.Find(rewriteFilename):
        print "WARNING: doMenvaFileIOCheck: rewriteFilename should not be the same as the actual fileName of the layer or this test function won't work correctly... fixing up the name to be different, but you should change the test code."
        rewriteFilename = rewriteFilename + "DISAMBIG.menva"
        
    # Remove stale files
    removeFiles(layerFilename, rewriteFilename)
    
    # Write out layer
    Assert(layer.Export(layerFilename), "Failed to write %s"%layer)
    
    # Load and rewrite layer
    rereadLayer = Sdf.Layer.FindOrOpen(layerFilename)
    Assert(rereadLayer, "Failed to reload layer")
    
    Assert(rereadLayer.Export(rewriteFilename), "Failed to rewrite %s"%rereadLayer)

    # Compare files.
    fd = open(layerFilename, "r")
    layerData1 = fd.readlines()
    fd.close()
    fd = open(rewriteFilename, "r")
    layerData2 = fd.readlines()
    fd.close()

    if layerData1 != layerData2:
        print "ERROR: '%s' and '%s' are different.\n\n"%(layerFilename, rewriteFilename)
        os.system("/usr/bin/diff %s %s"%(layerFilename, rewriteFilename))
        AssertEqual(layerData1, layerData2)
    
    # Remove files after check if we're not running verbose
    if not verbose:
        removeFiles(layerFilename, rewriteFilename)

    if verbose:
        print "File I/O looks good for %s."%layer

def PathElemsToPrefixes(absolute, elements):
    if absolute:
        string = "/";
    else:
        string = ""
    
    lastElemWasDotDot = False
    didFirst = False
    
    for elem in elements:
        if elem == Sdf.Path.parentPathElement:
            # dotdot
            if didFirst:
                string = string + "/"
            else:
                didFirst = True
            string = string + elem
            lastElemWasDotDot = True
        elif elem[0] == ".":
            # property
            if lastElemWasDotDot:
                string = string + "/"
            string = string + elem
            lastElemWasDotDot = False
        elif elem[0] == "[":
            # rel attr or sub-attr indices, don't care which
            string = string + elem
            lastElemWasDotDot = False
        else:
            if didFirst:
                string = string + "/"
            else:
                didFirst = True
            string = string + elem
            lastElemWasDotDot = False
    if not string:
        return []
    path = Sdf.Path(string)
    return path.GetPrefixes()
