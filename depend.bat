@echo off
setlocal EnableDelayedExpansion

echo %1>>m.txt 
echo %2>>m.txt
echo %3>>m.txt

if exist %1.1 (
	set /p=%1 %2<nul > %1
	for /f "tokens=*" %%i in (%1.1) do echo %%i>> %1
	del %3.1
)
