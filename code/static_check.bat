@echo off

pushd code
set Wildcard=*.h *.cpp *.int *.c

echo ------
echo ------

echo STATICS FOUND:
findstr -s -n -i -l "static" %Wildcard%

echo ------
echo ------

echo GLOBALS FOUND:
findstr -s -n -i -l "local_persist" %Wildcard%
findstr -s -n -i -l "global_variable" %Wildcard%

echo ------
echo ------
