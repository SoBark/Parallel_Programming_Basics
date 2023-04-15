#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
using namespace std;
#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
<< endl; exit(EXIT_FAILURE); }
//#define m_on 0
const int TASKS_COUNT = 100;
const int THREAD_COUNT = 100; // Число потоков 
int task_list[TASKS_COUNT]; // Массив заданий

struct thread_input
{
    int thread_num;
};

int current_task = 0; // Указатель на текущее задание
pthread_mutex_t mutex; // Мьютекс
pthread_spinlock_t lock; // Спинлок
void do_task(int task_no)
{
    //int num = 2;
    int result = 0;
    for (int i = 0; i < 10000000; i++) result += task_list[0];
}
void *thread_job_mutex(void *arg)
{
    thread_input* input =  (thread_input*) arg;
    int task_no;
    int err;
    int sum = 2;
    // Перебираем в цикле доступные задания
    while(true) {
        // Захватываем мьютекс для исключительного доступа
        // к указателю текущего задания (переменная
        // current_task)
        err = pthread_mutex_lock(&mutex);
        if(err != 0){
            delete input;
            err_exit(err, "Cannot lock mutex");}
        // Запоминаем номер текущего задания, которое будем исполнять
        task_no = current_task;
        // Сдвигаем указатель текущего задания на следующее
        //sleep(rand()%2);
        current_task++;
        sum = 2;
        for (int i = 0; i < 1000000 ; i++)
        {
            sum += task_list[0];
        }
        // Освобождаем мьютекс
        err = pthread_mutex_unlock(&mutex);
        if(err != 0){
            delete input;
            err_exit(err, "Cannot unlock mutex");}
        // Если запомненный номер задания не превышает
        // количества заданий, вызываем функцию, которая
        // выполнит задание.
        // В противном случае завершаем работу потока
        if(task_no < TASKS_COUNT){
            //printf("Поток %d выполняет %d задание.\n", input->thread_num, task_no);
            do_task(task_no);
            }
        else{
            //printf("Поток %d завершает работу.\n", input->thread_num);
            delete input;
            return NULL;
            }
    }
}

void *thread_job_spinlock(void *arg)
{
    thread_input* input =  (thread_input*) arg;
    int task_no;
    int err;
    int sum = 2;
    // Перебираем в цикле доступные задания
    while(true) {
        // Захватываем спинлок для исключительного доступа
        // к указателю текущего задания (переменная
        // current_task)
        err = pthread_spin_lock(&lock);
        if(err != 0){
            delete input;
            err_exit(err, "Cannot lock spinlock");}
        // Запоминаем номер текущего задания, которое будем исполнять
        task_no = current_task;
        // Сдвигаем указатель текущего задания на следующее
        //sleep(rand()%2);
        current_task++;
        sum = 2;
        for (int i = 0; i < 1000000 ; i++)
        {
            sum += task_list[0];
        }
        // Освобождаем спинлок
        err = pthread_spin_unlock(&lock);
        if(err != 0){
            delete input;
            err_exit(err, "Cannot unlock spinlock");}
        // Если запомненный номер задания не превышает
        // количества заданий, вызываем функцию, которая
        // выполнит задание.
        // В противном случае завершаем работу потока
        if(task_no < TASKS_COUNT){
            //printf("Поток %d выполняет %d задание.\n", input->thread_num, task_no);
            do_task(task_no);
            }
        else{
            //printf("Поток %d завершает работу.\n", input->thread_num);
            delete input;
            return NULL;
            }
    }
}

int main()
{
    const int EXEC_COUNT = 10; // Число измерений
    pthread_t threads[THREAD_COUNT]; // Идентификаторы потоков
    int err; // Код ошибки
    thread_input* input;
    double time_mutex = 0;
    double time_spinlock = 0;
    //std::chrono::duration<double> elapsed_seconds;
    // Инициализируем массив заданий случайными числами
    for(int i=0; i<TASKS_COUNT; ++i)
        task_list[i] = rand() % TASKS_COUNT;
    for (int j; j < EXEC_COUNT; j++){

        current_task = 0;
        //spinlock
        auto start_s = std::chrono::steady_clock::now();
        err = pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);
        if(err != 0)
            err_exit(err, "Cannot initialize spinlock");
        // Создаём потоки
        for (int i=0; i<THREAD_COUNT; i++){
            input = new thread_input;
            input->thread_num = i;
            err = pthread_create(&threads[i], NULL, thread_job_spinlock, input);
            if(err != 0)
                err_exit(err, "Cannot create thread");
        }
        for (int i =0; i < THREAD_COUNT; i++)
            pthread_join(threads[i], NULL);
        // Освобождаем ресурсы, связанные с мьютексом
        pthread_spin_destroy(&lock);
        auto end_s = std::chrono::steady_clock::now();
        auto elapsed_time = chrono::duration_cast<std::chrono::milliseconds>(end_s-start_s);
        time_spinlock += elapsed_time.count();

        // Инициализируем мьютекс
        current_task = 0;
        auto start = std::chrono::steady_clock::now();
        err = pthread_mutex_init(&mutex, NULL);
        if(err != 0)
            err_exit(err, "Cannot initialize mutex");
        // Создаём потоки
        for (int i=0; i<THREAD_COUNT; i++){
            input = new thread_input;
            input->thread_num = i;
            err = pthread_create(&threads[i], NULL, thread_job_mutex, input);
            if(err != 0)
                err_exit(err, "Cannot create thread");
        }
        for (int i =0; i < THREAD_COUNT; i++)
            pthread_join(threads[i], NULL);
        // Освобождаем ресурсы, связанные с мьютексом
        pthread_mutex_destroy(&mutex);
        auto end = std::chrono::steady_clock::now();
        elapsed_time = chrono::duration_cast<std::chrono::milliseconds>(end-start);
        time_mutex += elapsed_time.count();
    }
    std::cout << "Mutex: " << time_mutex / EXEC_COUNT << " ms" << std::endl;
    std::cout << "Spinlock: " << time_spinlock / EXEC_COUNT << " ms" << std::endl;
    return 0;
}