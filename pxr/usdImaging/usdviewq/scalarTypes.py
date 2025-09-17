#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
def GetScalarTypeFromAttr(attr):
    '''
    returns the (scalar, isArray) where isArray is True if it was an array type
    '''
    # Usd.Attribute and customAttributes.CustomAttribute have a
    # GetTypeName function, while Sdf.AttributeSpec has a typeName attr.
    if hasattr(attr, 'GetTypeName'):
        typeName = attr.GetTypeName()
    elif hasattr(attr, 'typeName'):
        typeName = attr.typeName
    else:
        typeName = ""

    from pxr import Sdf
    if isinstance(typeName, Sdf.ValueTypeName):
        return typeName.scalarType, typeName.isArray
    else:
        return None, False

_toStringFnCache = {}

def ToString(v, valueType=None):
    """Returns a string representing a "detailed view" of the value v.
    This string is used in the watch window"""

    from pxr import Tf, Gf
    global _toStringFnCache
    
    # Check cache.
    t = type(v)
    cacheKey = (t, valueType)
    fn = _toStringFnCache.get(cacheKey)
    if fn:
        return fn(v)

    # Cache miss. Compute string typeName for the given value
    # using the given valueType as a hint.
    typeName = ""

    from pxr import Sdf
    if isinstance(valueType, Sdf.ValueTypeName):
        tfType = valueType.type
    else:
        tfType = Tf.Type.Find(t)

    if tfType != Tf.Type.Unknown:
        typeName = tfType.typeName

    # Pretty-print "None"
    if v is None:
        fn = lambda _: 'None'

    # Pretty-print a bounding box
    elif isinstance(v, Gf.BBox3d):
        def bboxToString(v):
            prettyMatrix = (
                "%s\n%s\n%s\n%s" %
                (v.matrix[0], v.matrix[1],
                 v.matrix[2], v.matrix[3])).replace("(","").replace(")","")
            result = ("Endpts of box diagonal:\n"
                      "%s\n%s\n\nTransform matrix:\n%s\n"
                      % (v.box.GetCorner(0), v.box.GetCorner(7), prettyMatrix))
            if (v.hasZeroAreaPrimitives):
                result += "\nHas zero-area primitives\n"
            else:
                result += "\nDoes not have zero-area primitives\n"
            worldSpaceRange = Gf.Range3d()
            worldSpaceRange.UnionWith(v.matrix.Transform(v.GetRange().GetMin()))
            worldSpaceRange.UnionWith(v.matrix.Transform(v.GetRange().GetMax()))
            result += "\nWorld-space range:\n%s\n%s\n" % \
                      (worldSpaceRange.GetMin(), worldSpaceRange.GetMax())
            return result
        fn = lambda b: bboxToString(b)

    # Pretty-print a GfMatrix*
    elif typeName.startswith("GfMatrix"):
        def matrixToString(v):
            result = ""
            numRows = int(typeName[8])
            for i in range(numRows):
                result += str(v[i]) + "\n"
            result = result.replace("(","").replace(")","")
            return result
        fn = lambda m: matrixToString(m)

    # Pretty-print a GfVec* or a GfRange*
    elif typeName.startswith("GfVec") or typeName.startswith("GfRange"):
        fn = lambda v: str(v)
        
    # pretty print an int
    elif isinstance(v, int):
        fn = lambda i: "{:,d}".format(i)

    # pretty print a float
    elif isinstance(v, float):
        fn = lambda f: "{:,.6f}".format(f)

    # print a string as-is
    elif isinstance(v, str):
        fn = lambda s: s

    else:
        import pprint
        fn = lambda v: pprint.pformat(v)

    # Populate cache and invoke function to produce the string.
    _toStringFnCache[cacheKey] = fn
    return fn(v)

def ToClipboard(v, typeName=None):

    # XXX: we can have this give you the repr
    return ToString(v, typeName)

