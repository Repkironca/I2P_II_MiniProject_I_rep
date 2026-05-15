@echo off
echo === 1. 編譯你的 Compiler ===
:: 加上 -w 關閉所有 Warning
gcc -w calculator_merged.c -o my_calc.exe

echo === 2. 編譯助教的 Assembly Parser ===
:: 加上 ..\ 回到上一層去找助教的資料夾
gcc -w ..\assembly_parser\main.c -o parser_tool.exe

echo === 3. 執行你的 Compiler (生成組語中...) ===
:: 測資路徑也要加上 ..\
my_calc.exe < ..\assembly_parser\testcase\1.txt > input.txt
echo (組語已成功寫入 input.txt)

echo === 4. 執行 Assembly Parser 驗證結果 ===
parser_tool.exe < input.txt > output.txt

echo === 5. 驗證結果 (output.txt) ===
type output.txt

echo.
pause