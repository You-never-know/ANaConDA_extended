@rem
@rem Description:
@rem   A script for starting and configuring Cygwin shells which are able to
@rem   run the shell scripts (.sh files) in the tools folder. By default, an
@rem   interactive login shell is started inside a Cygwin terminal (mintty).
@rem Author:
@rem   Jan Fiedor
@rem Version:
@rem   2.3
@rem Created:
@rem   03.06.2015
@rem Last Update:
@rem   28.07.2015
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

:InstallCygwin
@rem Get the directory where to store Cygwin setup files and downloaded packages
@pushd %SCRIPT_DIR%\..\..
@set "CYGWIN_SETUP_DIR=%CD%\Install\Cygwin"
@popd
@choice /N /m "Store Cygwin setup files and downloaded packages in %CYGWIN_SETUP_DIR%? [Y/N]"
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
  powershell.exe -command "Start-BitsTransfer %CYGWIN_URL% %CYGWIN_SETUP_DIR%\%CYGWIN_SETUP%"
) else (
  bitsadmin.exe /transfer "CygwinDownloadJob" "%CYGWIN_URL%" "%CYGWIN_SETUP_DIR%\%CYGWIN_SETUP%"
)

@rem Check if the setup files downloaded successfully
@if not exist %CYGWIN_SETUP_DIR%\%CYGWIN_SETUP% (
  @echo error: failed to download Cygwin setup files ^(%CYGWIN_SETUP%^).
  @exit /b 1
)

@rem Get the directory where Cygwin should be installed
@pushd %SCRIPT_DIR%\..\..
@set "CYGWIN_INSTALL_DIR=%CD%\Cygwin"
@popd
@choice /N /m "Install Cygwin to %CYGWIN_INSTALL_DIR%? [Y/N]"
@if errorlevel 2 (
  @call :ChooseFolder "Custom Cygwin Root Directory"
  @set "CYGWIN_INSTALL_DIR=!FOLDER!"
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
  --no-shortcuts --packages bash,mintty,make,wget
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

@rem Check if Visual Studio 2013 configuration scripts are present
@if not exist "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" (
  @echo error: Visual Studio 2013 not installed properly, vcvarsall.bat not found.
  @exit /b 1
)

@call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" %TARGET%

@rem Find Cygwin, need its shells to run the scripts in the tools folder
@if "%CYGWIN_HOME%" == "" (
  @for /f "tokens=1,2*" %%i in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Cygwin\setup" /v "rootdir"') do (
    @if "%%i" == "rootdir" (
      @set "CYGWIN_HOME=%%k"
    )
  )
)

@if "%CYGWIN_HOME%" == "" (
  @choice /N /m "Cygwin not found, do you want to install it now? [Y/N]"
  @if errorlevel 2 (
    @echo error: Cygwin not found.
    @exit /b 1
  )
  @if errorlevel 1 @call :InstallCygwin
)

@if "%CYGWIN_HOME%\bin\mintty.exe" == "" (
  @choice /N /m "Cygwin terminal (mintty) not found, do you want to install it now? [Y/N]"
  @if errorlevel 2 (
    @echo error: Cygwin terminal ^(mintty^) not found.
    @exit /b 1
  )
  @if errorlevel 1 @call :InstallCygwin "%CYGWIN_HOME%"
)

@if "%CYGWIN_HOME%\bin\bash.exe" == "" (
  @choice /N /m "Cygwin bourne shell (bash) not found, do you want to install it now? [Y/N]"
  @if errorlevel 2 (
    @echo error: Cygwin bourne shell ^(bash^) not found.
    @exit /b 1
  )
  @if errorlevel 1 @call :InstallCygwin "%CYGWIN_HOME%"
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
