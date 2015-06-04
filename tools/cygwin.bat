@rem
@rem Description:
@rem   A script for starting and configuring Cygwin shells which are able to
@rem   run the shell scripts (.sh files) in the tools folder. By default, an
@rem   interactive login shell is started inside a Cygwin terminal (mintty).
@rem Author:
@rem   Jan Fiedor
@rem Version:
@rem   1.2
@rem Created:
@rem   03.06.2015
@rem Last Update:
@rem   04.06.2015
@rem

@rem Remember the directory where the script is before processing parameters
@set SCRIPT_DIR=%~dp0

@rem Default values for optional parameters
@set EXECUTION_ENVIRONMENT=terminal

@rem Process the optional parameters
:ProcessNextParameter
@if "%~1" == "" @goto :AllParametersProcessed
@if "%~1" == "--no-terminal" (
  @set EXECUTION_ENVIRONMENT=shell
)
@if "%~1" == "-c" @goto :ProcessShellCommand
@if "%~1" == "--command" @goto :ProcessShellCommand
@shift
@goto :ProcessNextParameter

:ProcessShellCommand
@shift
@set SHELL_COMMAND=%~1
@shift
@goto :ProcessNextParameter

:AllParametersProcessed

@rem Find Visual Studio 2013, can compile ANaConDA and is supported by PIN
@if "%VS120COMNTOOLS%" == "" (
  @echo "error: Visual Studio 2013 not found."
  @exit /b 1
)

@call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64

@rem Find Cygwin, need its shells to run the scripts in the tools folder
@for /f "tokens=1,2*" %%i in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Cygwin\setup" /v "rootdir"') do (
  @if "%%i" == "rootdir" (
    @set "CYGWIN_HOME=%%k"
  )
)

@if "%CYGWIN_HOME%" == "" (
  @echo "error: Cygwin not found."
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
