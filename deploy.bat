@echo off

set cfg=%1
set rnt=%2
set mod=%3
set algo_outdir=out\algo_%rnt%_%cfg%
set out_dir=..\..\..\cTraderDev\InternalLibs\TargetProcess\%rnt%\%cfg%\

IF /I "%cfg%"=="debug" (
  set algo_debug=true
) ELSE (
  set algo_debug=false
)

if /I "%mod%"=="-q" (
  GOTO :ninja
)

echo Generating project...
call rd %algo_outdir% /s /q
call gn gen --filters="//algo/win:*" --ide=vs %algo_outdir% "--args=is_debug=%algo_debug% target_cpu=\"%rnt%\""

:ninja
echo Compiling...
call ninja -C %algo_outdir% algo/win:broker
call ninja -C %algo_outdir% algo/win:host

echo Publishing...
call xcopy %algo_outdir%\algo* %out_dir% /y /EXCLUDE:deploy_exclude.txt
call xcopy %algo_outdir%\base.* %out_dir% /y /EXCLUDE:deploy_exclude.txt
call xcopy %algo_outdir%\boringssl.* %out_dir% /y /EXCLUDE:deploy_exclude.txt
call xcopy %algo_outdir%\libc++.* %out_dir% /y /EXCLUDE:deploy_exclude.txt
call xcopy %algo_outdir%\absl.* %out_dir% /y /EXCLUDE:deploy_exclude.txt
call xcopy %algo_outdir%\libperfetto.* %out_dir% /y /EXCLUDE:deploy_exclude.txt
call xcopy %algo_outdir%\zlib.* %out_dir% /y /EXCLUDE:deploy_exclude.txt