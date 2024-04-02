#
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

import json

import pxr

from .options import Options, LogLevel

class Base:
    """Base abstract class.  Derived classes may override many of its
    methods.  At the very least, a concrete class should override
    to_usd().

    The usdotion schema defines an 'unknown' attribute which is used to
    be compatible with potential new parameters that the schema does not
    support.

    """

    def __init__(self, otio_item = None):
        """Constructor.

        Args:
        otio_item (otio.schema.*): A valid otio.schema or None
        
        """
        self.jsonData = {}
        self.otio_item = otio_item
        self.usd_prim  = None
        if otio_item:
            self.from_json_string(otio_item.to_json_string())
        
    def from_json_string(self, s):
        """Load our internal dictionary from a JSON string.

        Args:
        s (str): A valid JSON string.

        Returns:
        None

        """

        self.jsonData = json.loads(s)
        
    def to_json_string(self):
        """Returns our internal dictionary as a valid JSON string.

        Args:
        None

        Returns:
        str: A JSON string

        """

        return json.dumps(self.jsonData)

    def from_usd(self, usd_prim):
        """Convert an OTIO USD GPrim to our internal directionary.

        Args:
        usd_prim (GPrim): A valid USD GPrim.

        Returns:
        dict: A copy of the internal jsonData.

        """

        self.get_usd_attributes(usd_prim)
        return self.jsonData

    def to_usd(self, stage, usd_path):
        """Given a stage and a usd_path, subclasses should call
        _create_usd() with the actual OTIO type for the class.


        Args:
        usd_prim (GPrim): A valid USD GPrim.

        Returns:
        None

        """

        pass
    
    def set_usd_attribute(self, usd_prim, key, value):
        """Sets an attribute in the USD primitive.

        Args:
        usd_prim (GPrim): A valid USD GPrim.
        key (str): key to set the value for.
        value:     value to set, can be bool, str, or dict 

        Returns:
        None

        """

        if Options.log_level >= LogLevel.VERBOSE:
            print(f'\t\tSetting {key} = {value}')
        attr = usd_prim.GetAttribute(key)
        if isinstance(value, dict):
            value = json.dumps(value)
        elif value is None:
            value = json.dumps(value)

        try:
            attr.Set(value)
        except pxr.Tf.ErrorException:
            usd_type = usd_prim.GetTypeName()
            if Options.log_level >= LogLevel.VERBOSE:
                print(f'WARNING: Unknown attribute {key} for {usd_type} at')
                print(f'{usd_prim}')
                print(f'Valid Properties:')
                for i in usd_prim.GetPropertyNames():
                    print(f'\t{i}')
            unknown = usd_prim.GetAttribute('unknown').Get()
            if not unknown:
                unknown = '{}'
            if isinstance(unknown, str):
                unknown_dict = json.loads(unknown)
                unknown_dict[key] = value
                self.set_usd_attribute(usd_prim, 'unknown', unknown_dict)
    
    def set_usd_attributes(self, usd_prim):
        """Sets all USD attributes from a given string taking them
        from the internal dict.

        Args:
        usd_prim (GPrim): A valid USD prim.

        Returns:
        None

        """

        self.filter_attributes()
        
        if self.jsonData and len(self.jsonData) > 0:
            for key, val in self.jsonData.items():
                self.set_usd_attribute(usd_prim, key, val)
            
    def get_usd_attribute(self, usd_prim, key):
        """Get a USD attribute from a GPrim and stores it in
        our internal dictionary.

        Args:
        usd_prim (GPrim): A valid USD prim.
        key (str): Attribute name.

        Returns:
        None

        """
        
        val = usd_prim.GetAttribute(key).Get()
        self.jsonData[key] = val
        
    def get_usd_attributes(self, usd_prim):
        """Get all USD attributes from a USD GPrim.
        Note that if it there is an "unknown" attribute, it
        is expanded to new key/value pairs and deleted.

        Args:
        usd_prim (GPrim): A valid USD prim.

        Returns:
        None
        """

        attrs = usd_prim.GetPropertyNames()
        for attr in attrs:
            #
            # 'unknown' attribute is special.
            #
            if attr == 'unknown':
                continue
            self.get_usd_attribute(usd_prim, attr)


        #
        # Deal with the special unknown attr.
        #
        # We assume it is a JSON string, which we load
        # and merge with the current keys.
        #
        self.get_usd_attribute(usd_prim, 'unknown')
        unknown = self.jsonData['unknown']
        if unknown and len(unknown) > 0:
            self.jsonData.merge(json.loads(unknown))

        del self.jsonData['unknown']

        
    def filter_attributes(self):
        """Subclasses should override this method to filter the
        attributes from the interanl dictionary that should be handled
        specially, like "children" or "tracks".

        Returns:
        None

        """

        pass
    
    def _remove_keys(self, keys):
        """Remove a list of keys from our internal dictionary

        Args:
        keys (list): list of str keys.

        Returns:
        None

        """
        for key in keys:
            self.jsonData.pop(key, None)

    def _create_usd(self, stage, usd_path, usd_type):
        usd_prim = stage.DefinePrim(usd_path, usd_type)
        self.set_usd_attributes(usd_prim)
        if Options.log_level >= LogLevel.INFO:
            prim_type = usd_prim.GetTypeName()
            print(f'Created {prim_type} at {usd_path}')
        return usd_prim
