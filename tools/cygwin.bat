::
:: Copyright (C) 2015-2019 Jan Fiedor <fiedorjan@centrum.cz>
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

@rem
@rem Description:
@rem   A script for starting and configuring Cygwin shells which are able to
@rem   run the shell scripts (.sh files) in the tools folder. By default, an
@rem   interactive login shell is started inside a Cygwin terminal (mintty).
@rem Author:
@rem   Jan Fiedor
@rem Version:
@rem   2.7.1.1
@rem Created:
@rem   03.06.2015
@rem Last Update:
@rem   09.10.2017
@rem

@rem Expand variables at the execution time rather than the parse time
@setlocal EnableDelayedExpansion

@rem Remember the directory where the script is before processing parameters
@set SCRIPT_DIR=%~dp0

@rem Settings section
@rem ----------------

@rem Cygwin information
@set "CYGWIN_SETUP_32=setup-x86.exe"
@set "CYGWIN_SETUP_64=setup-x86_64.exe"
@set "CYGWIN_URL_32=http://cygwin.com/%CYGWIN_SETUP%"
@set "CYGWIN_URL_64=http://cygwin.com/%CYGWIN_SETUP_64%"

@rem Skip the section containing functions
@goto :ProgramSection

@rem Functions section
@rem -----------------

:ChooseFolder
@rem Check if we have PowerShell available (search the folders given by PATH)
@rem If PowerShell is available, use it to compose a Choose Folder Dialog
@rem Else create a C# application implementing the Choose Folder Dialog
@for %%i in ("powershell.exe") do @if "%%~$PATH:i" neq "" (
  @set FOLDER_CHOOSER=powershell -sta ^" ^
    Add-Type -AssemblyName System.Windows.Forms ^| Out-Null; ^
    $f = New-Object System.Windows.Forms.FolderBrowserDialog; ^
    $f.SelectedPath = '%cd%'; ^
    $f.Description = '%~1'; ^
    $f.ShowNewFolderButton = $true; ^
    $f.ShowDialog^(^); ^
    $f.SelectedPath ^"
) else (
  @set FOLDER_CHOOSER=%TEMP%\fchooser.exe
  @if exist !FOLDER_CHOOSER! @del !FOLDER_CHOOSER!
  @echo ^
    using System; using System.Windows.Forms; ^
    class dummy {^
      [STAThread] ^
      public static void Main^(^) { ^
        FolderBrowserDialog f = new FolderBrowserDialog^(^); ^
        f.SelectedPath = System.Environment.CurrentDirectory; ^
        f.Description = "%~1"; ^
        f.ShowNewFolderButton = true; ^
        if ^(f.ShowDialog^(^) == DialogResult.OK^) { ^
          Console.Write^(f.SelectedPath^); ^
        } ^
      } ^
    } >"%TEMP%\fchooser.cs"
  @for /f "delims=" %%i in ('dir /b /s "%WINDIR%\Microsoft.NET\*csc.exe" 2^>NUL') do (
    @if not exist "!FOLDER_CHOOSER!" (
      "%%i" /nologo /out:"!FOLDER_CHOOSER!" "%TEMP%\fchooser.cs" 2>NUL
    )
  )
  @del "%TEMP%\fchooser.cs"
  @if not exist "!FOLDER_CHOOSER!" (
    @set /P FOLDER="%~1: "
    @goto :EOF
  )
)

@rem Show the Choose Folder Dialog and let the user to choose a folder
@for /f "delims=" %%i in ('%FOLDER_CHOOSER%') do @set "FOLDER=%%i"

@rem Temporary files cleanup
@del "%TEMP%\fchooser.exe" 2>NUL

@rem End of function :ChooseFolder
@goto :EOF

@rem
@rem Description:
@rem   Installs or updates Cygwin. Sets the following variables:
@rem   - CYGWIN_HOME [STRING]
@rem     A directory where Cygwin was installed or updated.
@rem Parameters:
@rem   [STRING] A path to a directory where Cygwin is installed (for updating).
@rem Output:
@rem   None
@rem Return:
@rem   Nothing
@rem
:InstallCygwin
@rem Get the directory where to store Cygwin setup files and downloaded packages
@pushd %SCRIPT_DIR%\..\..
@set "CYGWIN_SETUP_DIR=%CD%\Install\Cygwin"
@popd
@choice /T %CHOICE_TIMEOUT% /D Y /N /m "Store Cygwin setup files and downloaded packages in %CYGWIN_SETUP_DIR%? [Y/n]"
@if errorlevel 2 (
  @call :ChooseFolder "Custom Cygwin Local Package Directory"
  @set "CYGWIN_SETUP_DIR=!FOLDER!"
)

