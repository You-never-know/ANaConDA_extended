::
:: Copyright (C) 2012-2019 Jan Fiedor <fiedorjan@centrum.cz>
::
:: This file is part of ANaConDA.
::
:: ANaConDA is free software: you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation, either version 3 of the License, or
:: any later version.
::
:: ANaConDA is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with ANaConDA. If not, see <http://www.gnu.org/licenses/>.
::

::
:: Description:
::   A script simplifying the usage of ANaConDA.
:: Author:
::   Jan Fiedor
:: Version:
::   0.2
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

if not exist "%PIN_HOME%\pin.bat" (
  if "%PIN_HOME%" == "" (
    echo error: pin.bat not found, the PIN_HOME variable is not set or empty, set it to point to the installation directory of PIN.
    exit /b
  )
  for %%i in ("%PIN_HOME%") do if not exist %%~si\NUL (
    echo error: pin.bat not found, the PIN_HOME variable do not point to a directory, set it to point to the installation directory of PIN.
    exit /b
  )
  echo error: pin.bat not found, set the PIN_HOME variable to point to the installation directory of PIN.
  exit /b
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

"%PIN_HOME%\pin.bat" -t "%CD%\lib\intel64\anaconda" --show-settings -a %ANALYSER% -- %APP%
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
