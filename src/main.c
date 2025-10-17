#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <math.h> //  gcc -o main main.c -lm
/*
Функция sqrt() объявлена в <math.h>,
но сама она живёт в отдельной библиотеке — libm (math library).
Поэтому при компиляции нужно добавить ключ -lm, чтобы подключить её.
*/

#include "functions.с"


void* worker_1(void* arg){
    Thread_data* d = (Thread_data*)arg;
    double local_max = 0.0;

    for (int i = d->start_i; i < d->end_i; ++i){
        for (int j = i + 1; j < d->count_points - 1; ++j){
            for (int k = j + 1; k < d->count_points; ++k){
                double area = triangle_area(d->points[i], d->points[j], d->points[k]);
                if (area > local_max) {local_max = area;}
            }
        }
    }
    d->thread_max_area = local_max;
    return NULL;
}

int current_i = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void* worker_2(void* arg){
    Thread_data* data = (Thread_data*)arg;
        double local_max = 0.0;

        while (1) {
            int i;

            // захватываем индекс i
            pthread_mutex_lock(&mutex); // Возвращаемое значение: 0 при успехе, код ошибки при неудаче.
            if (current_i >= data->count_points - 2) {
                pthread_mutex_unlock(&mutex); // Возвращаемое значение: 0 при успехе, ошибка если мьютекс не был захвачен этим потоком.
                break;  // больше i нет
            }
            i = current_i;
            current_i++;
            pthread_mutex_unlock(&mutex); 

            // вычисляем комбинации для текущего i
            for (int j = i+1; j < data->count_points-1; ++j) {
                for (int k = j+1; k < data->count_points; ++k) {
                    double area = triangle_area(data->points[i], data->points[j], data->points[k]);
                    if (area > local_max) local_max = area;
                }
            }
        }

        data->thread_max_area = local_max;
        return NULL;
}

int main(int argc, char *argv[]){
    if (argc < 3){
        printf("Usage: %s {input_file} {max_threads}\n", argv[0]);
        return 1;
    }
    const char* filename = argv[1]; // Имя файла с строками, содержащие координаты x, y, z
    int max_threads;
    
    // sscanf(string, "%d %d %d", &a, &b, &c) строку записываем в три числовые переменные
    // Считываем строку argv[2] - заданное кол-во потоков от пользователя и "считываем" её как число, записывая в max_threads
    if (sscanf(argv[2], "%d", &max_threads) != 1 || max_threads < 0 || max_threads > 20){
        printf("Error: max_threads must be 0 < max_threads <= 20\n");
        return 1;
    };

    pthread_t threads[max_threads]; // pthread_t — Это тип данных, который описывает поток (его идентификатор, дескриптор).
    Point points[1000]; // Массив, содержащий в каждой ячейке класс Point, которые представляет из себя 3 атрибута x, y, z

    const char* base_path = "../test/";
    char full_path[256];
    // sprintf - Записываем строки base_path и filename как "%s%s", то есть соединяем в переменную full_path
    sprintf(full_path, "%s%s", base_path, filename);
    // printf("%s", full_path);

    // Чтение с файла точек в массив
    FILE* file = fopen(full_path, "r");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        exit(1);
    }
    int count_points = 0;
    while (fscanf(file, "%lf %lf %lf", &points[count_points].x, &points[count_points].y, &points[count_points].z) == 3){
        count_points++;
    }
    fclose(file);

    if (count_points < 3)
    {
        fprintf(stderr, "Error: Few points for triangle\n");
        return 1;
    }
    // printf("Count of points: %d\n", count_points);
    // for (int i = 0; i < count_points; ++i){
    //     printf("%.2lf %.2lf %.2lf\n", points[i].x, points[i].y, points[i].z);
    // }

    // ================================== Метод 1 =======================================
    // Замер времени
    struct timespec t_start, t_end;
    double time_worker1, time_worker2;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    int chunk_size = max(1, ceil((count_points - 2) / max_threads));
    Thread_data data[max_threads];
    for (int t = 0; t < max_threads; ++t){
        data[t].points = points;
        data[t].count_points = count_points;
        data[t].start_i = t * chunk_size;
        data[t].end_i = data[t].start_i + chunk_size;
        if (data[t].end_i > count_points - 2) {data[t].end_i = count_points - 2;}

        pthread_create(
            &threads[t],    // Куда сохранить идентификатор потока
            NULL,           // Атрибуты (NULL = по умолчанию, создание потока с настройками по умолчанию)
            worker_1,       // Функция, которую поток должен выполнить
            &data[t]       // аргумент, который передаётся в эту функцию
        );
    }

    double global_max = 0.0;
    for (int t = 0; t < max_threads; ++t){
        pthread_join(threads[t], NULL); // второй аргумент – указатель, куда будет записан возврат из потока. NULL, так как ничего не возвращаем
        if (data[t].thread_max_area > global_max)
        {global_max = data[t].thread_max_area;}
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    time_worker1 = elapsed_ms(t_start, t_end);
    printf("Метод 1 с фиксированным диапазоном (делим массив на части для индекса i):\nМаксимальная площадь = %.2f\nВремя = %.2f ms\n", global_max, time_worker1);

    
    // ================================== Метод 2 ======================================
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    for (int t = 0; t < max_threads; ++t){
        data[t].points = points;
        data[t].count_points = count_points;
        data[t].start_i = t * chunk_size;
        data[t].end_i = data[t].start_i + chunk_size;
        if (data[t].end_i > count_points - 2) {data[t].end_i = count_points - 2;}

        pthread_create(
            &threads[t],    
            NULL,           
            worker_2,       
            &data[t]       
        );
    }
    
    global_max = 0.0;
    for (int t = 0; t < max_threads; ++t){
        pthread_join(threads[t], NULL); 
        if (data[t].thread_max_area > global_max)
        {global_max = data[t].thread_max_area;}
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    time_worker2 = elapsed_ms(t_start, t_end);
    printf("Метод 2 с динамическим распределением (поочередно потоки берут индекс i):\nМаксимальная площадь = %.2f\nВремя = %.2f ms\n", global_max, time_worker2);


    return 0;
}