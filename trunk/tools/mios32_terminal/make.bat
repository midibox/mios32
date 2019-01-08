@echo off
set VCINSTALLDIR=c:\program files (x86)\microsoft visual studio 9.0\VC
set VSINSTALLDIR=c:\program files (x86)\microsoft visual studio 9.0

if "%VSINSTALLDIR%"=="" goto error_no_VSINSTALLDIR
if "%VCINSTALLDIR%"=="" goto error_no_VCINSTALLDIR

echo Setting environment for using Microsoft Visual Studio 2008 x86 tools.

rem
rem Root of Visual Studio IDE installed files.
rem
set DevEnvDir=%VSINSTALLDIR%\Common7\IDE

set PATH=%DevEnvDir%;%VCINSTALLDIR%\BIN;%VSINSTALLDIR%\Common7\Tools;%VSINSTALLDIR%\Common7\Tools\bin;%VCINSTALLDIR%\PlatformSDK\bin;%FrameworkSDKDir%\bin;%FrameworkDir%\%FrameworkVersion%;%VCINSTALLDIR%\VCPackages
set INCLUDE=%VCINSTALLDIR%\ATLMFC\INCLUDE;%VCINSTALLDIR%\INCLUDE;%VCINSTALLDIR%\PlatformSDK\include;%FrameworkSDKDir%\include;%INCLUDE%;C:\Program Files\Microsoft SDKs\Windows\v6.0A\Include
set LIB=C:\Program Files\Microsoft SDKs\Windows\v6.0A\Lib;%VCINSTALLDIR%\ATLMFC\LIB;%VCINSTALLDIR%\LIB;%VCINSTALLDIR%\PlatformSDK\lib;%FrameworkSDKDir%\lib;%MIOS32_PATH%\drivers\gnustep\portmidi\pm_win\Release VC9;%MIOS32_PATH%\drivers\gnustep\portmidi\porttime\Release VC9

"%VCINSTALLDIR%\bin\nmake" %1 %2 %3 %4

goto end

:error_no_VSINSTALLDIR
echo ERROR: VSINSTALLDIR variable is not set. 
goto end

:error_no_VCINSTALLDIR
echo ERROR: VCINSTALLDIR variable is not set. 
goto end
:end