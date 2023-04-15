#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstring>
#include <pthread.h>
#include <vector>
#include <dirent.h>
#include <stack>
#include <unistd.h>

#define err_exit(code, str) { std::cerr << str << ": " << strerror(code) \
<< std::endl; exit(EXIT_FAILURE); }

const int THREAD_COUNT = 3;
std::stack<std::string> stack_of_txt_files;
std::stack<std::string> stack_of_directories;
pthread_mutex_t mutex; // Мьютекс
pthread_mutex_t cond_lock; // Мьютекс для условной переменной
pthread_cond_t cond; //условная переменная
pthread_mutex_t cond_lock_job; // Мьютекс для условной переменной
pthread_mutex_t mutex_job; // Мьютекс для условной переменной
pthread_cond_t cond_job; //условная переменная
pthread_barrier_t barrier; //барьер
bool ready = false;
void* search_for_txt_files(void* args);
void search_for_txt_files_dir(std::string path_to_dir);
void* find_substring_in_txt_files(void* args);
void find_substring_in_file(std::string path_to_file, std::string *sub_str);
void* thread_job (void* args);


struct thread_input
{
    int work_status; // 0 - wait, 1 - work, -1 - exit
    void* (*func) (void*);
    void* input;
};

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "Pass as argument path to directory & substring\n";
    }
    int err;
    // Инициализируем барьер
    err = pthread_barrier_init(&barrier, NULL, THREAD_COUNT);
    if(err != 0)
        err_exit(err, "Cannot initialize barrier");
    // Инициализируем мьютекс и условную переменную
    err = pthread_cond_init(&cond, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize condition variable");
    err = pthread_mutex_init(&mutex, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");
    err = pthread_mutex_init(&cond_lock, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");
    err = pthread_cond_init(&cond_job, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize condition variable");
    err = pthread_mutex_init(&mutex_job, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");
    err = pthread_mutex_init(&cond_lock_job, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");


    std:: string path_to_dir = argv[1];
    std:: string sub_str = argv[2];
    pthread_t* thread = new pthread_t[THREAD_COUNT];
    thread_input* thread_arguments = new thread_input[THREAD_COUNT];
    //initialize thread pool
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        //err = pthread_create(&(thread[i]), NULL, thread_job, &(thread_arguments[i]));
        thread_arguments[i].work_status = 1;
        err = pthread_create(&(thread[i]), NULL, thread_job, &(thread_arguments[i]));
        // Если при создании потока произошла ошибка, выводим
        // сообщение об ошибке и прекращаем работу программы
        // std::cout << err << " КОД АШИБКИ\n";
        if(err != 0) {
            std::cout << "Cannot create a thread: " << strerror(err) << std::endl;
            exit(-1);
        }
    }
    
    //search_for_txt_files(path_to_dir);
    //сообщаем потокам рабочую функцию и входные данные
    stack_of_directories.push(path_to_dir);
    std::cout << "Ищем txt файлы" << std::endl;
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        thread_arguments[i].func = search_for_txt_files;
        thread_arguments[i].input = NULL;
    }
    ready = true;
    // Посылаем сигнал потокам
    err = pthread_cond_broadcast(&cond);
    if(err != 0)
        err_exit(err, "Cannot send signal");

    while(ready);
    //сообщаем потокам рабочую функцию и входные данные
    std::cout << "Ищем подстроку в найденных файлах" << std::endl;

    for (int i = 0; i < THREAD_COUNT; i++)
    {
        thread_arguments[i].func = find_substring_in_txt_files;
        thread_arguments[i].input = &sub_str;
    }
    ready = true;
    // Посылаем сигнал потокам
    err = pthread_cond_broadcast(&cond);
    if(err != 0)
        err_exit(err, "Cannot send signal");
    while(ready);
    std::cout << "Завершаем работу" << std::endl;
    //сообщаем потокам, что работы больше не будет
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        thread_arguments[i].work_status = -1;
    }
    ready = true;
    // Посылаем сигнал потокам
    err = pthread_cond_broadcast(&cond);
    if(err != 0)
        err_exit(err, "Cannot send signal");
    for (int i =0; i < THREAD_COUNT; i++)
    {
        int ret = pthread_join(thread[i], NULL);
    }
    // Освобождаем ресурсы, связанные с барьером
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&cond_lock);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex_job);
    pthread_mutex_destroy(&cond_lock_job);
    pthread_cond_destroy(&cond_job);

}


