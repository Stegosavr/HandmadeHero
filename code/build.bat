@echo off

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Oi -Od -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_WIN32=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib Winmm.lib

:: TODO - can we just build both with one exe?

IF NOT EXIST build mkdir build
pushd build

:: NOTE(grigory) - -subsystem sheisse for winXP compatibility
:: -MTd (instead of -MD) statically compiling c runtime lib to our exe, helps with compatibility

set vsPDBFix=-PDB:handmade_%random%.pdb

:: 32-bit build
:: cl %CommonCompilerFlags% ..\code\win32_handmade.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags% 

:: 64-bit build
cl %CommonCompilerFlags% ..\code\handmade.cpp -Fmhandmade.map -LD /link -incremental:no -opt:ref -EXPORT:GameGetSoundSamples -EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% ..\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags% 
popd
