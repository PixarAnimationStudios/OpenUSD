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
#
# Usage:
#   replaceSublayerSchemas source.py dest.py
#
# The command replaces the sublayer schema references so they will point
# to the proper location once installed. We are not using the python bindings
# on purpose, otherwise the script would require the libs that are just being built.
#
# Each sublayer of the format ../module_name/schema.usda is to be replaced with
# module_name/resources/schema.usda .

import sys

if len(sys.argv) != 3:
    print "Usage: %s {source.py dest.py}" % sys.argv[0]
    sys.exit(1)

with open(sys.argv[1], 'r') as s:
    with open(sys.argv[2], 'w') as d:
        import re
        pattern = re.compile(r"@.*\/([^\/]*)\/schema.usda@")
        for line in s:
            m = pattern.search(line)
            if m:
                d.write(line.replace(m.group(0), "@%s/resources/schema.usda@" % m.group(1)))
            else:
                d.write(line)
