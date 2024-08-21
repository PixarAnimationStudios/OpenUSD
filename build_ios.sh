#!/bin/bash

# Generates and builds USD for iOS using Xcode as the generator.
# To get the values for 'CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM' and 'CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY' open already signed Xcode project's 'project.pbxproj; using text editor and look for those. 

set -e

clear
rm -rf _build
python3 ./build_scripts/build_usd.py --build-target "iOS" --generator "Xcode" --cmake-build-args "\-DXCODE_ATTRIBUTE_CODE_SIGN_STYLE=\"Automatic\" \-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=\"M369BC24W9\" \-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY=\"Apple Development\"" --verbose --build-monolithic --usd-imaging --no-materialx --no-usdview --no-examples --no-tutorials --no-tools --no-python _build