
pushd %_CWD% 
set _CWD=%CD% 
popd 

for /f "delims=" %%a in ('"%GIT_BIN_DIR%\git" describe') do @set SPP_VERSION=%%a

set SPP_VERSION="SPP_VERSION=L"%SPP_VERSION%""
msbuild /t:Rebuild /property:Configuration=Release