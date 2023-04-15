#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
using namespace std;
#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
<< endl; exit(EXIT_FAILURE); }
//#define m_on 0
const int TASKS_COUNT = 10;
int task_list[TASKS_COUNT]; // Массив заданий

struct thread_input
{
    int thread_num;
};

int current_task = 0; // Указатель на текущее задание
pthread_mutex_t mutex; // Мьютекс
pthread_cond_t cond; // Условная переменная
void do_task(int task_no)
{
    //int num = 2;
    int result = 0;
    for (int i = 0; i < 123456789; i++) result += task_list[task_no];
}
void *thread_job(void *arg)
{
    thread_input* input =  (thread_input*) arg;
    int task_no;
    int err;
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
        current_task++;
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
            printf("Поток %d выполняет %d задание.\n", input->thread_num, task_no);
            do_task(task_no);}
        else{
            printf("Поток %d завершает работу.\n", input->thread_num);
            delete input;
            return NULL;}
    }
}

void *thread_job_cond(void *arg)
{
    int err;
    // Захватываем мьютекс и ожидаем появления товаров на складе
    err = pthread_mutex_lock(&mutex);
    if(err != 0)
        err_exit(err, "Cannot lock mutex");
    printf("Поток с условной переменной зашел в критическую секцию.\n");
    err = pthread_cond_wait(&cond, &mutex);
    if(err != 0)
        err_exit(err, "Cannot wait on condition variable");
    printf("Поток с условной переменной вернулся в критическую секцию. Остальные потоки завершили работу.\n");
}

int main()
{
    pthread_t thread1, thread2, thread_cond; // Идентификаторы потоков
    int err; // Код ошибки
    thread_input* input;
    // Инициализируем массив заданий случайными числами
    for(int i=0; i<TASKS_COUNT; ++i)
        task_list[i] = rand() % TASKS_COUNT;
    // Инициализируем мьютекс и условную переменную
    err = pthread_cond_init(&cond, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize condition variable");
    err = pthread_mutex_init(&mutex, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");
    // Создаём потоки
    err = pthread_create(&thread_cond, NULL, thread_job_cond, NULL);
    if(err != 0)
        err_exit(err, "Cannot create thread 1");
    input = new thread_input;
    input->thread_num = 1;
    err = pthread_create(&thread1, NULL, thread_job, input);
    if(err != 0)
        err_exit(err, "Cannot create thread 1");
    input = new thread_input;
    input->thread_num = 2;
    err = pthread_create(&thread2, NULL, thread_job, input);
    if(err != 0)
        err_exit(err, "Cannot create thread 2");
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    pthread_cond_signal(&cond);

    pthread_join(thread_cond, NULL);

    // Освобождаем ресурсы, связанные с мьютексом
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

}