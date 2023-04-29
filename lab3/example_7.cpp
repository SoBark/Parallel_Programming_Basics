#include <omp.h>
#include <iostream>
using namespace std;
double sum(double* a, int count)
{
if(count == 0)
return 0;
if(count == 1)
return a[0];
//
double res = 0;
double s1 = 0, s2 = 0;
int d = count/2;
// Порождаются задачи, которые распределяются
// между потоками
#pragma omp task shared(s1)
s1 = sum(a, d);
#pragma omp task shared(s2)
s2 = sum(a + d, count - d);
#pragma omp taskwait
//
res += s1 + s2;
//
return res;
}
int main() {
const int a_size = 1000;
double* a = new double[a_size];
// Инициализация массива
#pragma omp parallel for
for(int i = 0; i<a_size; i++)
a[i] = i;
double res = 0;
#pragma omp parallel
// Один из потоков команды порождает задачу
#pragma omp single nowait
res = sum(a, a_size);
cout << "Res = " << res << endl;
//
delete[] a;
//
return 0;
}