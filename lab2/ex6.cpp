#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <algorithm>
#include <cctype>
#include <pthread.h>
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <cmath>
#include <chrono>


const std::string VOWELS= "aeyuio";
const int THREAD_COUNT = 9;
const int REPEAT_TIMES = 500000;
struct ReduceInput
{
    int begin;
    int end;
};
struct MapInput
{
    std::vector<std::string> lines;
    int begin;
    int end;
};

std::map<char,int> MapReduce(
    std::vector<std::string> lines,
    void*map(void*),
    void*reduce(void*)
    );
void* Map(void* args);
void* Reduce(void* args);
void* thread_job(void* args);

std::string repeat(int n) {
    std::ostringstream os;
    for(int i = 0; i < n; i++)
        os << "repeat";
    return os.str();
}


int main()
{
    //std::vector<std::string> lines = {
    std::vector<std::string> lines;
    for (int i = 0 ; i < REPEAT_TIMES; i++)
    {
        lines.push_back("How many vocals do");
        lines.push_back("these two lines have");
    }
    auto start_s = std::chrono::steady_clock::now();
    auto results = MapReduce(lines, Map, Reduce);
    for (const auto& kv : results)
        std::cout << kv.first << ": " << kv.second << std::endl;
    auto end_s = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_s-start_s);
    std::cout << "Time for "<< THREAD_COUNT <<" threads: " << elapsed_time.count() << " ms" << std::endl;
    return 0;
}

//enum work

struct thread_input
{
    int work_status; // 0 - wait, 1 - work, -1 - exit
    void* (*func) (void*);
    void* input;
};

void* thread_job (void* args)
{
    //printf("thread    ahahahahah\n");
    thread_input *input = (thread_input*) args;
    input->func(input->input);
    return NULL;
}


std::multimap<char, int> mapResults;
std::map<char, std::vector<int>> reduceSources;
std::map<char,int> reduceResult;
pthread_spinlock_t mx;
//pthread_mutex_t mx;

std::map<char,int> MapReduce(
    std::vector<std::string> lines,
    void*map(void*),
    void*reduce(void*)
    )
{
    pthread_spin_init(&mx, PTHREAD_PROCESS_PRIVATE);
    //pthread_mutex_init(&mx, NULL);
    //std::cout<< "MapReduceStarted" << std::endl;
    int err;
    pthread_t* thread = new pthread_t[THREAD_COUNT];
    thread_input* thread_arguments = new thread_input[THREAD_COUNT];
    int step = (int)std::ceil((lines.size() *1.0) / THREAD_COUNT);
    MapInput* thread_arguments_map = new MapInput[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        thread_arguments_map[i].lines = lines;
        thread_arguments_map[i].begin = i*step;
        thread_arguments_map[i].end = i*step +step < lines.size() ? i*step +step : lines.size();
        thread_arguments[i].func = map;
        thread_arguments[i].input = &(thread_arguments_map[i]);
        //err = pthread_create(&(thread[i]), NULL, thread_job, &(thread_arguments[i]));
        err = pthread_create(&(thread[i]), NULL, thread_job, &(thread_arguments[i]));
        // Если при создании потока произошла ошибка, выводим
        // сообщение об ошибке и прекращаем работу программы
        // std::cout << err << " КОД АШИБКИ\n";
        if(err != 0) {
            std::cout << "Cannot create a thread: " << strerror(err) << std::endl;
            exit(-1);
        }
    }
    for (int i =0; i < THREAD_COUNT; i++)
    {
        int ret = pthread_join(thread[i], NULL);
    }
    //std::cout<< "Mapped. Threads_joined" << std::endl;
    // for (auto sourceItem : lines)
    // {
    //     map(&sourceItem);
    // }
    // for (const auto& kv : mapResults)
    // {
    //     std::cout << kv.first << ": "  << kv.second << std::endl;
    // }

    for (auto letter : VOWELS)
        {
            reduceSources.insert(std::pair<char, std::vector<int> >(letter, std::vector<int>()));
            reduceResult[letter] = 0;
        }
    //std::cout<< "StartToReduce" << std::endl;
    for (const auto& kv : mapResults)
        reduceSources[kv.first].push_back(kv.second);


    ReduceInput* thread_arguments_reduce = new ReduceInput[THREAD_COUNT];
    step = (int)std::ceil((VOWELS.length() *1.0) / THREAD_COUNT);
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        thread_arguments_reduce[i].begin = i*step;
        thread_arguments_reduce[i].end = i*step +step < VOWELS.size() ? i*step +step : VOWELS.size();
        thread_arguments[i].func = reduce;
        thread_arguments[i].input = &(thread_arguments_reduce[i]);
        //err = pthread_create(&(thread[i]), NULL, thread_job, &(thread_arguments[i]));
        err = pthread_create(&(thread[i]), NULL, thread_job, &(thread_arguments[i]));
        // Если при создании потока произошла ошибка, выводим
        // сообщение об ошибке и прекращаем работу программы
        // std::cout << err << " КОД АШИБКИ\n";
        if(err != 0) {
            std::cout << "Cannot create a thread: " << strerror(err) << std::endl;
            exit(-1);
        }
    }

    // ReduceInput * RI = new ReduceInput;
    // RI->begin = 0;
    // RI->end = VOWELS.length();
    // Reduce(RI);

    for (int i =0; i < THREAD_COUNT; i++)
    {
        int ret = pthread_join(thread[i], NULL);
    }
    return reduceResult;
}

void mapCountVowels(void* args)
{
    std::string *s = (std::string*) args;
    std::transform(s->begin(), s->end(), s->begin(),
    [](unsigned char c){ return std::tolower(c); });
    std::map<char, int> vowels;
    for (auto letter : VOWELS)
        vowels[letter] = 0;
    for (auto letter : *s)
    {   
        for (const auto& kv : vowels)
        {
            if (kv.first == letter)
                vowels[letter]++;
        }
    }
    // pthread_mutex_lock(&mx);
    pthread_spin_lock(&mx);
    for (const auto& kv : vowels)
    {
        if (kv.second > 0){
            mapResults.insert(std::pair<char, int>(kv.first, kv.second));
        }
    }
    pthread_spin_unlock(&mx);
    //pthread_mutex_unlock(&mx);
}

void* Map(void* args)
{
    //printf("ahahahahahaha\n");
    MapInput *input = (MapInput*) args;
    for (int i = input->begin; i < input->end; i++)
        mapCountVowels(&(input->lines[i]));
    return NULL;
}



void* Reduce(void* args)
{
    ReduceInput *input = (ReduceInput*) args;
    for (int i = input->begin; i < input->end; i++)
    {
        for (auto num : reduceSources[VOWELS[i]])
        {
            pthread_spin_lock(&mx);
            reduceResult[VOWELS[i]] += num;
            pthread_spin_unlock(&mx);
        }
    }
    //delete input;
    return NULL;
}

