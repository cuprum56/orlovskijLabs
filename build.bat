@echo off

cl /nologo /W4 /WX /std:c11 /D_CRT_SECURE_NO_WARNINGS /Isrc/include ^
    /Fe:parent.exe src\parent.c src\win_utils.c || exit /b 1

cl /nologo /W4 /WX /std:c11 /D_CRT_SECURE_NO_WARNINGS /Isrc/include ^
    /Fe:child.exe src\child.c src\win_utils.c || exit /b 1

echo Build finished: parent.exe, child.exe
