#!/bin/bash

# Путь к исходникам
SRC_DIR="./src"
EXEC="./main"
THREADS=4   # кол-во потоков (можно менять)

# Компиляция
echo "Компиляция main.c..."

# Папка с тестовыми файлами
TEST_DIR="../test"

# Запуск для каждого файла test_*.txt
for file in $TEST_DIR/test_*.txt; do
    echo "=============================="
    echo "Файл: $file"
    $EXEC $(basename $file) $THREADS
done
