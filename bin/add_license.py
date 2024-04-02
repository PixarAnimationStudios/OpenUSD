#!/usr/bin/env python
#
# Copyright 2024 Gonzalo Garramuño for Signly
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
# This script adds licensing information to all .py
#
# You must run it from the root of the OpenUSD project.
#

import os
import glob
import re
import sys
import stat
import shutil

PYTHON_DIRS = [
    'pxr/usd/bin/usdotio/usdOtio/python'
]

def process_python_files():
  for python_dir in PYTHON_DIRS:
      python_files = glob.glob( python_dir + "/*.py" )

      files = python_files

      for f in files:
            with open( f, encoding='utf-8' ) as ip:
                text = ip.read()
            with open( f + ".new", "w", encoding='utf-8' ) as out:

                if not re.search( "Copyright", text ):
                    license = """# Copyright 2024 Gonzalo Garramuño for Signly
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
"""
                    print("Adding copyright to",f)
                    text = license + text

                out.write( text )

            shutil.move( f + ".new", f )
 



process_python_files()
