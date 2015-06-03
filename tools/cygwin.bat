@rem
@rem Description:
@rem   A script for configuring and starting the Cygwin terminal so it is able
@rem   to run the shell scripts in the tools folder.
@rem Author:
@rem   Jan Fiedor
@rem Version:
@rem   1.1
@rem Created:
@rem   03.06.2015
@rem Last Update:
@rem   03.06.2015
@rem

@if "%VS120COMNTOOLS%"=="" (
  echo "error: Visual Studio 2013 not found."
  exit /b 1
)

@call "%VS120COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64

@for /f "tokens=1,2*" %%i in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Cygwin\setup" /v "rootdir"') do (
  @if "%%i"=="rootdir" (
    @set "CYGWIN_HOME=%%k"
  )
)

@if "%CYGWIN_HOME%"=="" (
  echo "error: Cygwin not found."
  exit /b 1
)

@rem If no command is given, just open the terminal, else execute the command
@if [%1] == [] (
  @set BASH_COMMAND=exec bash
) else (
  @set BASH_COMMAND=%*
)

@rem Start a Cygwin bash terminal in the ANaConDA directory
"%CYGWIN_HOME%\bin\mintty.exe" -i /Cygwin-Terminal.ico "%CYGWIN_HOME%\bin\bash.exe" -l -c "cd \"%~dp0/..\"; %BASH_COMMAND%"

@rem End of script
