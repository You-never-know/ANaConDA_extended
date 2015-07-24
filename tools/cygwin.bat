@rem
@rem Description:
@rem   A script for starting and configuring Cygwin shells which are able to
@rem   run the shell scripts (.sh files) in the tools folder. By default, an
@rem   interactive login shell is started inside a Cygwin terminal (mintty).
@rem Author:
@rem   Jan Fiedor
@rem Version:
@rem   1.5
@rem Created:
@rem   03.06.2015
@rem Last Update:
@rem   24.07.2015
@rem

@rem Expand variables at the execution time rather than the parse time
@setlocal EnableDelayedExpansion

@rem Remember the directory where the script is before processing parameters
@set SCRIPT_DIR=%~dp0

@rem Settings section
@rem ----------------

@rem Cygwin information
@set "CYGWIN_SETUP=setup-x86.exe"
@set "CYGWIN_SETUP_64=setup-x86_64.exe"
@set "CYGWIN_URL=https://cygwin.com/%CYGWIN_SETUP%"
@set "CYGWIN_URL_64=https://cygwin.com/%CYGWIN_SETUP_64%"

@rem Skip the section containing functions
@goto :ProgramSection

@rem Functions section
@rem -----------------

:ChooseFolder
@set /P FOLDER=%1
@goto :EOF

:InstallCygwin
@rem Get the directory where to store Cygwin setup files and downloaded packages
@pushd %SCRIPT_DIR%\..\..
@set "CYGWIN_SETUP_DIR=%CD%\Install\Cygwin"
@popd
@choice /N /m "Store Cygwin setup files and downloaded packages in %CYGWIN_SETUP_DIR%? [Y/N]"
@if errorlevel 2 (
  @call :ChooseFolder "Enter a custom directory: "
  @set "CYGWIN_SETUP_DIR=!FOLDER!"
)

@rem Create the directory for the Cygwin setup files and downloaded packages
@mkdir %CYGWIN_SETUP_DIR% >NUL 2>&1
@if errorlevel 1 (
  @echo error: cannot create directory %CYGWIN_SETUP_DIR%.
  @exit /b %errorlevel%
)

@rem Determine the version of the operating system (needed for download command)
@for /f "tokens=1,2*" %%i in ('reg query "HKLM\Software\Microsoft\Windows NT\CurrentVersion" /v "CurrentVersion"') do (
  @if "%%i" == "CurrentVersion" (
    @set "WINVER=%%k"
  )
)

@rem Use PowerShell commands to download Cygwin on newer versions of Windows
@if %WINVER% GTR 6.0 (
  @if /I "%TARGET%" == "amd64" (
    powershell.exe -command "Start-BitsTransfer %CYGWIN_URL_64% %CYGWIN_SETUP_DIR%\%CYGWIN_SETUP_64%"
  ) else (
    powershell.exe -command "Start-BitsTransfer %CYGWIN_URL% %CYGWIN_SETUP_DIR%\%CYGWIN_SETUP%"
  )
) else (
  @if /I "%TARGET%" == "amd64" (
    bitsadmin.exe /transfer "CygwinDownloadJob" "%CYGWIN_URL_64%" "%CYGWIN_SETUP_DIR%\%CYGWIN_SETUP_64%"
  ) else (
    bitsadmin.exe /transfer "CygwinDownloadJob" "%CYGWIN_URL%" "%CYGWIN_SETUP_DIR%\%CYGWIN_SETUP%"
  )
)
@goto :CheckCygwinHome

@rem Program section
@rem ---------------

:ProgramSection
@rem Default values for optional parameters
@set EXECUTION_ENVIRONMENT=terminal
@set TARGET=%PROCESSOR_ARCHITECTURE%

@rem Process the optional parameters
:ProcessNextParameter
@if "%~1" == "" @goto :AllParametersProcessed
@if "%~1" == "--no-terminal" (
  @set EXECUTION_ENVIRONMENT=shell
)
@if "%~1" == "-c" @goto :ProcessShellCommand
@if "%~1" == "--command" @goto :ProcessShellCommand
@if "%~1" == "-t" @goto :ProcessTarget
@if "%~1" == "--target" @goto :ProcessTarget
@shift
@goto :ProcessNextParameter

:ProcessShellCommand
@shift
@set SHELL_COMMAND=%~1
@shift
@goto :ProcessNextParameter

:ProcessTarget
@shift
@if "%~1" == "amd64" @set TARGET=amd64
@if "%~1" == "x64" @set TARGET=amd64
@if "%~1" == "x86_64" @set TARGET=amd64
@if "%~1" == "x86" @set TARGET=x86
@shift
@goto :ProcessNextParameter

:AllParametersProcessed

@rem Find Visual Studio 2013, can compile ANaConDA and is supported by PIN
@if "%VS120COMNTOOLS%" == "" (
  @echo error: Visual Studio 2013 not found.
  @exit /b 1
)

@call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" %TARGET%

@rem Find Cygwin, need its shells to run the scripts in the tools folder
@for /f "tokens=1,2*" %%i in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Cygwin\setup" /v "rootdir"') do (
  @if "%%i" == "rootdir" (
    @set "CYGWIN_HOME=%%k"
  )
)

@if "%CYGWIN_HOME%" == "" (
  @choice /N /m "Cygwin not found, do you want to install it now? [Y/N]"
  @if errorlevel 2 @goto :CheckCygwinHome
  @if errorlevel 1 @goto :InstallCygwin
)

:CheckCygwinHome
@if "%CYGWIN_HOME%" == "" (
  @echo error: Cygwin not found.
  @exit /b 1
)

@rem If no command is given, just leave the shell or terminal opened
@if "%SHELL_COMMAND%" == "" (
  @set SHELL_COMMAND=exec bash
)

@rem Start a Cygwin shell in the ANaConDA directory
@if "%EXECUTION_ENVIRONMENT%" == "terminal" (
  "%CYGWIN_HOME%\bin\mintty.exe" -i /Cygwin-Terminal.ico "%CYGWIN_HOME%\bin\bash.exe" -l -c "cd \"%SCRIPT_DIR%/..\"; %SHELL_COMMAND%"
) else (
  "%CYGWIN_HOME%\bin\bash.exe" -l -c "cd \"%SCRIPT_DIR%/..\"; %SHELL_COMMAND%"
)

@rem End of script
