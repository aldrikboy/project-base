@echo off

set __VSCMD_ARG_no_logo=""

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.28.29333\include

DEL /S /Q build >nul
mkdir build >nul 2>nul

mkdir "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.42.34433\include\projectbase\" >nul 2>nul
robocopy "src" "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.42.34433\include\projectbase" /E

robocopy "examples\data" "build\data" /E

cd build
rc /fo icon.res ../icon.rc
cl /Z7 /DMODE_DEBUG /std:c11 /nologo /c ..\src\entry.c /Feprojectbase-debug.lib
lib /OUT:projectbase-debug.lib icon.res user32.lib gdi32.lib winmm.lib shlwapi.lib opengl32.lib kernel32.lib glu32.lib comdlg32.lib dbghelp.lib advapi32.lib shell32.lib

cl /Z7 /DMODE_DEBUG /std:c11 ..\examples\example_window.c /Feexample_window.exe projectbase-debug.lib icon.res
cd ..

call build\example_window.exe
REM call devenv build\example_window.exe