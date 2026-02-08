mkdir package
pushd package
del /q *
rem copy ..\README.md .\
rem copy ..\LICENSE .\

for %%x in (COD11MP CODMP CODUOMP JAMP JASP JK2MP JK2SP MOHAA MOHBT MOHSH Q3A QUAKE2 RTCWMP RTCWSP SIN SOF2MP SOF2SP STEF2 STVOYHM STVOYSP WET) do (
    copy ..\bin\Release-%%x\x86\advchat_qmm_%%x.dll .\
    copy ..\bin\Release-%%x\x64\advchat_qmm_x86_64_%%x.dll .\     
)
popd
