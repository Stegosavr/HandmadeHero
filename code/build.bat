@echo off

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Oi -Od -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -FC -Z7 -Fmwin32_handmade.map 
set CommonLinkerFlags=-opt:ref user32.lib Gdi32.lib Winmm.lib

:: TODO - can we just build both with one exe?

IF NOT EXIST build mkdir build
pushd build

:: NOTE(grigory) - -subsystem sheisse for winXP compatibility
:: -MTd (instead of -MD) statically compiling c runtime lib to our exe, helps with compatibility

:: 32-bit build
:: cl %CommonCompilerFlags% ..\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags% 

:: 64-bit build
cl %CommonCompilerFlags% ..\code\win32_handmade.cpp /link %CommonLinkerFlags% 
popd
