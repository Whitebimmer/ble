setlocal EnableDelayedExpansion

for /f %%i in ('dir /ad/b') do (
	set /p create="create makefile in "%%i" (y,n)?"
	if !create!==y (
		set /p create_lib="create lib (y,n)?"
		cd %%i
		call :create_makefile !create_lib!
		cd ..
	)
)

exit


:create_makefile

echo obj_S = \>Makefile
@for /r %%i in (*.S) do  (
	set xx=%%i
	set file=!xx:%CD%\=!
	set file=!file:\=/!
	set file=!file:~0,-1!
	echo    !file!o \>>Makefile
)

echo. >>Makefile

echo objs = \>>Makefile

@for /r %%i in (*.c) do  (
	set xx=%%i
	set file=!xx:%CD%\=!
	set file=!file:\=/!
	set file=!file:~0,-1!
	echo    !file!o \>>Makefile
)

echo. >>Makefile

echo includes += \>>Makefile
for /f %%i in ('dir /ad/b/s') do (
	set xx=%%i
	set dirn=!xx:%CD%\=!
	set dirn=!dirn:\=/!
	echo -I!dirn! \>>Makefile
)

echo. >>Makefile

echo. >>Makefile

echo DEFINS +=>>Makefile

echo CONFIG_LIB=%1>>Makefile
	 
echo -include $(TOOLS)/rule.mk>>Makefile

echo. >>Makefile

goto :EOF

