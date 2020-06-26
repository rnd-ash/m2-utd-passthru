@echo off

if exist "C:\Program Files (x86)\macchina\passthru" echo Driver folder already exists, ignoring
if not exist "C:\Program Files (x86)\macchina\passthru" mkdir "C:\Program Files (x86)\macchina\passthru"

echo Merging registry
regedit.exe /S  "%~dp0\macchina-passthru.reg"

echo Install complete!
pause