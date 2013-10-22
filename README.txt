Freiburg, 26th of September, 2011
UPDATE: 10th of May, 2013 

[W]ASABI [A]ffect [S]imulation [A]rchitecture for [B]elievable [I]nteractivity
GUI part version 0.7
Copyright (C) 2011 Christian Becker-Asano.
All rights reserved.

qwt as submodule: http://stackoverflow.com/a/465151

Contact: Christian Becker-Asano (christian (at) becker-asano (dot) de)

1a. Setup

As of July 2013, this program can be compiled as using "Qt Creator" version 2.7.2 available here: 
http://qt-project.org/downloads
Internally it uses MinGW gcc 4.8.0 for Qt5.1.0 (under MS Windows 7). 
Older versions of Qt shipping with older versions of the Qt Creator might work as well, but lately I ran into serious trouble with the Nokia SDK version. Thus, I switched to the qt-project.org version of Qt5.

It depends on Qt, OpenGL, QWT (http://qwt.sourceforge.net/), and the WASABIEngine as dynamically linked libraries. The first two come with the Qt SDK. The last two are integrated as submodules.

After a fresh clone of this project or after the first "Pull" that updates your local repo to the submodule version, you will only have the empty subdirectories "WASABIEngine" and "WASABI-qwt-clone" and compiling will fail. Thus, perform:
$ git submodule update --init "WASABIEngine"
$ git submodule update --init "WASABI-qwt-clone"

1b. Compiling

-- Win7 --

OLD instructions for the Nokia SDK version of Qt4: 
===
IMPORTANT: Please make sure to change the setting of every project to use the "MinGW (32bit)" toolchain and not the option "MinGW as a gcc for Windows targets" under Projects->Goals->Build configurations->Toolchains". In addition, currently only the RELEASE configuration seems to work. So, please also change to "release" configuration in each project.
Then proceed as follows:
A1. open the file "\WASABI-qwt-clone\qwt-code\qwt\qwt.pro" in QtCreator not using "shadow build".
A2. change the Toolchain setting as explained above for both the debug and release configuration.
A3. change to the release configuration (left panel button)
A4. "build" the project (Ctrl-B). This will take some time.
B1. open the file "\WASABIEngine\WASABIEngine.pro" in QtCreator not using "shadow build".
B2. change the Toolchain setting as explained above for both the debug and release configuration.
B3. change to the release configuration (left panel button)
B4. "build" the project (Ctrl-B). This won't take too long.
C1. set the project "WASABIGuiQt" to be the "active project" (right-click menu)
C2. change the Toolchain setting as explained above for both the debug and release configuration.
C3. change to the release configuration (left panel button)
C4. press the play button to compile and run.
===

NEW instructions for the qt-project.org-Version of Qt5 (UPDATED 22nd of October, 2013):
===
A1. Open the file "WASABIGuiQt.pro" by double clicking on it.
A2. use the default configuration for "Desktop Qt 5.1.0 MinGW 32bit" with two seperate directories for "Debug" and "Release".
    That looks something like "../build-WASABIGuiQt-Desktop_Qt_5_1_0_MinGW_32bit-Debug" and "../build-WASABIGuiQt-Desktop_Qt_5_1_0_MinGW_32bit-Release" for me.
A3. Try to compile in Debug mode and you will get a message like "cannot find -lWASABIEngine".
B1. Open the file "WASABIEngine/WASABIEngine.pro".
B2. Leave the build-paths as is. In my case it looks like
    <path\to\git-working-dir>\WASABIQtGui\build-WASABIEngine-Desktop_Qt_5_1_1_MinGW_32bit-[Debug/Release].
B3. Change to "Debug" mode on the left panel (the Computer screen button), and hit "build" (the hammer button).
	This should result in the file "../build-WASABIEngine-Desktop_Qt_5_1_1_MinGW_32bit-Debug/debug/WASABIEngine.dll".
	Similarly, you may try to build the release version.
C1. Open the file "WASABI-qwt-clone/qwt-code/qwt/qwt.pro", 
C2. IMPORTANT: Change both build directories for Debug and Release to "WASABI-qwt-clone/qwt-code/qwt/lib".
C3. Change to "Debug" mode again, and hit "build".
	This takes some time, because all the qwt projects are compiled as well.
C4. You might need to add an environment variable LIBRARY_PATH, which points to "C:\Qt\Qt5.1.1\5.1.1\mingw48_32\bin" in my case.
    Otherwise, some libraries might not be found during linking.
A4. Finally, make the WASABIGuiQt project the active project by right-clicking on it in the "Projects" pane 
    and selecting the corresponding option.
A5. Press the green "Play" button to the left.
	You will be presented with an error message like "Unable to load WASABI.ini", because the following files need to be copied to the folder indicated in that message:
	- WASABI.ini
	- *.emo_dyn
	- *.emo_pad
	- *.se
	They can be found in the main folder of the project.

Take a look at the WASABIGuiQt.pro file, if you run into trouble. The system expects to find the "WASABIEngine.dll" in the directory "../build-WASABIEngine-Desktop_Qt_5_1_1_MinGW_32bit-[Debug/Release]/[debug/release]" subdirectory. The qwt.dll should reside in the "WASABI-qwt-clone/qwt-code/qwt/lib" subdirectory.

-- (Ubuntu) Linux --

to be described soon.

1c. Running

-- Win7 --

Starting directly from within QtCreator should work out-of-the-box. If you want to start the executable, you may have to copy the dlls into the same folder as the executable, i.e., the release/debug directories. Furthermore, as it depends on your choice of options in QtCreator which directory structure will be generated by the development environment, you might also have to copy the following initialization files to the same folder as the executable (debug and/or release again):
- WASABI.ini
- *.emo_dyn
- *.emo_pad
- *.se
These file(types) can be found in the source folder alongside the sources.

-- Linux --

TODO: needs to be updated..
In case of a linux environment, I found that the "WASABIGuiQt" executable seems to expect the WASABIEngine library in "/usr/lib" (as the output of "ldd WASABIGuiQt" told me). Thus, after manually copying the library (together with the symbolic links, "sudo cp lib* /usr/lib") to that folder, the Gui runs fine. Sorry for this inconvenience. If anybody nows how to automate/fix this, please let me now.

2. Documentation
I used doxygen to compile an html documentation from the source code. It can be found in
"WASABIEngine\doxygen\html".

3. Additional information
To get additional information about WASABI, reading my PhD thesis might be helpful.
@phdthesis{
  author = {Becker-Asano, Christian},
  title = {WASABI: Affect Simulation for Agents with Believable Interactivity},
  school = {Faculty of Technology, University of Bielefeld},
  year = {2008},
  note = {IOS Press (DISKI 319)},
  url = {http://www.becker-asano.de/Becker-Asano_WASABI_Thesis.pdf}
}

Anyway, please feel free to contact me.

Best,
CBA
