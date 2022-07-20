echo Creating folders

mkdir ..\bin\driver\wfp\windows7\std\i386
mkdir ..\bin\driver\wfp\windows7\std\amd64
mkdir ..\bin\driver\wfp\windows7\wpp\i386
mkdir ..\bin\driver\wfp\windows7\wpp\amd64
mkdir ..\bin\driver\wfp\windows7\demo\i386
mkdir ..\bin\driver\wfp\windows7\demo\amd64
mkdir ..\bin\driver\wfp\windows7\demo_wpp\i386
mkdir ..\bin\driver\wfp\windows7\demo_wpp\amd64

echo ** Building x86 version
cmd /c builddrv.bat free WIN7 -mcz

echo ** Building x64 version
cmd /c builddrv.bat free X64 -mcz

echo ** Copying the binaries

copy std\objfre_win7_x86\i386\netfilter2.sys ..\bin\driver\wfp\windows7\std\i386
copy std\objfre_win7_amd64\amd64\netfilter2.sys ..\bin\driver\wfp\windows7\std\amd64

copy wpp\objfre_win7_x86\i386\netfilter2.sys ..\bin\driver\wfp\windows7\wpp\i386
copy wpp\objfre_win7_amd64\amd64\netfilter2.sys ..\bin\driver\wfp\windows7\wpp\amd64

copy demo\objfre_win7_x86\i386\netfilter2.sys ..\bin\driver\wfp\windows7\demo\i386
copy demo\objfre_win7_amd64\amd64\netfilter2.sys ..\bin\driver\wfp\windows7\demo\amd64

copy demo_wpp\objfre_win7_x86\i386\netfilter2.sys ..\bin\driver\wfp\windows7\demo_wpp\i386
copy demo_wpp\objfre_win7_amd64\amd64\netfilter2.sys ..\bin\driver\wfp\windows7\demo_wpp\amd64
