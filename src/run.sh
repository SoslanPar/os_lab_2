#!/bin/bash

# Путь к исходникам
SRC_DIR="./src"
EXEC="./main"
THREADS=20   # кол-во потоков (можно менять)
echo Количество потоков: $THREADS
TEST_DIR="../test"

# Замер времени начала
START=$(date +%s%3N)

# Запуск для каждого файла test_*.txt
for file in $TEST_DIR/test_*.txt; do
    echo "=============================="
    echo "Файл: $file"
    $EXEC $(basename $file) $THREADS
done

# Замер времени конца
END=$(date +%s%3N)

# Вывод времени выполнения в миллисекундах
TOTAL_MS=$((END - START))
echo "Общее время для всех тестов: ${TOTAL_MS} ms"