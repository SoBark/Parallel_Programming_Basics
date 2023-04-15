#include <pthread.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstring>


#define err_exit(code, str) { std::cerr << str << ": " << strerror(code) \
<< std::endl; exit(EXIT_FAILURE); }




pthread_mutex_t mutex;

void* thread_job(void* args)
{
    sleep(1);
    pthread_mutex_unlock(&mutex);
    return NULL;
}



int main()
{
    int err;
    pthread_t thread;
    pthread_mutex_init(&mutex, NULL);
    std::cout << "Inited mutex" << std::endl;
    pthread_mutex_lock(&mutex);
    std::cout << "Going to deadlock myself" << std::endl;
    pthread_create(&thread, NULL, thread_job, NULL);
    err = pthread_mutex_lock(&mutex); /* protect the queue */
        if(err != 0){
            err_exit(err, "Cannot lock mutex");}
    pthread_mutex_destroy(&mutex);
    std::cout << "err" << err << std::endl;

}