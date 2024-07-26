# Copyright 2024 Gonzalo Garramuño for Signly, Ltd.
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


def get_optional_timecode(usd_prim, usd_attr):
    attr = usd_prim.GetAttribute(usd_attr)
    if not attr:
        return None
    attr = attr.Get()
    if not attr:
        return None
    return attr.GetValue()

def get_timecode(usd_prim, usd_attr):
    attr = usd_prim.GetAttribute(usd_attr)
    if not attr:
        print(f'{usd_prim} Invalid timecode attribute "{usd_attr}"')
        return None
    attr = attr.Get()
    if not attr:
        print(f'{usd_prim} Invalid timecode "{usd_attr}" read attribute "{attr}"')
        return 0.0
    return attr.GetValue()


def get_double(usd_prim, usd_attr):
    attr = usd_prim.GetAttribute(usd_attr)
    if not attr:
        print(f'{usd_prim} Invalid double attribute "{usd_attr}"')
        return 0.0
    return attr.Get()

def get_asset(usd_prim, usd_attr):
    attr = usd_prim.GetAttribute(usd_attr)
    if not attr:
        print(f'{usd_prim} Invalid asset attribute "{usd_attr}"')
        return None
    attr = attr.Get()
    if not attr:
        print(f'{usd_prim} Invalid read asset "usd_attr" attribute "{attr}"')
        return None
    value = attr.path
    return value

def get_string(usd_prim, usd_attr):
    attr = usd_prim.GetAttribute(usd_attr)
    if not attr:
        print(f'{usd_prim} Invalid string attribute "{usd_attr}"')
        return None
    return attr.Get()

#
# Temporary comparison function as we don't load the omni libraries.
#
def is_a(usd_prim, usd_type):
    usd_type_name = usd_prim.GetTypeName()
    return usd_type_name == usd_type
