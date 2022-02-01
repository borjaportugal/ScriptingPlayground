@echo off

set Wildcard=*.h *.cpp *.inl *.c *.json

call :find "TODO"
call :find "STUDY"

goto eof

rem searches for the input parameter
:find
  echo -------
  echo %* FOUND:
  findstr -s -n -i -l %* %Wildcard% | findstr -v External\ | findstr -v Thread\
  echo -------
exit /b 0

:eof

pause
