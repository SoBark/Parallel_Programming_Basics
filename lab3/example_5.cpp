#include <omp.h>
#include <iostream>
using namespace std;
int main()
{
    int sum = 0;
    const int a_size = 100;
    int a[a_size];
    for (int i = 0; i < 100; i++)
        a[i] = i;
// Часть 1
#pragma omp parallel reduction(+ \
                               : sum)
    {
        int id = omp_get_thread_num();
#pragma omp for
        for (int i = 0; i < a_size; i++)
            sum += a[i];
        cout << "Thread " << id << ", partial sum = " << sum
             << endl;
    }
    cout << "#0 Final sum = " << sum << endl;
    // Часть 2
    sum = 0;
#pragma omp parallel for reduction(+ \
                                   : sum)
    for (int i = 0; i < a_size; i++)
        sum += a[i];
    cout << "#1 Final sum = " << sum << endl;
    //
    return 0;
}