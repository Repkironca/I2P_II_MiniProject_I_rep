@echo off
gcc -w opt_3.c -o rf.exe
gcc -w ../assembly_parser/main.c -o ap.exe
for /L %%i in (1, 1, 7) do (
	rf.exe < "../assembly_parser/testcase/%%i.txt" > temp_out.txt
	ap.exe < temp_out.txt
	type output.txt
)