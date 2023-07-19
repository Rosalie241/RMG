@echo off
setlocal enableextensions disabledelayedexpansion
cls
set FPROJ=GLideN64.sln
set DMN=REM
set ERR=if errorlevel 1 goto err

:: This allows the script to work from outside the repository for ease of
:: use by third-party tools with extended functionality such as regression
:: tracking
set TPSM=GLideN64\projects\msvc\
set EXIT=exit
if exist "%~dp0%TPSM%%FPROJ%" goto slavemode
set TPSM=
set EXIT=exit /b
set MSG=%FPROJ% does not exist in the same directory
if not exist "%~dp0%TPSM%%FPROJ%" goto err
:slavemode

set REB=
set CONF=Release
set EXT=7z
set DQTD=1
set EBQ=start "" explorer .
for %%B in (EX86 EX64 D7Z DPJQT DMQT DPJWTL DMCL ESIM) do set "%%B=0"
set DMQT=1
for %%K in (msbuild vctip mspdbsrv) do taskkill /im %%K.exe /f 2>nul
if "%*"=="" goto help
for %%P in (%*) do (
	if /i "%%P"=="--clean" goto clean
	if /i "%%P"=="--x86" set EX86=1
	if /i "%%P"=="--x64" set EX64=1
	if /i "%%P"=="--all" set EX86=1& set EX64=1
	if /i "%%P"=="--debug" set CONF=Debug
	if /i "%%P"=="--zip" set EXT=zip
	if /i "%%P"=="--nopak" set D7Z=1
	if /i "%%P"=="--rebuild" set REB=/t:Rebuild
	if /i "%%P"=="--nopjqt" set DPJQT=1
REM	if /i "%%P"=="--nomqt" set DMQT=1
REM	if /i "%%P"=="--noqt" set DPJQT=1& set DMQT=1
	if /i "%%P"=="--nopjwtl" set DPJWTL=1
	if /i "%%P"=="--nomcl" set DMCL=1
	if /i "%%P"=="--dlqt" set DQTD=0
	if /i "%%P"=="--sim" set ESIM=1
	if /i "%%P"=="--q" set EBQ=REM
)

set /a TSK=%EX86%+%EX64%
if %TSK%==0 goto help
set /a DQT=%DPJQT%+%DMQT%
set /a MOD=%DQT%+%DPJWTL%+%DMCL%
set MSG=All compilation tasks were disabled on request
if %MOD%==4 goto err
set MSG=7z was not found in the environment
set /a MOD=%D7Z%+%DQTD%
if %MOD% NEQ 2 7z >nul 2>&1
%ERR%

set tVSPF=%ProgramFiles(x86)%
if "%PROCESSOR_ARCHITECTURE%"=="x86" set tVSPF=%ProgramFiles%
set "tVSPF=%tVSPF%\Microsoft Visual Studio"
set tVSDS=Common7\Tools\VsDevCmd.bat
set "tVSWHERE=%tVSPF%\Installer\vswhere.exe"
set MOD=\ \Community\ \Enterprise\ \Professional\
if exist "%tVSWHERE%" (
	for /f "usebackq tokens=*" %%U in (`"%tVSWHERE%" -latest -property installationPath`) do set tVSPF=%%U
)
goto vsbeg
:vsenv
	for %%V in (%MOD%) do (
		if exist "%tVSPF%%~1%%V%tVSDS%" set "tVSDS=%tVSPF%%~1%%V%tVSDS%"
	)
	goto:eof

:vsbeg
call :vsenv
call :vsenv "\2017"
set MOD=\
call :vsenv " 14.0"
call :vsenv " 12.0"

set MSG=Visual Studio developer environment was not loaded
msbuild -version >nul 2>&1
if errorlevel 1 call "%tVSDS%"
%ERR%
set MSG=Git was not found in the environment
git --version >nul 2>&1
%ERR%

set ARCH=x86
if %EX64%==1 set ARCH=x64
set "TARCH=%ARCH%"
:X86
pushd "%~dp0%TPSM%..\..\"
if "%ARCH%"=="x86" set TARCH=Win32

if %DQT%==2 goto noqt
if defined QTDIR_%ARCH% goto nodl
set "QTVER=qt-5_15-%ARCH%-msvc2017-static"
if exist "..\Qt\%QTVER%\include\QtCore" goto nodl

set MSG=Path to Qt %ARCH% was not specified or detected
if %DQTD%==1 goto err
set MSG=cURL was not found in the environment
curl --version >nul 2>&1
%ERR%
del /f /q "..\%QTVER%.7z" 2>nul
type nul
set QTURL=https://github.com/gonetz/GLideN64/releases/download/qt_build
set MSG=cURL failed to download:^& echo %QTURL%/%QTVER%.7z
curl -L -o "..\%QTVER%.7z" "%QTURL%/%QTVER%.7z"
%ERR%
set MSG=7z failed to extract:^& echo %QTVER%.7z
7z x -y "..\%QTVER%.7z" -o"..\Qt"
%ERR%

