@echo off
set "ROOT_DIR=%~dp0"
set "ASSET_DIR=%ROOT_DIR%Assets"
echo Deleting .bin cache files under %ASSET_DIR% ...
del /s /q "%ASSET_DIR%\*.bin"

for %%D in (Build Intermediate) do (
    if exist "%ROOT_DIR%%%D" (
        echo Deleting %ROOT_DIR%%%D ...
        rmdir /s /q "%ROOT_DIR%%%D"
    )
)
echo Done.
PAUSE
