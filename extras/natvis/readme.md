# Visual Studio Native Visualization for USD
Natvis files create custom views of objects in the C++ debugger, for example a string instead of a memory pointer. See [here](https://learn.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects?view=vs-2022) for more details.

## How to Use
Download the file(s) for the version(s) you are interested in. After downloading, right click > Properties, check Unblock, OK.
Copy the file to C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Packages\Debugger\Visualizers - adjust as needed if you installed Visual Studio elsewhere.

## Multiple Versions
Sometimes a developer needs to work with multiple versions of USD - for example one in the production code of the main app, and another version in some development branch. For this reason, we want to keep distinct files for each USD version.
* When improving an existing version, update the existing file.
* Create new files for a new USd versions, or if making breaking changes.

NOTE: OpenUSD uses versions in its namespaces, e.g. `pxrInternal_v0_23` for version 23.
To make a natvis file for a new version start by renaming all the namespaces as appropriate, then check if other changes are needed too.


# Contributing

## Natvis reference
Microsoft has an [online resource](https://learn.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects) that describes the natvis file format.
Some other resources: https://www.cppstories.com/2021/natvis-tutorial/
The other `*.natvis` files provided by Microsoft in your visual studio directory will also be useful as examples.

## Editing
Editing can be done in any text editor.

## Efficient development iteration
Natvis files get (re)loaded on every debug run. But that can get tedious.
A convenient approach to test changes is to have the debugger stopped at a callstack where variables of the class one is trying to support are available in a watch window (e.g., the `Locals` or `Auto` watch windows).
After making edits to the `*.natvis` file one can issue the `.natvisreload` command in the immediate window to reload all `*.natvis` files.
Switching back to the watch window will reveal the effect.
Repeat as needed.

## Debugging / syntax checking
While doing this it is a good idea to enable `Natvis diagnostic messages` by setting that value in `Tools -> Options -> Debugging -> Output Window` to `Error` or `Verbose`.
That will cause diagnostic messages to appear in the `Output` window. Note that visualizations are only used when needed (when trying to display a variable of the type).
