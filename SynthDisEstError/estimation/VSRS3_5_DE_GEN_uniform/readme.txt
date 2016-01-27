
This directory contains the View Synthesis Reference Software (VSRS) as part of the 
ISO/IEC JTC1/SC29 WG 11 (MPEG) 3D Video Call for Proposals


Please refer to doc/SoftwareManualVSRS.doc for compilation, usage and configuration instructions.


OpenCV library
--------------
Note that the OpenCV library is required to be able to compile the source code.
The version of OpenCV is not important, any version from 1.0 to 2.2 can be used.

OpenCV can be downloaded from:
http://sourceforge.net/projects/opencvlibrary/

Installation instructions and IDE setup can be found here:
http://opencv.willowgarage.com/wiki/InstallGuide

Instruction how to setup MS Visual Studio can be found here:
http://opencv.willowgarage.com/wiki/VisualC%2B%2B


Depending on your OpenCV version, you may need to update the MS Visual Studio project file to
set-up the correct linker dependencies.

Open the VSRS solution (e.g. VSRSVC8.sln) in Visual Studio
 In the solutions Explorer select the ViewSyn project (e.g ViewSynVC8)
 then select  Project -> Properties (or Alt-F7) -> Configuration Properties -> Linker -> Input -> Additional Dependencies:

for Opencv1.0-1.1 add following libraries:  cv.lib highgui.lib cvaux.lib cxcore.lib
for Opencv2.0     add following libraries:  cv200.lib highgui200.lib cvaux200.lib cxcore200.lib

