
pushd %_CWD% 
set _CWD=%CD% 
popd 

for /f "delims=" %%a in ('"%GIT_BIN_DIR%\git" describe') do @set SPP_VERSION_BARE=%%a

set SPP_VERSION="SPP_VERSION=L"%SPP_VERSION_BARE%""
msbuild /t:Rebuild /property:Configuration=Release

del SPP-%SPP_VERSION_BARE%.zip
zip -j SPP-%SPP_VERSION_BARE%.zip Release\ServerPingPlugin.dll