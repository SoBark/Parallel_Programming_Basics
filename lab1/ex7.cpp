#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <cmath>
using namespace std;
/* Функция, которую будет исполнять созданный поток */
struct thread_arg
{
    double (*func)(double);
    int n; // длина массива
    double* arr; //начало массива
};

void *thread_job(void *arg)
{
  thread_arg* arguments = (thread_arg*) arg;
  int err;
  if (arguments->n <= 0)
    return NULL;
  for (int i =0; i < arguments->n; i++)
  {
    arguments->arr[i] = arguments->func(arguments->arr[i]);
  }
}
int main(int argc, char* argv[])
{
if (argc < 3){
    cout<< "Too few arguments. Need at least two.\n";
    exit(1);
} 
int n_threads = (argc < 3) ? 1 : atoi(argv[2]);
int n_arr = atoi(argv[1]);
cout << "Количество дополнительных потоков:\t" << n_threads << "\n";
// Определяем переменные: идентификатор потока и код ошибки
double *arr = new double[n_arr];
cout << "Исходный массив:" << endl;
for (int i = 0; i < n_arr; i++){
    arr[i] = i;
    cout << arr[i] << endl;
}


int err;
// Создаём поток
int step = (int)std::ceil((n_arr *1.0) / n_threads);
// cout << "step " << step <<endl;
// cout << "n_threads " << n_threads <<endl;
pthread_t* thread = new pthread_t[n_threads];
thread_arg* thread_arguments = new thread_arg[n_threads];
for (int i=0; i < n_threads; i++)
{
    
    thread_arguments[i].func = std::cos;
    thread_arguments[i].n = (n_arr - i*step) <= 0 ? -1: step;
    thread_arguments[i].arr = arr+i*step;
    err = pthread_create(&(thread[i]), NULL, thread_job, &(thread_arguments[i]));
    // Если при создании потока произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(err != 0) {
    cout << "Cannot create a thread: " << strerror(err) << endl;
    exit(-1);
    }
}

for (int i =0; i < n_threads; i++)
{
    int ret = pthread_join(thread[i], NULL);
}
cout << "Измененный массив:" << endl;
for (int i=0; i < n_arr; i++)
    cout << arr[i] << endl;
// Ожидаем завершения созданного потока перед завершением
// работы программы
delete[] thread_arguments;
delete[] arr;
delete[] thread;

pthread_exit(NULL);
}