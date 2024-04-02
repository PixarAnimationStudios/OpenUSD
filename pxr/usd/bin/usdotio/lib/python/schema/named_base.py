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

from .base       import Base
from .options    import Options, LogLevel

class NamedBase(Base):

    """Base abstract class that handles metadata.  
       Derived classes may override many of its methods.
  
       At the very least, a concrete class should override the
       constructor to create a new otio_item with default parameters
       and to_usd() to create the actual USD primitive.

    """
    
    FILTER_KEYS = [
        'metadata',
    ]
    
    def __init__(self, otio_item = None):
        """Constructor.
        This abstract class handles all classes that have a name and a
        metadata.

        Args:
        otio_item (type): Optional otio.schema.* type

        """

        super().__init__(otio_item)
        self.metadata = json.dumps(self.jsonData.get('metadata', '{}'))
        if otio_item:
            self.name = otio_item.name
        else:
            self.name = self.__class__.__name__

    def filter_attributes(self):
        """Filter the attributes for this abstract class and its children
        classes.

        Args:
        None

        Returns:
        None

        """

        super().filter_attributes()
        self._remove_keys(NamedBase.FILTER_KEYS)

    def set_usd_attributes(self, usd_prim):
        """Sets the attributes for this abstract class and its

        Args:
        usd_prim (GPrim): A valid USD primitive.

        Returns:
        None

        """

        self.set_usd_attribute(usd_prim, 'metadata', self.metadata)
        
        super().set_usd_attributes(usd_prim)

    def get_usd_attributes(self, usd_prim):
        """Gets the special USD attributes for a GPrim.

        Currently, it handles dealing with "metadata" properly.

        Args:
        usd_prim (GPrim): A valid USD primitive.

        Returns:
        None

        """

        super().get_usd_attributes(usd_prim)

        # Convert the metadata string into an actual dict
        metadata = self.jsonData.get('metadata')
        if metadata:
            try:
                self.jsonData['metadata'] = json.loads(metadata)
            except json.JSONDecodeError as e:
                # Handle the error if JSON decoding fails
                print(f"Error decoding JSON: {e}")
                self.jsonData['metadata'] = {}
        else:
            self.jsonData['metadata'] = {}
