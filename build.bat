@echo off
SetLocal EnableDelayedExpansion

SET STD=/std:clatest /experimental:c11atomics 
SET MYCFLAGS=/Oy- /Zi /Od

rem MYCFLAGS		+=-fdebug-info-for-profiling
rem MYCFLAGS		+=-flto

del /q /f build\jasb.*
del /q /f build\jasb\*
rem del /q /f build\jasb\TracyClient.obj
del /q /f build\*.pdb

SET DEFINES=/DTRACY_CALLSTACK=30 /DTRACY_ENABLE /DTRACY_SAMPLING_HZ=8000
SET ICFLAGS=/IC:/tracy/public /IC:/tracy/public/tracy /Ibuilder
SET LFLAGS=/DEBUG:FULL /PDB:build\jasb2.pdb /PROFILE
rem SET LFLAGS		+=-flto
rem SET LFLAGS		+=-fuse-ld=lld 
SET MYLIBS=

SET NAME=jasb
SET BUILD_DIR=build
SET OBJ_DIR=%BUILD_DIR%\jasb
SET EXTENSION=.exe
SET OUTPUT=%BUILD_DIR%\%NAME%.exe

SET SRC=builder\jasb.c

SET OBJS=%OBJ_DIR%\jasb.obj

SET TRACY_SRC=src\TracyClient.cpp
SET TRACY_OBJ=%OBJ_DIR%\TracyClient.obj

rem cl %STD% /fsanitize=address /nologo /Fo: "%OBJS%" %MYCFLAGS% /Fd%BUILD_DIR%\%NAME%.pdb %DEFINES% %ICFLAGS% /c /Tc %SRC%
cl %STD% /nologo /Fo: "%OBJS%" %MYCFLAGS% /Fd%BUILD_DIR%\%NAME%.pdb %DEFINES% %ICFLAGS% /c /Tc %SRC%
cl %STD% /nologo /Fo: "%TRACY_OBJ%" %MYCFLAGS% /Fd%BUILD_DIR%\tracy.pdb %DEFINES% %ICFLAGS% /c /Tp %TRACY_SRC%

link /NOLOGO %LFLAGS% %OBJS% %TRACY_OBJ% /OUT:%OUTPUT% /MAP:%BUILD_DIR%\%NAME%.map %MYLIBS%
