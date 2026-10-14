@echo off
set "ASSET_DIR=%~dp0Assets"
echo Deleting .bin cache files under %ASSET_DIR% ...
del /s /q "%ASSET_DIR%\*.bin"
echo Done.
PAUSE
