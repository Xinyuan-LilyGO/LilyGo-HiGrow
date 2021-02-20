echo off

if "%NANO_PB%" == "" (
    echo 'NANO_PB' environment variable not defined
    goto end
) ELSE (
    echo 'NANO_PB' directory set to %NANO_PB%
)

set nanopb_generator_path=%NANO_PB%\generator\nanopb_generator.py
set protoc_bin_path=%NANO_PB%\generator-bin
set protoc_path=%NANO_PB%\generator\protoc
set nanopb_inc_path=%NANO_PB%\generator\proto
set PATH=%PATH%;%protoc_bin_path%

set filename=%1
IF "%filename%" == "" GOTO empty
echo Filename set to %filename%
echo Making python protos...
mkdir pyprotos
call python -m grpc_tools.protoc --proto_path=protos --python_out=./pyprotos/ %filename%.proto
:: call python -m grpc_tools.protoc --proto_path=protos --proto_path=%nanopb_inc_path% --python_out=./pyprotos/ %filename%.proto :: with nano_pb

echo Making c/h...
mkdir ..\src\protos
call python %nanopb_generator_path% protos\%filename%.proto --output-dir=../src/

goto end

:empty
echo  "make_protos.bat <protoname>    e.g. make_protos.bat lightprogram"

:end