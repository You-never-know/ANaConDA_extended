::
:: Description:
::   A script simplifying the usage of ANaConDA.
:: Author:
::   Jan Fiedor
:: Version:
::   0.1
:: Created:
::   26.06.2012
:: Last Update:
::   27.06.2012
::

@echo off

setlocal enabledelayedexpansion

if [%1] == [] (
  echo error: no analyser specified.
  echo.
  goto :usage
)

if [%1] == [/?] (
  goto :usage
)
if [%1] == [-h] (
  goto :usage
)

if [%2] == [] (
  echo error: no program specified.
  echo.
  goto :usage
)

if not exist %PIN_HOME%\pin.bat (
  if "%PIN_HOME%" == "" (
    echo error: pin.bat not found, the PIN_HOME variable is not set or empty, set it to point to the installation directory of PIN.
    exit /b
  )
  for %%i in (%PIN_HOME%) do if not exist %%~si\NUL (
    echo error: pin.bat not found, the PIN_HOME variable do not point to a directory, set it to point to the installation directory of PIN.
    exit /b
  )
)

set ANALYSER=%1
shift

if [%1]==[] goto afterloop
set APP=%1
:loop
shift
if [%1]==[] goto afterloop
set APP=%APP% %1
goto loop
:afterloop

%PIN_HOME%\pin.bat -t %CD%\lib\intel64\anaconda --show-settings -a %ANALYSER% -- %APP%
goto :end

:usage
echo Usage:
echo   anaconda.bat ^<analyser^> ^<program^> [^<arguments^>]
echo.
echo   ^<analyser^>  A path to the analyser to be used.
echo   ^<program^>   A path to the program to be analysed.
echo   ^<arguments^> A space-separated list of arguments passed to the program.

:end

endlocal

:: End of file anaconda.bat
