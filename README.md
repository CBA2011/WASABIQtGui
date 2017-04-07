Freiburg, 26th of September, 2011
UPDATE: 10th of May, 2013
UPDATE: 7th of April, 2017

[W]ASABI [A]ffect [S]imulation [A]rchitecture for [B]elievable [I]nteractivity
GUI part version 0.7
Copyright (C) 2011 Christian Becker-Asano.
All rights reserved.

Contact: Christian Becker-Asano (christian (at) becker-asano (dot) de)

# General procedure to compile and run (Windows 64bit)

1. Setup

Starting from 2017 this project can be compiled and run with QtCreator 4.2.1 using Qt5.8.0 (MinGW, 32bit) on Windows. Older versions of Qt (e.g. Qt5.1 and up) and other operating systems (MacOS, Linux) might work as well.

It depends on Qt, OpenGL, and the WASABIEngine as dynamically linked libraries. The first two come with the Qt SDK. The last one is integrated as submodules.

After a fresh clone of this project or after the first "Pull" that updates your local repo to the submodule version, you will only have the empty subdirectories "WASABIEngine" and "WASABI-qwt-clone" and compiling will fail. Thus, perform:
$ git submodule update --init "WASABIEngine"
$ git submodule update --init "WASABI-qwt-clone"

2. Compiling

-- Win7 --

Instructions for the qt-project.org-Version of Qt5 (UPDATED 22nd of October, 2013, and again on April 7, 2017):
* Open the file "WASABIGuiQt.pro" by double clicking on it.
* use the default configuration for "Desktop Qt 5.8.0 MinGW 32bit" with two seperate directories for "Debug" and "Release".
  That looks something like "../build-WASABIGuiQt-Desktop_Qt_5_8_0_MinGW_32bit-Debug" and "../build-WASABIGuiQt-Desktop_Qt_5_8_0_MinGW_32bit-Release" for me.
* Try to compile in Debug mode and you will get a message like "cannot find -lWASABIEngine".
- Open the file "WASABIEngine/WASABIEngine.pro".
- Change both build-paths for debug and release to:
  <path\to\git-working-dir>\WASABIQtGui\build-WASABIEngine
  The WASABIEngine builds will then reside in the (newly created) subdirectories "debug" and "release".
- Change to "Debug" mode on the left panel (the Computer screen button), and hit "build" (the hammer button).
  This should result in the file "../build-WASABIEngine/debug/WASABIEngine.dll". Similarly, you may try to build the release version.
* Finally, make the WASABIGuiQt project the active project by right-clicking on it in the "Projects" pane and selecting the corresponding option.

3. Running

* Press the green "Play" button to the left.
  You will be presented with an error message like "Unable to load WASABI.ini".
  You have to copy the "xml" subfolder with all its contents into the build directory. Then, the file WASABI_agent_default.xml can be referenced in the WASABI.ini as it currently is and this file needs to be copied to the build folder as well. All data will then be loaded from the xml file. It conforms the EmotionML standard (http://www.w3.org/TR/emotionml/).
  Optionally, you can automate this copy process by adding a "Custom Process Step" in the projects "Build Settings" dialog in QtCreator.
  + Command: cp.exe
  + Arguments: -r ../WASABIQtGui/WASABI.ini ../WASABIQtGui/xml .
  + Working directory: %{buildDir}
In the end of the build process, you should find the following line in the Compile Output now:
  + "Starting: "C:\MinGW\msys\1.0\bin\cp.exe" -r ../WASABIQtGui/WASABI.ini ../WASABIQtGui/xml ."

Take a look at the WASABIGuiQt.pro file, if you run into trouble. 

# Notes on Linux environments

TODO: needs to be updated..
In case of a linux environment, I found that the "WASABIGuiQt" executable seems to expect the WASABIEngine library in "/usr/lib" (as the output of "ldd WASABIGuiQt" told me). Thus, after manually copying the library (together with the symbolic links, "sudo cp lib* /usr/lib") to that folder, the Gui runs fine. Sorry for this inconvenience. If anybody nows how to automate/fix this, please let me now.

# Documentation
I used doxygen to compile an html documentation from the source code. It can be found in
"WASABIEngine\doxygen\html".

# Using git to commit (locally) and push (to global repository):
Recently, TortoiseGit started to ask me for my login credentials, which was not the case so far. To fix this, proceed as follows:
a. start the "Git Bash" by pressing the Windows-Key and typing "git" in the search field.
b. change into the directory with your git project, e.g., "cd d:basano/Documents/Git/Test/WASABIQtGui"
c. type "git remote set-url origin git@github.com:<username>/<repo.git>" with <username> and <repo.git> changed accordingly, e.g.,
   git remote set-url origin git@github.com:CBA2011/WASABIQtGui.git

Then, you will be asked to provide your pasphrase for your ssh-key again.
Please also check this https://help.github.com/articles/set-up-git page.

# Additional information
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