:nodl
set "QTDIR=%~dp0%TPSM%..\..\..\Qt\%QTVER%"
if "%ARCH%"=="x64" (
	if defined QTDIR_x64 set "QTDIR="%QTDIR_x64%""
) else (
	if defined QTDIR_x86 set "QTDIR="%QTDIR_x86%""
)
set QTDIR=%QTDIR:"=%
set MSG=Something went wrong when detecting Qt %ARCH%:^& echo %QTDIR%
if not exist "%QTDIR%\include\QtCore" goto err
:noqt

if not defined BUILDROUTE set "BUILDROUTE="%~dp0%TPSM%..\..\build""
set BUILDROUTE=%BUILDROUTE:"=%
for /f "tokens=1" %%R in ('git rev-parse --short HEAD') do set "REV=GLideN64-%%R-"
set "IDTS=-%ARCH%"
if "%CONF%"=="Debug" set "IDTS=%IDTS%-dbg"
if defined TOOLSET set "IDTS="-%TOOLSET%%IDTS%""
set IDTS=%IDTS:"=%
set "PJ64QT=%BUILDROUTE%\%REV%Project64-Qt%IDTS%"
::set "M64QT=%BUILDROUTE%\%REV%Mupen64Plus-Qt%IDTS%"
set "PJ64WTL=%BUILDROUTE%\%REV%Project64-WTL%IDTS%"
set "M64CL=%BUILDROUTE%\%REV%Mupen64Plus-CLI%IDTS%"

set MOD=
if "%ARCH%"=="x64" set MOD=_x64
set "PJ64PluginsDirQT%MOD%=%PJ64QT%"
::set "Mupen64PluginsDirQT%MOD%=%M64QT%"
set "PJ64PluginsDirWTL%MOD%=%PJ64WTL%"
set "Mupen64PluginsDir%MOD%=%M64CL%"

set MOD=%RANDOM:~-1%
if %MOD% LEQ 4 (set MOD=Lylat
	) else if %MOD% GEQ 8 (set MOD=Kildean) else set MOD=Caryll

set BPJQT=msbuild
if %ESIM%==1 (
	set "BPJQT=echo msbuild"
	md "translations\wtl" 2>nul
	cd.>"translations\wtl\%MOD%.Lang"
	pushd "%BUILDROUTE%"
	for /f "tokens=*" %%S in ('dir /ad /b %REV%*%IDTS%') do (
		if "%%S" NEQ "" rd /s /q "%%S"
	)
	del /f /q "%REV%*%IDTS%.%EXT%" 2>nul
	popd
	type nul
)
for %%C in (MQT PJWTL MCL) do set "B%%C=%BPJQT%"
if %DPJQT%==1 set BPJQT=REM
if %DMQT%==1 set BMQT=REM
if %DPJWTL%==1 set BPJWTL=REM
if %DMCL%==1 set BMCL=REM

set "PTS=Platform=%TARCH%"
if defined TOOLSET set "PTS="%PTS%;PlatformToolset=%TOOLSET%""
set PTS=%PTS:"=%

goto mbbeg
:mbcl
	%~1 "%~dp0%TPSM%%~2" /m /p:Configuration=%CONF%%~3;%PTS% %REB%
	%ERR%
	if "%~1" NEQ "REM" echo.
	goto:eof

:mbbeg
set MSG=Qt version, architecture and path are really correct?^& echo %QTDIR%
call :mbcl "%BPJQT%" "GLideNUI.vcxproj"
%DMN%
call :mbcl "%BPJQT%" "%FPROJ%" "_qt"
%DMN%

::if %DPJQT%==1 call :mbcl "%BMQT%" "GLideNUI.vcxproj"
%DMN%
::call :mbcl "%BMQT%" "%FPROJ%" "_mupenplus_qt"
%DMN%

set MSG=ERROR!
call :mbcl "%BPJWTL%" "GLideNUI-wtl.vcxproj"
%DMN%
call :mbcl "%BPJWTL%" "%FPROJ%" "_wtl"
%DMN%

call :mbcl "%BMCL%" "%FPROJ%" "_mupenplus"
%DMN%

goto pjqt
:cini
	set MSG=Failed to copy some project files to:^& echo %~1
	if %ESIM%==1 md "%~1" 2>nul
	type nul
	copy /y ini\GLideN64.custom.ini "%~1\"
	%ERR%
	for %%D in (exp ilk lib) do del /f /q "%~1\*.%%D" 2>nul
	type nul
	goto:eof