@rem Create the directory for the Cygwin setup files and downloaded packages
@if not exist %CYGWIN_SETUP_DIR% (
  @mkdir %CYGWIN_SETUP_DIR% >NUL 2>&1
  @if errorlevel 1 (
    @echo error: cannot create directory %CYGWIN_SETUP_DIR%.
    @exit /b %errorlevel%
  )
)

@rem Determine the version of the operating system (needed for download command)
@for /f "tokens=1,2*" %%i in ('reg query "HKLM\Software\Microsoft\Windows NT\CurrentVersion" /v "CurrentVersion"') do (
  @if "%%i" == "CurrentVersion" (
    @set "WINVER=%%k"
  )
)

@rem Determine which Cygwin setup files to use
@if /I "%TARGET%" == "amd64" (
  @set "CYGWIN_URL=%CYGWIN_URL_64%"
  @set "CYGWIN_SETUP=%CYGWIN_SETUP_64%"
) else (
  @set "CYGWIN_URL=%CYGWIN_URL_32%"
  @set "CYGWIN_SETUP=%CYGWIN_SETUP_32%"
)

@rem Use PowerShell commands to download Cygwin on newer versions of Windows
@if %WINVER% GTR 6.0 (
  powershell.exe -command "Import-Module BitsTransfer; Start-BitsTransfer %CYGWIN_URL% %CYGWIN_SETUP_DIR%\%CYGWIN_SETUP%"
) else (
  bitsadmin.exe /transfer "CygwinDownloadJob" "%CYGWIN_URL%" "%CYGWIN_SETUP_DIR%\%CYGWIN_SETUP%"
)

@rem Check if the setup files downloaded successfully
@if not exist %CYGWIN_SETUP_DIR%\%CYGWIN_SETUP% (
  @echo error: failed to download Cygwin setup files ^(%CYGWIN_SETUP%^).
  @exit /b 1
)

@rem Get the directory where Cygwin should be installed
@if not exist "%~1" (
  @pushd %SCRIPT_DIR%\..\..
  @set "CYGWIN_INSTALL_DIR=!CD!\Cygwin"
  @popd
  @choice /T %CHOICE_TIMEOUT% /D Y /N /m "Install Cygwin to !CYGWIN_INSTALL_DIR!? [Y/n]"
  @if errorlevel 2 (
    @call :ChooseFolder "Custom Cygwin Root Directory"
    @set "CYGWIN_INSTALL_DIR=!FOLDER!"
  )
) else (
  @set "CYGWIN_INSTALL_DIR=%~1"
)

@rem Create the directory where Cygwin should be installed
@if not exist %CYGWIN_INSTALL_DIR% (
  @mkdir %CYGWIN_INSTALL_DIR% >NUL 2>&1
  @if errorlevel 1 (
    @echo error: cannot create directory %CYGWIN_INSTALL_DIR%.
    @exit /b %errorlevel%
  )
)

@rem Install Cygwin with the packages required by the automation scripts
@pushd %CYGWIN_SETUP_DIR%
@cmd /C %CYGWIN_SETUP% --quiet-mode --download --root %CYGWIN_INSTALL_DIR% ^
  --no-admin --local-install --local-package-dir %CYGWIN_SETUP_DIR% ^
  --site http://ftp.inf.tu-dresden.de/software/windows/cygwin32/ ^
  --no-shortcuts --packages bash,mintty,make,wget,unzip,zip,rsync,git,openssh
@popd

@rem Installation complete, configure the environment
@set "CYGWIN_HOME=%CYGWIN_INSTALL_DIR%"

@rem End of function :InstallCygwin
@goto :EOF

@rem Program section
@rem ---------------

:ProgramSection
@rem Default values for optional parameters
@set EXECUTION_ENVIRONMENT=terminal
@set TARGET=%PROCESSOR_ARCHITECTURE%
@set QUIET=0

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
@if "%~1" == "--quiet" (
  @set QUIET=1
)
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

@rem In quiet mode, use the defaults and continue without prompting the user
@if "%QUIET%" == "1" (
  @set CHOICE_TIMEOUT=0
) else (
  @set CHOICE_TIMEOUT=9999
)

@rem Find Visual Studio 2013, can compile ANaConDA and is supported by PIN
@if "%VS120COMNTOOLS%" == "" (
  @echo warning: Visual Studio 2013 not found.
) else (
  @if not exist "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" (
    @echo warning: Visual Studio 2013 not installed properly, vcvarsall.bat not found.
  ) else (
    @call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" %TARGET%
  )
)

