#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
using namespace std;
/* Функция, которую будет исполнять созданный поток */
struct thread_arg
{
    int first = 0;
    int second = 0;
};

void *thread_job(void *arg)
{
  thread_arg* arguments = (thread_arg*) arg;
cout << "Thread is running..." <<"First argument: "<< arguments->first <<"; Second argument: " << arguments->second <<endl;
}
int main(int argc, char* argv[])
{
if (argc < 3){
    cout<< "Too few arguments. Need at least two.\n";
    exit(1);
} 
int n = (argc < 4) ? 1 : atoi(argv[3]);
int stack_size = (argc < 5)  ? 5*1024*1024 : atoi(argv[4])*1024*1024;
cout << "Количество дополнительных потоков:\t" << n << "\n";
cout << "Минимальный размер стека потоков:\t" << stack_size / (1024*1024) << " mb\n";
// Определяем переменные: идентификатор потока и код ошибки
pthread_t thread;
pthread_attr_t thread_attr; // Атрибуты потока
thread_arg thread_arguments;
thread_arguments.first = atoi(argv[1]);
thread_arguments.second = atoi(argv[2]);

int err;
// Инициализируем переменную для хранения атрибутов потока
err = pthread_attr_init(&thread_attr);
if(err != 0) {
cout << "Cannot create thread attribute: " << strerror(err) << endl;
exit(-1);
}
// Устанавливаем минимальный размер стека для потока (в байтах)
err = pthread_attr_setstacksize(&thread_attr, stack_size);
if(err != 0) {
cout << "Setting stack size attribute failed: " << strerror(err)
<< endl;
exit(-1);
}
// Создаём поток
for (int i=0; i < n; i++)
{
    err = pthread_create(&thread, &thread_attr, thread_job, &thread_arguments);
    // Если при создании потока произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(err != 0) {
    cout << "Cannot create a thread: " << strerror(err) << endl;
    exit(-1);
    }
}
// Ожидаем завершения созданного потока перед завершением
// работы программы
pthread_exit(NULL);
}