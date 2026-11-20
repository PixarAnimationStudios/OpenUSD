Advanced building on Mac
============================
## Table of Contents
- [Building With Build Script](#building-with-build-script)
- [Building With Python support](#building-with-python-support)
- [Building With Xcode](#building-with-xcode)
- [Debugging And Running with Xcode](#debugging-and-running-with-xcode)
- [Useful Resources](#useful-resources)

## Building With Build Script
The simplest way to build USD on macOS is by running the ```build_usd.py``` 
script. This script will download required dependencies and build 
and install them along with USD in a given directory. 
This document describes the differences in the usage of the build script on macOS.
See the generic [BUILDING.md](BUILDING.md) for a comprehensive advanced building reference.
## Building With Python Support
Building with Python support means usdview will be built and capabilities for Python bindings.
### Intel Prerequisites
Install PySide2 and PyOpenGL:
- sudo python3 -m pip upgrade
- sudo pip3 install PySide2 PyOpenGL
### Apple Silicon Prerequisites
Python3 3.9 or higher. You can see your version by executing the following in the terminal:
```
> python3 --version
```
If it's less than 3.9, you can visit www.python3.org and install a newer distro.
Install PySide6 and PyOpenGL.
- sudo python3 -m pip upgrade
- sudo pip3 install PySide6 PyOpenGL
### For a simple build with Python
In a terminal, run ```xcode-select``` to ensure command line developer tools are 
installed. Then run the script.
For example, the following will download, build, and install USD's dependencies,
then build and install USD into ```/opt/local/USD```.
```
> python3 USD/build_scripts/build_usd.py /opt/local/USD
```
### No Python
To disable python and usdview support, you can run with --no-python and you won't need PySide or PyOpenGL
```
> python3 USD/build_scripts/build_usd.py --no-python /opt/local/USD
```
## Building with Xcode
To debug, frame capture, and perform common developer tasks, it may be useful to build for Xcode

To build for Xcode, specify the Xcode generator from the `build_usd.py` script.
<br>Take note, in order to build for Xcode in 22.08 release of USD, we need to integrate [PR#1959](https://github.com/PixarAnimationStudios/USD/pull/1959) in our source

Add `--generator Xcode` to the build command, case-sensitive. For example:
```
> python USD/build_scripts/build_usd.py --generator Xcode /opt/local/USD
```

Build from Xcode:
Just open up the Xcode project: `/%RepoDirName%/build/%RepoDirName%/usd.xcodeproj` <br>
and build as usual using the `ALL_BUILD` scheme.

## Debugging And Running with Xcode
For debugging, add `--build-variant debug` to the `build_usd.py` options:
```
> python USD/build_scripts/build_usd.py --generator Xcode --build-variant debug /opt/local/USD
```
### Scheme Setup
Once the Xcode project has been generated, select the `ALL_BUILD` target and open the `Edit Scheme...` dialog for it.
1. In the Info panel: 
    Set Python.app as the executable:
    - To find it, run in terminal: 
    ```
    > python3 -c 'import sys; print(sys.path)'
    ```
    - Take note of the path to your python.framework and its version
    - The path to your Python.app should then look something like this:
      - `%prefix%/Library/Frameworks/Python.framework/Versions/%version%/Resoucres/Python.app`
      - where `%prefix%` is the prefix on your machine and `%version%` is the version on your machine found with earlier command
2. In the Arguments panel:
    - Arguments to pass on Launch:
        - `%BuildDir%/bin/usdview`, where `%BuildDir%` is the location where you built USD
        - Path to a valid *.usda, *.usd file.
    - Environment Variables:
        - `PYTHONPATH %BuildDir%/lib/python`
        - `PXR_SHARED_LIBRARY_LOCATION %BuildDir%/lib`
3. In the Options panel:
    - Use custom working directory: Enabled
    - Set to: `%BuildDir%/bin`
    - GPU Frame Capture: Metal
    - Document Versions: Off
### Adding Debug Permissions to python.app
Python.app does not have get-allow entitlements, meaning by default it can't be hooked into by a debugger.
There are several ways to work around that, including: codesigning python.app yourself, disabling SIP, or building your own Python distro.
Code signing python.app yourself may impact your machine the least. To do that, you have to have a code signing certificate. To list your existing ones you can execute
```
> security find-identity -v -p codesigning
```
Then codesign python.app with your codesign identity (use the one listed from the command above).
An example certificate id can be `3U8A612C8480FAC679247D6E5019EAE9BAB3E825`.
Then to codesign python.app (we located it above) you can execute:
```
> codesign --force --sign 3U8A612C8480FAC679247D6E5019EAE9BAB3E825 %Prefix%/Library/Frameworks/Python.framework/Versions/%version%/Resources/Python.app
```
Now you should be able to debug usdview inside Xcode
## Useful Resources
- [How to do a frame capture](https://developer.apple.com/documentation/metal/debugging_tools/enabling_frame_capture?language=objc)
- [How to use Instruments](https://developer.apple.com/documentation/metal/performance_tuning/using_metal_system_trace_in_instruments_to_profile_your_app?language=objc)
- [How to perform code signing Tasks](https://developer.apple.com/support/code-signing/)