@rem Find Cygwin, need its shells to run the scripts in the tools folder
@if "%CYGWIN_HOME%" == "" (
  @for /f "tokens=1,2*" %%i in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Cygwin\setup" /v "rootdir"') do (
    @if "%%i" == "rootdir" (
      @set "CYGWIN_HOME=%%k"
    )
  )
  @if "%CYGWIN_HOME%" == "" (
    @for /f "tokens=1,2*" %%i in ('reg query "HKEY_CURRENT_USER\SOFTWARE\Cygwin\setup" /v "rootdir"') do (
      @if "%%i" == "rootdir" (
        @set "CYGWIN_HOME=%%k"
      )
    )
  )
)

@if "%CYGWIN_HOME%" == "" (
  @choice /T %CHOICE_TIMEOUT% /D Y /N /m "Cygwin not found, do you want to install it now? [Y/n]"
  @if errorlevel 2 (
    @echo error: Cygwin not found.
    @exit /b 1
  )
  @if errorlevel 1 @call :InstallCygwin
)

@if not exist "%CYGWIN_HOME%\bin\mintty.exe" (
  @choice /T %CHOICE_TIMEOUT% /D Y /N /m "Cygwin terminal (mintty) not found, do you want to install it now? [Y/n]"
  @if errorlevel 2 (
    @echo error: Cygwin terminal ^(mintty^) not found.
    @exit /b 1
  )
  @if errorlevel 1 @call :InstallCygwin "%CYGWIN_HOME%"
)

@if not exist "%CYGWIN_HOME%\bin\bash.exe" (
  @choice /T %CHOICE_TIMEOUT% /D Y /N /m "Cygwin bourne shell (bash) not found, do you want to install it now? [Y/n]"
  @if errorlevel 2 (
    @echo error: Cygwin bourne shell ^(bash^) not found.
    @exit /b 1
  )
  @if errorlevel 1 @call :InstallCygwin "%CYGWIN_HOME%"
)

@rem If Cygwin is configured to use DACL (Windows ACL) to protect files, it may
@rem prevent compilation of ANaConDA as the created folders may be inaccessible
@for /f "delims=" %%i in ('findstr cygdrive %CYGWIN_HOME%\etc\fstab') do @set "CYGDRIVE_CONFIG=%%i"
@if not "%CYGDRIVE_CONFIG%" == "" (
  @for /f "tokens=4 delims= " %%i in ("%CYGDRIVE_CONFIG%") do (
    @set CYGDRIVE_MOUNT_OPTIONS="%%i"
    @if "!CYGDRIVE_MOUNT_OPTIONS:noacl=!" == "!CYGDRIVE_MOUNT_OPTIONS!" (
      @choice /T %CHOICE_TIMEOUT% /D Y /N /m "Cygwin is enforcing Linux permissions which may prevent the framework to compile, do you want to disable this enforcement? [Y/n]"
      @if errorlevel 2 @goto :CygdriveCheckEnd
      @if errorlevel 1 (
        @if not exist "%CYGWIN_HOME%\etc\fstab.orig" (
          @copy "%CYGWIN_HOME%\etc\fstab" "%CYGWIN_HOME%\etc\fstab.orig"
        )
        @copy "%CYGWIN_HOME%\etc\fstab" "%CYGWIN_HOME%\etc\fstab.bak"
        @copy /y NUL "%CYGWIN_HOME%\etc\fstab" >NUL
        @for /f "tokens=1,2,3,4*" %%i in ("%CYGDRIVE_CONFIG%") do (
          @set CYGDRIVE_CONFIG_NEW=%%i %%j %%k !CYGDRIVE_MOUNT_OPTIONS:"=!,noacl %%m
        )
        @for /f skip^=2^ tokens^=1*^ delims^=]^ eol^= %%i in ('find /n /v "" %CYGWIN_HOME%\etc\fstab.orig') do (
          @if "%%j" == "%CYGDRIVE_CONFIG%" (
            @echo !CYGDRIVE_CONFIG_NEW! >>"%CYGWIN_HOME%\etc\fstab"
          ) else (
            @if "%%j" == "" (
              @echo. >>"%CYGWIN_HOME%\etc\fstab"
            ) else (
              @echo %%j >>"%CYGWIN_HOME%\etc\fstab"
            )
          )
        )
      )
    )
  )
)
:CygdriveCheckEnd

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
