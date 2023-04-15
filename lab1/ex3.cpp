#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <chrono>
#include <cmath>
using namespace std;
/* Функция, которую будет исполнять созданный поток */
void *thread_job(void *arg)
{
cout << "Thread is running..." << endl;
}
void *thread_job_action(void *arg)
{
    int* n = (int*)arg;
    int k = 10;
    for (int i = 0; i < *n; i++)
        k+=10;
}

// struct thread_input(void *arg)
// {

// }

int main()
{
// Определяем переменные: идентификатор потока и код ошибки
pthread_t thread;
int err, ret;
std::chrono::duration<double> elapsed_seconds;
int n = 1;
double time_start_thread = 1;
// for (int i = 0; i < n; i++)
// {
//     auto start = std::chrono::steady_clock::now();
//     err = pthread_create(&thread, NULL, thread_job, NULL);
//     auto end = std::chrono::steady_clock::now();
//     elapsed_seconds = end-start;
//     ret = pthread_join(thread, NULL);
//     //std::cout << "empty thread: " << elapsed_seconds.count() << "s\n";
//     time_empty_thread += elapsed_seconds.count();
//     // Если при создании потока произошла ошибка, выводим
//     // сообщение об ошибке и прекращаем работу программы
//     if(err != 0) {
//     cout << "Cannot create a thread: " << strerror(err) << endl;
//     exit(-1);
//     }
// }
// time_empty_thread /= n;
// std::cout << "Average time to create thread: " << time_empty_thread << "s\n";
double time_thread = time_start_thread;

//for (int i = 1; time_start_thread / time_thread >= 0.5; i*=10)
cout<< "n\t\tt_creation\tt_all\t\tt_creation/t_all\n";
for (int i = 1; i < std::pow(10, 8) + 1; i*=10)
{
    auto start = std::chrono::steady_clock::now();
    err = pthread_create(&thread, NULL, thread_job_action, &i);
    auto start_thread = std::chrono::steady_clock::now();
    if(err != 0) {
    cout << "Cannot create a thread: " << strerror(err) << endl;
    exit(-1);
    }
    ret = pthread_join(thread, NULL);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds_start_thread = start_thread - start;
    elapsed_seconds = end-start;
    time_thread = elapsed_seconds.count();
    time_start_thread = elapsed_seconds_start_thread.count();
    std::cout << i << (i < 10000000 ? "\t\t" : "\t");
    std::cout << elapsed_seconds_start_thread.count() << "s\t";
    std::cout << elapsed_seconds.count() << "s\t";
    std::cout << time_start_thread / time_thread  << "\n";
}
// Ожидаем завершения созданного потока перед завершением
// работы программы
pthread_exit(NULL);
}