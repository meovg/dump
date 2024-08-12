::convert all avif image files from a subdirectory into png

::shut up twat 
@echo off

::create output directory
set odir=%1_%2
mkdir %odir%

::convert every avif image
::also tell ffmpeg to stop littering the console
::and shut the fuck up
for %%I in (%1/*.avif) do (
	set temp=%%~nI
	call ffmpeg -v quiet -stats -i %1/%%I %odir%/%%temp:~0,7%%.%2
)