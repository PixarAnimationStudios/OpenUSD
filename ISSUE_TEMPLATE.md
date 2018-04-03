### Description of Issue
UsdGeomNurbsCurves doesn't have notion of "form", which could be "open", "close" or "periodic". And whenever importing usd nurbsCurve into Maya, it always create "open" curve, which causes bugs.
### Steps to Reproduce
1. Load pxrUsd plugin
2. Export a closed nurbsCurve to a usd file
3. Import that usd file into Maya
4. The nurbsCurve is not displayed correctly in Maya viewport
### System Information (OS, Hardware)
Windows 7(but probably on any platform)
### Package Versions
0.8.4 and dev branch
### Build Flags
--maya
