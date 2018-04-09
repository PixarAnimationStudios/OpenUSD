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
def GetScalarTypeFromAttr(attr):
    """
    returns the (scalar, isArray) where isArray is True if it was an array type
    """
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

def ToString(v, typeName=None):
    """Returns a string representing a "detailed view" of the value v.
    This string is used in the watch window"""

    from pxr import Tf, Gf
    global _toStringFnCache
    
    # Check cache.
    t = type(v)
    fn = _toStringFnCache.get(t)
    if fn:
        return fn(v)

    if not typeName:
        typeName = str(v.__class__)

    if typeName:
        from pxr import Sdf
        if isinstance(typeName, Sdf.ValueTypeName):
            tfType = typeName.type
        else:
            tfType = Sdf.GetTypeForValueTypeName(typeName)
        if tfType != Tf.Type.Unknown:
            typeName = tfType.typeName

    # Cache miss.
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

    # Pretty-print a GfVec*
    elif typeName.startswith("GfVec"):
        fn = lambda v: str(v)

    # Pretty-print a TfTimeStamp
    elif isinstance(v, Tf.TimeStamp):
        def timeStampToString(v):
            from datetime import datetime
            dt = datetime.fromtimestamp(v.Get())
            return dt.isoformat(' ')
        fn = lambda t: timeStampToString(t)
        
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
    _toStringFnCache[t] = fn
    return fn(v)

def ToClipboard(v, typeName=None):

    # XXX: we can have this give you the repr
    return ToString(v, typeName)

