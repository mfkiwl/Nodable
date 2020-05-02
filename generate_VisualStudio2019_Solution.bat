@echo off

echo ---------------------------------------------------
echo   Nodable - Configure Visual Studio 2019 Solution.
echo ---------------------------------------------------

set BUILD_FOLDER=build
::set VS_STUDIO_VERION="Visual Studio 15 2017"
set VS_STUDIO_VERION="Visual Studio 16 2019"

IF NOT EXIST build GOTO :build

:choice
set /P c=The target folder "%BUILD_FOLDER%" exists, erase and continue ? [Y/N]
if /I "%c%" EQU "Y" goto :deleteFolder
if /I "%c%" EQU "N" goto :abort
goto :choice

:deleteFolder
rmdir /S /Q %BUILD_FOLDER%
mkdir %BUILD_FOLDER%

:build
cmake -G %VS_STUDIO_VERION% -B %BUILD_FOLDER% || goto :error

:done
echo Finished! Open %BUILD_FOLDER%/Nodable.sln
IF "%1"=="--no-prompt" exit
pause
exit

:abort
echo Configuration canceled.
pause
exit

:error
echo Configuration failed: Unable to generate Solution.
pause