:pjqt
if %DPJQT%==1 goto mqt
call :cini "%PJ64QT%"
%DMN%
copy /y translations\release\*.qm "%PJ64QT%\"
%ERR%

:mqt
if %DMQT%==1 goto pjwtl
::call :cini "%M64QT%"
%DMN%
::copy /y translations\release\*.qm "%M64QT%\"
%ERR%

:pjwtl
if %DPJWTL%==1 goto mcl
call :cini "%PJ64WTL%"
%DMN%
md "%PJ64WTL%\translations" 2>nul
type nul
copy /y translations\wtl\*.Lang "%PJ64WTL%\translations\"
%ERR%

:mcl
if %DMCL%==1 goto pkg
call :cini "%M64CL%"
%DMN%

:pkg
set MSG=The route could not be accessed:^& echo %BUILDROUTE%
pushd "%BUILDROUTE%"
%ERR%
set MOD=binaries
if %D7Z%==0 (
	set MOD=compressed files
	for /f "tokens=*" %%Z in ('dir /ad /b %REV%*%IDTS%') do 7z a -t%EXT% "%%Z.%EXT%" ".\%%Z\*"
)

if "%ARCH%"=="x64" (
	if %TSK%==2 set ARCH=x86& goto X86
)

set MSG=DONE!^& echo Path to the %MOD%:^& echo %CD%
%EBQ%
:err
set WTF=%ERRORLEVEL%
set DMN=%EXIT% %ERRORLEVEL%
echo.
echo %MSG%
%EXIT% %WTF%

:clean
pushd "%~dp0%TPSM%"
for /f "tokens=*" %%E in ('dir /ad /b') do (
	if /i "%%E" NEQ "lib" rd /s /q "%%E"
)
cd ..\..
del /f /q "src\Revision.h" 2>nul
rd /s /q "build" 2>nul
rd /s /q "translations\wtl" 2>nul
%EXIT% 0

:help
echo.
echo GLideN64's simplified build and packing script
echo.
echo Usage:
echo   set ^<variable^>
echo   %~nx0 ^<architecture^> ^<other^>
echo.
echo ^<Variables^>
echo   set BUILDROUTE=^<Custom build folder^>
echo     e.g.         "Z:\My share folder"
echo   set QTDIR_x86=^<Your Qt x86 path^>
echo     e.g.        "D:\Static Qt\qt-5.7.1-x86-msvc2013"
echo   set QTDIR_x64=^<Your Qt x64 path^>
echo     e.g.        F:\Qt\qt-5_7_1-x64-msvc2015-static
echo   set TOOLSET=^<Custom PlatformToolset^>
echo     e.g.      v141_xp
echo.
echo ^<Architectures^>
echo   --x86       To compile x86
echo   --x64       To compile x64
echo   --all       To compile x86 and x64, equivalent to using '--x86'
echo               and '--x64' simultaneously
echo.
echo ^<Others^>
echo   --clean     Clean ALL auto-generated build files within the project
echo   --debug     For debug builds
echo   --dlqt      Auto download and configure Qt for VS2017-2019, it would
echo               have no effect if Qt variables are used or if the same
echo               version has already been extracted
::echo   --noqt      To build without Qt support, equivalent to using '--nopjqt'
::echo               and '--nomqt' simultaneously, it will ignore the effects of
::echo               Qt variables and '--dlqt'
echo   --nopjqt    To skip Project64 Qt builds
::echo   --nomqt     To skip Mupen64Plus Qt builds
echo   --nopjwtl   To skip Project64 WTL builds
echo   --nomcl     To skip Mupen64Plus CLI builds
echo   --nopak     To skip packing the binaries, '--zip' will be ineffective
echo               It will disable 7-Zip completely if '--dlqt' isn't used
echo   --rebuild   To rebuild without cleaning
echo   --sim       Simulated build, quick environment check without compiling
echo               It's destructive to the final product, it removes binaries
echo               and creates dummy files, use '--clean' to discard changes
echo   --q         Don't interact with Windows Explorer at the end
echo   --zip       To package ZIP files instead 7Z files
echo.
echo Usage examples:
echo.
echo   %~nx0 --all --dlqt
echo.
echo   %~nx0 --x86 --debug --rebuild
echo.
echo   set BUILDROUTE=H:\%USERNAME%\experiment
echo   set TOOLSET=ClangCL
echo   %~nx0 --nopjqt --x64
echo.
echo   set QTDIR_x86="G:\Static Qt\qt-5.7.1-x86-msvc2015"
echo   set QTDIR_x64=G:\Static Qt\qt-5.7.1-x64-msvc2015
echo   %~nx0 --nopjwtl --nomcl --all
%EXIT% 0
