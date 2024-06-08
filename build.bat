REM Build script
@ECHO OFF
SetLocal EnableDelayedExpansion

REM Get a list of all the .c files
SET cFilenames=
FOR /R src/ %%f in (*.c) do (
    SET cFilenames=!cFilenames! %%f
)

ECHO "Files:" %cFilenames%

SET assembly=resumator
SET ext=exe
SET cFLAGS=-g3 -Wall -Wextra -Werror -fsanitize=undefined -fsanitize-trap -Wno-unused-variable -Wno-unused-parameter
SET iFLAGS=-Isrc/
SET lFLAGS=-fuse-ld=lld -Wl,--pdb=
SET DEFINES=-DDEBUG

ECHO "Building %assembly%%..."
clang %cFilenames% %cFLAGS% -o ./build/%assembly%.%ext% %DEFINES% %iFLAGS% %lFLAGS%