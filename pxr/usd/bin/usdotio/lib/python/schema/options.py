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

from enum import Enum, auto

class LogLevel(Enum):
    """Enum class defining the verbose mode of the script.
    """

    QUIET   = auto()   # Completely silent.  Overwrites files without asking.
    NORMAL  = auto()   # Print basic info and check for overwriting files
    INFO    = auto()   # Basic information.
    VERBOSE = auto()   # More detailed information.
    TRACE   = auto()   # Lots of messages
    DEBUG   = auto()   # Debug mode.  Lots of messages and, if using 'save'
                       # mode, it will *NOT* use opentimelineio to save the
                       # file.  Useful for when the otio cannot parse the
                       # timeline
    
    def __ge__(self, other):
        """Greater or equal comparison operator.

        Args:
        other (a LogLevel or an int): Another LogLevel or an int.

        Returns:
        bool: Whether self is higher than other

        """

        if isinstance(other, self.__class__):
            return self.value >= other.value
        elif isinstance(other, int):
            return self.value >= other
        return False  # Handle non-enum comparison

class Options:
    """usdotio global Options class.
    """
    
    #
    # When this is True, we skip all interactive questions in the script 
    #
    yes = False

    #
    #
    #
    log_level = LogLevel.NORMAL

    @staticmethod
    def continue_prompt():
        """
        Prompt user to continue or cancel, unless the -yes flag was passed in.
        """
        if Options.yes:
            return
        response = input("\nShall I continue (y/n)? ")
        if response.lower() == 'y':
            return
        elif response.lower() == 'n':
            print('Aborting...')
            exit(1)
        else:
            print("Invalid input. Please enter 'y' or 'n'.")
            Options.continue_prompt()
