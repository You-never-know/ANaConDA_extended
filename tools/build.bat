@rem
@rem Description:
@rem   A script simplifying building of the ANaConDA framework and its parts.
@rem Author:
@rem   Jan Fiedor
@rem Version:
@rem   1.1
@rem Created:
@rem   03.06.2015
@rem Last Update:
@rem   04.06.2015
@rem

@rem Launch the build script in a Cygwin environment
@call "%~dp0/cygwin.bat" --no-terminal -c "tools/build.sh %*"

@rem End of script