void* thread_job (void* args)
{
    //printf("thread    ahahahahah\n");
    int err;
    int sem_val;
    thread_input *input = (thread_input*) args;
    std::cout<< pthread_self() << " : запустился" << std::endl;
    while (input->work_status != -1)
    {
        std::cout<< pthread_self() << " : ожидаю условную переменную" << std::endl;
        while(!ready) pthread_cond_wait(&cond,&cond_lock);
        std::cout<< pthread_self() << " : прошел условную переменную" << std::endl;
        //do job
        if (input->work_status == -1)
        {
            std::cout<< pthread_self() << " : поток вышел" << std::endl;
            err = pthread_mutex_unlock (&cond_lock);;
            return NULL;
        }
        err = pthread_mutex_unlock (&cond_lock);;
        std::cout<< pthread_self() << " : начал работу" << std::endl;
        input->func(input->input);
        std::cout<< pthread_self() << " : закончил работу" << std::endl;
        //work done
        if(err != 0){
            err_exit(err, "Cannot unlock mutex");}
        err = pthread_barrier_wait(&barrier);
        if((err != 0) && (err != PTHREAD_BARRIER_SERIAL_THREAD))
            err_exit(err, "Cannot wait on barrier");
        ready = false;
    }
    return NULL;
}

void* search_for_txt_files(void*args)
{
    int err;
    while(true)
    {
        err = pthread_mutex_lock(&mutex);
        if(err != 0){
            err_exit(err, "Cannot lock mutex");}
        if (stack_of_directories.empty())
            {
                err = pthread_mutex_unlock(&mutex);
                if(err != 0){
                    err_exit(err, "Cannot unlock mutex");}
                err = pthread_barrier_wait(&barrier);
                if((err != 0) && (err != PTHREAD_BARRIER_SERIAL_THREAD))
                    err_exit(err, "Cannot wait on barrier");
                if(stack_of_directories.empty())
                    return NULL;
            }
        auto dir_name = stack_of_directories.top();
        stack_of_directories.pop();
        err = pthread_mutex_unlock(&mutex);
        if(err != 0){
            err_exit(err, "Cannot unlock mutex");}
        search_for_txt_files_dir(dir_name);
        
    }
}


//void search_for_txt_files(std::string path_to_dir)
void search_for_txt_files_dir(std::string path_to_dir)
{
    auto dirp = opendir(path_to_dir.c_str());
    dirent* dp;
    while((dp = readdir(dirp)) != NULL)
    {
        if(dp->d_type == DT_DIR)
        {
            std::string fname = dp->d_name;
            if(fname != "." && fname != "..")
            {
                int err;
                err = pthread_mutex_lock(&mutex_job);
                if(err != 0){
                    err_exit(err, "Cannot lock mutex");}   
                stack_of_directories.push(path_to_dir + "/" + fname);
                err = pthread_mutex_unlock(&mutex_job);
                if(err != 0){
                    err_exit(err, "Cannot unlock mutex");}
            }
            continue;
        }
        std::string fname = dp->d_name;

        if (fname.find("txt", (fname.length() - 3)) != std::string::npos)
            stack_of_txt_files.push(path_to_dir + "/" + fname);
    }
    return;
}

void* find_substring_in_txt_files (void* args)
{
    int err;
    std::string* sub_str = (std::string*) args;
    //while (!stack_of_txt_files.empty())
    while (true)
    {
        err = pthread_mutex_lock(&mutex);
        if(err != 0){
            err_exit(err, "Cannot lock mutex");}
        if (stack_of_txt_files.empty())
            {
                err = pthread_mutex_unlock(&mutex);
                if(err != 0){
                    err_exit(err, "Cannot unlock mutex");}
                return NULL;
            }
        auto fname = stack_of_txt_files.top();
        stack_of_txt_files.pop();
        err = pthread_mutex_unlock(&mutex);
        if(err != 0){
            err_exit(err, "Cannot unlock mutex");}
        
        find_substring_in_file(fname, sub_str);
    }
    return NULL;
}

void find_substring_in_file(std::string path_to_file, std::string *sub_str)
{
    std::ifstream fileInput(path_to_file);
    if(!fileInput.is_open())
    {
        std::cout << "Unable to open file: " << path_to_file << std::endl;
        return;
    }
    std::string line;
    int line_n = 1;
    while(std::getline(fileInput, line))
    {
        if(line.find(sub_str->c_str(), 0) != std::string::npos)
            std::cout<< "file: " << path_to_file << " \t||\t" << line_n << "\t||\t" << line << std::endl;
        line_n++;
    }
    
    return;

}