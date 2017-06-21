# dllGatherer
Console application that finds .dll files required for an executable to run properly and copies them to the executable directory. It doesn't replace existing files.  

Compilation
-----------
Requires Qt library.

Run (in ddlGatherer source directory or pointing to it):

    qmake

and then:

    make
Although it solves the issue for other executables, this program itself has the issue so it requires a first time manual setup, in my local setup the list of files is:  
libgcc_s_seh-1.dll (comes with mingw C++ compiler)    
libicudt58.dll (ICU, comes with Qt by default)  
libicuin58.dll (ICU, comes with Qt by default)  
libicuuc58.dll (ICU, comes with Qt by default)  
libstdc++-6.dll (comes with mingw C++ compiler)  
libwinpthread-1.dll (comes with mingw C++ compiler)  
Qt5Core.dll (Qt)  
zlib1.dll (zlib, comes with Qt by default)  
The files must be in the same directory as the dllGatherer executable or in one of the PATH directories. Some of the files might be optional, like the libicu* ones because the local installed Qt library doesn't have ICU.  
If it still complains about needing dlls use a program like cygcheck.exe, Cygwin or Msys2 install it, or search for a "dependency walker" program to find the remaining.  

"config.json" (required in the same directory) structure-example
----------------------------------------------------------------
```json
{
  "cygcheckPath": "H:\\msys64\\usr\\bin\\cygcheck.exe"
    , "cygcheckDependencyNotFoundError": "cygcheck: track_down: could not find "
    , "windeployqtPath": "H:\\msys64\\mingw64\\bin\\windeployqt.exe"
  , "includeDllPaths": ["H:\\msys64\\mingw64\\bin"]
    , "excludeDllPaths": ["C:\\Program Files (x86)\\gtk-3.8.1", "H:\\FreeArc\\bin"]
}
```
"cygcheckPath" mandatory, cygcheck executable path or anything that outputs like it

"cygcheckDependencyNotFoundError" optional, defaults to "cygcheck: track_down: could not find " (the default error when cygcheck can't find a dll), this message is parsed to get the dll afterwards.

"windeployqtPath" optional

"includeDllPaths" optional, additional directories to search .dll files, by default dllGatherer searches in the PATH directories for dlls

"excludeDllPaths" optional, there might be undesired PATH directories to find/copy dlls from

Command line usage
------------------

    dllGatherer.exe [another exe path]
    
