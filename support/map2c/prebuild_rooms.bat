@echo off
if !%1==! goto error

echo Compiling rooms...

pushd .
cd ..\..\..\rooms

REM %%i needed instead of %i for for-loops
REM % ~ n i means filename without extension
REM (and I had to insert spaces, because DOS-batch will evaluate this term even in remarks...)

REM MS-DOS-SET: Beware of spaces around =

set nummap=0
for %%i in (*.map) do echo Compiling %%i... && ..\port\windows\%1\map2c %%i %%~ni.cpp && set /a nummap+=1
set /a nummapx=nummap+1

echo Building header file...
echo // Auto-generated .h file > allrooms.h
echo // DO NOT HAND EDIT >> allrooms.h
echo // Generated by prebuild_rooms.bat >> allrooms.h
echo. >> allrooms.h
echo #define NUM_ALLROOMDEFS %nummap% >> allrooms.h
echo extern const ROOM_DEF *glb_allroomdefs[%nummapx%]; >> allrooms.h

echo Building .cpp file..
echo // Auto-generated .cpp file > allrooms.cpp
echo // DO NOT HAND EDIT >> allrooms.cpp
echo // Generated by prebuild_rooms.bat >> allrooms.cpp
echo. >> allrooms.cpp
echo #include "../map.h" >> allrooms.cpp

for %%i in (*.map) do echo #include "%%~ni.cpp" >> allrooms.cpp
echo. >> allrooms.cpp

echo const ROOM_DEF *glb_allroomdefs[%nummapx%] = >> allrooms.cpp
echo { >> allrooms.cpp

REM ^ is the batch escape character, & wouldn't be printed

for %%i in (*.map) do echo   ^&glb_%%~ni_roomdef, >> allrooms.cpp
echo   0 >> allrooms.cpp
echo }; >> allrooms.cpp

popd
goto exit

:error
echo Usage: prebuild_bitmaps.bat (Debug^|Release)
:exit