echo Creating folders

mkdir ..\bin\driver\tdi\std\i386
mkdir ..\bin\driver\tdi\std\amd64
mkdir ..\bin\driver\tdi\wpp\i386
mkdir ..\bin\driver\tdi\wpp\amd64
mkdir ..\bin\driver\tdi\demo\i386
mkdir ..\bin\driver\tdi\demo\amd64
mkdir ..\bin\driver\tdi\demo_wpp\i386
mkdir ..\bin\driver\tdi\demo_wpp\amd64

echo ** Building x86 version
cmd /c builddrv.bat free WXP -mcz

echo ** Building x64 version
cmd /c builddrv.bat free AMD64 -mcz WNET

echo ** Copying the binaries

copy std\objfre_wxp_x86\i386\netfilter2.sys ..\bin\driver\tdi\std\i386
copy std\objfre_wnet_amd64\amd64\netfilter2.sys ..\bin\driver\tdi\std\amd64

copy wpp\objfre_wxp_x86\i386\netfilter2.sys ..\bin\driver\tdi\wpp\i386
copy wpp\objfre_wnet_amd64\amd64\netfilter2.sys ..\bin\driver\tdi\wpp\amd64

copy demo\objfre_wxp_x86\i386\netfilter2.sys ..\bin\driver\tdi\demo\i386
copy demo\objfre_wnet_amd64\amd64\netfilter2.sys ..\bin\driver\tdi\demo\amd64

copy demo_wpp\objfre_wxp_x86\i386\netfilter2.sys ..\bin\driver\tdi\demo_wpp\i386
copy demo_wpp\objfre_wnet_amd64\amd64\netfilter2.sys ..\bin\driver\tdi\demo_wpp\amd64
