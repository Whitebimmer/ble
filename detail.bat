@echo off
setlocal EnableDelayedExpansion

set output_dir=e:\detail
set temp_dir=e:\detail\temp

if not exist %output_dir% mkdir %output_dir% 
if not exist %temp_dir% mkdir %temp_dir% 


set /p pic="picture 1: "
if !pic!x!=x copy !pic! %output_dir%\01.jpg

set /p pic="base: "
width400.exe !pic!

call:wait_completed %temp_dir%\1.jpg
copy %temp_dir%\1.jpg  %output_dir%\03.jpg
del %temp_dir%\*.jpg


:size730
set /p pic="show: "
730.exe !pic!
add_magin.exe %output_dir%\!pic!
set /p pic="continue? y:n  : "
if !pic!==y goto size730












exit

:wait_completed
if not exist %1 goto wait_completed
goto :EOF


