#include <omp.h>
#include <iostream>
using namespace std;
int main()
{
    int num_threads = 8;
    omp_set_num_threads(num_threads);
// Директива OpenMP: объявление параллельной области
#pragma omp parallel
    {
        int size = omp_get_num_threads();
        int id = omp_get_thread_num();
        for (int i = 0; i < size; i++)
        {
            if (i == id)
            {
                cout << "Number of threads = " << size << endl;
                cout << "Hello World from thread " << id << endl;
                cout.flush();
            }
#pragma omp barrier
        }
    }
    return 0;
}