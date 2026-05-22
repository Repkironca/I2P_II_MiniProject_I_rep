    @echo off
    gcc -w calculator_merged.c -o cm.exe
    gcc -w ../assembly_parser/main.c -o ap.exe
    for /L %%i in (1, 1, 6) do (
        cm.exe < ../assembly_parser/testcase/%%i.txt > temp.txt
        ap.exe < temp.txt
        type output.txt
    )