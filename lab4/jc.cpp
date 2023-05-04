#include <stdio.h>
#include <string.h>
#include <iostream>
#include <cmath>
#include "mpi.h"

using namespace std;

const double a = pow(10, 5);

const double eps = 1.0e-8;

/*  Начальное приближение */
const double phi0 = 0.0;

// Границы области
const double x_start = -1.0;
const double x_end = 1.0;

const double y_start = -1.0;
const double y_end = 1.0;

const double z_start = -1.0;
const double z_end = 1.0;

// Размеры области
const double Dx = x_end - x_start;
const double Dy = y_end - y_start;
const double Dz = z_end - z_start;

// Количество узлов сетки
const int Nx = 10;
const int Ny = 10;
const int Nz = 10;

// Размеры шага на сетке
const double hx = Dx / (Nx - 1);
const double hy = Dy / (Ny - 1);
const double hz = Dz / (Nz - 1);

inline double phi(double x, double y, double z)
{return pow(x, 2) + pow(y, 2) + pow(z, 2);}

inline double rho(double phiValue)
{return 6.0 - a * phiValue;}

const int ROOT_PROC = 0;


const int LOWER_PROC_TAG = 100;
const int UPPER_PROC_TAG = 101;
const int X_LOCAL_START_TAG = 102;
const int X_LOCAL_END_TAG = 103;
const int LOWER_BOUND_TAG = 104;
const int UPPER_BOUND_TAG = 105;


/* Получение координаты узла */
inline double Node(double start, double step, int index)
{return start + step * index;}

void Inic(double***& grid, int size, double start)
{
	// Создаём массив и заполняем начальными значениями
	grid = new double** [size];
	for (int i = 0; i < size; i++) {
		grid[i] = new double* [Ny];

		for (int j = 0; j < Ny; j++) {
			grid[i][j] = new double[Nz];

			for (int k = 1; k < Nz - 1; k++) {
				grid[i][j][k] = phi0;
			}
		}
	}

	// Записываем краевые значения
	double xCurr;
	for (int i = 0; i < size; i++) {
		xCurr = Node(start, hx, i);

		for (int k = 0; k < Nz; k++) {
			// При j = 0
			grid[i][0][k] = phi(xCurr, y_start, Node(z_start, hz, k));
		}

		for (int k = 0; k < Nz; k++) {
			// При j = Ny - 1
			grid[i][Ny - 1][k] = phi(xCurr, y_end, Node(z_start, hz, k));
		}

		double yCurr;
		for (int j = 1; j < Ny - 1; j++) {
			yCurr = Node(y_start, hy, j);

			// При k = 0
			grid[i][j][0] = phi(xCurr, yCurr, z_start);

			// При k = Nz - 1
			grid[i][j][Nz - 1] = phi(xCurr, yCurr, z_end);
		}
	}
}

void deleteGrid(double*** grid, int size)
{
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < Ny; j++) {
			delete[] grid[i][j];
		}
		delete[] grid[i];
	}
	delete[] grid;
}

// Считаем точность, как максимальное значение модуля отклонения
// от истинного значения функции
double getError(double*** grid, int size, double start, int rootProcRank)
{
	// Значение ошибки на некотором узле
	double currErr;
	// Максимальное значение ошибки данного процесса
	double maxLocalErr = 0.0;

	for (int i = 1; i < size - 1; i++) {
		for (int j = 1; j < Ny - 1; j++) {
			for (int k = 1; k < Nz - 1; k++) {
				currErr = abs(grid[i][j][k] -
					phi(Node(start, hx, i), Node(y_start, hy, j), Node(z_start, hz, k)));

				if (currErr > maxLocalErr) {
					maxLocalErr = currErr;
				}
			}
		}
	}

	// Максимальное значение ошибки по всем процессам
	double absoluteMax = -1;
	MPI_Reduce((void*)&maxLocalErr, (void*)&absoluteMax, 1, MPI_DOUBLE, MPI_MAX, rootProcRank, MPI_COMM_WORLD);

	return absoluteMax;
}

void Jacobi(double***& grid1, int size, int start, int lowerProcRank, int upperProcRank)
{
	MPI_Request arr_recv[4];// send_request_child, send_request_parent, recv_request_child, recv_request_parent;
	int rank;
	int number_of_process;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &number_of_process);

	MPI_Status status;

	// Значение сходимости для некоторого узла сетки
	double currConverg;
	// Максимальное значение сходимости по всем узлам на некоторой итерации
	double maxLocalConverg;

	// Флаг, показывающий, является ли эпсилон меньше любого значения сходимости для данного процесса
	bool isEpsilonLower = true;

	const double hx2 = pow(hx, 2);
	const double hy2 = pow(hy, 2);
	const double hz2 = pow(hz, 2);

	// Константа, вынесенная за скобки
	double c = 1 / ((2 / hx2) + (2 / hy2) + (2 / hz2) + a);

	// Второй массив для того, чтобы использовать значения предыдущей итерации
	double*** grid2;
	// Просто копируем входной массив
	grid2 = new double** [size];
	for (int i = 0; i < size; i++) {
		grid2[i] = new double* [Ny];

		for (int j = 0; j < Ny; j++) {
			grid2[i][j] = new double[Nz];

			for (int k = 0; k < Nz; k++) {
				grid2[i][j][k] = grid1[i][j][k];
			}
		}
	}

	// Указатель на массив, из которого на некоторой итерации
	// берутся значения для расчёта
	double*** currentSourcePtr = grid1;
	// Указатель на массив, в который на некоторой итерации
	// Записываются новые значения
	double*** currentDestPtr = grid2;
	// Вспомогательный указатель для перемены указателей на массивы
	double*** tmpPtr;

	int messageLength = (Ny - 2) * (Nz - 2);
	
	double* messageBufSend_1 = new double[messageLength];
	double* messageBufSend_2 = new double[messageLength];

	double* messageBufReqv_1 = new double[messageLength];
	double* messageBufReqv_2 = new double[messageLength];

	// Флаг, который показывает, что нужно продолжать вычисления 
	bool loopFlag = true;

	MPI_Request requests[4];

	while (loopFlag) {
		maxLocalConverg = 0.0;

		// Сначала вычисляем граничные значения
		// При i = 1
		for (int j = 1; j < Ny - 1; j++) {
			for (int k = 1; k < Nz - 1; k++) {
				// Первая дробь в скобках
				currentDestPtr[1][j][k] = (currentSourcePtr[2][j][k] + currentSourcePtr[0][j][k]) / hx2;

				// Вторая дробь в скобках
				currentDestPtr[1][j][k] += (currentSourcePtr[1][j + 1][k] + currentSourcePtr[1][j - 1][k]) / hy2;

				// Третья дробь в скобках
				currentDestPtr[1][j][k] += (currentSourcePtr[1][j][k + 1] + currentSourcePtr[1][j][k - 1]) / hz2;

				// Остальная часть вычисления нового значения для данного узла
				currentDestPtr[1][j][k] -= rho(currentSourcePtr[1][j][k]);
				currentDestPtr[1][j][k] *= c;

				// Сходимость для данного узла
				currConverg = abs(currentDestPtr[1][j][k] - currentSourcePtr[1][j][k]);
				if (currConverg > maxLocalConverg) {
					maxLocalConverg = currConverg;
				}
			}
		}

		// Если процесс должен отправить свой крайний слой с младшим значением x (не содержит слоя с x = 0)
		if (lowerProcRank != -1) {
			for (int j = 0; j < Ny - 2; j++) {
				for (int k = 0; k < Nz - 2; k++) {
					messageBufSend_1[(Ny - 2) * j + k] = currentDestPtr[1][j + 1][k + 1];
				}
			}
			// Отправляем слой младшему процессу
			MPI_Isend((void*)messageBufSend_1, messageLength, MPI_DOUBLE, lowerProcRank, UPPER_BOUND_TAG, MPI_COMM_WORLD, &requests[0]);
			MPI_Irecv((void*)messageBufReqv_1, messageLength, MPI_DOUBLE, lowerProcRank, LOWER_BOUND_TAG, MPI_COMM_WORLD, &requests[2]);
		}

		// Если процесс должен отправить свой крайний слой со старшим значением x (не содержит слоя с x = Nx - 1)
		if (upperProcRank != -1) {
			for (int j = 0; j < Ny - 2; j++) {
				for (int k = 0; k < Nz - 2; k++) {
					messageBufSend_2[(Ny - 2) * j + k] = currentDestPtr[size - 2][j + 1][k + 1];
				}
			}
            
			// Отправляем слой старшему процессу
			MPI_Isend((void*)messageBufSend_2, messageLength, MPI_DOUBLE, upperProcRank, LOWER_BOUND_TAG, MPI_COMM_WORLD, &requests[1]);
			MPI_Irecv((void*)messageBufReqv_2, messageLength, MPI_DOUBLE, upperProcRank, UPPER_BOUND_TAG, MPI_COMM_WORLD, &requests[3]);
		}
		

		for (int i = 2; i < size - 1; i++) {
			for (int j = 1; j < Ny - 1; j++) {
				for (int k = 1; k < Nz - 1; k++) {

					// Первая дробь в скобках
					currentDestPtr[i][j][k] = (currentSourcePtr[i + 1][j][k] + currentSourcePtr[i - 1][j][k]) / hx2;

					// Вторая дробь в скобках
					currentDestPtr[i][j][k] += (currentSourcePtr[i][j + 1][k] + currentSourcePtr[i][j - 1][k]) / hy2;

					// Третья дробь в скобках
					currentDestPtr[i][j][k] += (currentSourcePtr[i][j][k + 1] + currentSourcePtr[i][j][k - 1]) / hz2;

					// Остальная часть вычисления нового значения для данного узла
					currentDestPtr[i][j][k] -= rho(currentSourcePtr[i][j][k]);
					currentDestPtr[i][j][k] *= c;

					// Сходимость для данного узла
					currConverg = abs(currentDestPtr[i][j][k] - currentSourcePtr[i][j][k]);
					if (currConverg > maxLocalConverg) {
						maxLocalConverg = currConverg;
					}

				}
			}
		}

		if (maxLocalConverg < eps) {
			isEpsilonLower = false;
		}

		// Применяем логичекую операцию ИЛИ над флагом сходимости между всеми процессами и помещаем результат во флаг цикла.
		// Таким образом, цикл завершится, когда у всех процессов сходимость будет меньше, чем эпсилон
		MPI_Reduce((void*)&isEpsilonLower, (void*)&loopFlag, 1, MPI_C_BOOL, MPI_LOR, ROOT_PROC, MPI_COMM_WORLD);
		MPI_Bcast((void*)&loopFlag, 1, MPI_C_BOOL, ROOT_PROC, MPI_COMM_WORLD);

		// Меняем местами указатели на массив-источник и массив-приёмник
		tmpPtr = currentSourcePtr;
		currentSourcePtr = currentDestPtr;
		currentDestPtr = tmpPtr;

		MPI_Status status;

		if (lowerProcRank != -1)
		{
			MPI_Wait(&requests[0], &status);
		}
		
		if (upperProcRank != -1)
		{
			MPI_Wait(&requests[1], &status);
		}

		if (lowerProcRank != -1)
		{
			MPI_Wait(&requests[2], &status);
			for (int j = 0; j < Ny - 2; j++) {
				for (int k = 0; k < Nz - 2; k++) {
					currentDestPtr[0][j + 1][k + 1] = messageBufReqv_1[(Ny - 2) * j + k];
				}
			}
		}
		if (upperProcRank != -1)
		{
			MPI_Wait(&requests[3], &status);
			for (int j = 0; j < Ny - 2; j++) {
				for (int k = 0; k < Nz - 2; k++) {
					currentDestPtr[size - 1][j + 1][k + 1] = messageBufReqv_2[(Ny - 2) * j + k];
				}
			}
		}
		// MPI_Barrier(MPI_COMM_WORLD);
	}


	// В итоге массив должен содержать значения последней итерации
	if (currentSourcePtr == grid1) {
		deleteGrid(grid2, size);
	}
	else {
		tmpPtr = grid1;
		grid1 = currentSourcePtr;
		deleteGrid(tmpPtr, size);
	}

	delete[] messageBufSend_1;
	delete[] messageBufSend_2;
	delete[] messageBufReqv_1;
	delete[] messageBufReqv_2;
}

int main(int argc, char** argv)
{
	// Количество процессов
	int procCount;

	// Ранг текущего процесса
	int myRank;

	// Ранг младшего процесса для текущего
	// (Содержит слой, который необходим для просчёта локального слоя с младшим по x значением)
	// Если -1, то такого процесса нет
	int myLowerProcRank;
	// Ранг старшего процесса для текущего
	// (Содержит слой, который необходим для просчёта локального слоя со старшим по x значением)
	// Если -1, то такого процесса нет
	int myUpperProcRank;

	// Глобальный индекс по x, с которого начинается область текущего процесса (включительно)
	int MyStart;
	// Глобальный индекс по x, на котором заканчивается область текущего процесса (включительно)
	int MyEnd;

	// Статус возврата
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &procCount);

	// Если процессов больше, чем внутренних слоёв
	if (procCount > (Nx - 2)) {
		if (myRank == ROOT_PROC) {
			std::cout << "Too many processes!" << std::endl;
		}

		MPI_Abort(MPI_COMM_WORLD, 0);
	}

	if (myRank == ROOT_PROC) {
		std::cout << "Proccess initiated." << std::endl;

		int integerPart = (Nx - 2) / procCount;
		int remainder = (Nx - 2) % procCount;

		// Размер партии (сразу вычисляем для главного процесса)
		int batchSize = integerPart + ((myRank < remainder) ? 1 : 0);


		myLowerProcRank = -1;
		myUpperProcRank = procCount == 1 ? -1 : 1;

		MyStart = 1;
		MyEnd = MyStart + batchSize - 1;

		// Младший процесс, для процесса-получателя
		int lowerProcRank;
		// Старший процесс, для процесса-получателя
		int upperProcRank;

		// Начало области для процесса-получателя
		int startIndex = 1;
		// Конец области для процесса-получателя
		int endIndex;

		// Рассылаем всем процессам необходимую информацию
		for (int destRank = 1; destRank < procCount; destRank++) {
			lowerProcRank = destRank - 1;
			MPI_Send((void*)&lowerProcRank, 1, MPI_INT, destRank, LOWER_PROC_TAG, MPI_COMM_WORLD);

			upperProcRank = destRank == procCount - 1 ? -1 : destRank + 1;
			MPI_Send((void*)&upperProcRank, 1, MPI_INT, destRank, UPPER_PROC_TAG, MPI_COMM_WORLD);

			startIndex += batchSize;

			batchSize = integerPart + ((destRank < remainder) ? 1 : 0);

			MPI_Send((void*)&startIndex, 1, MPI_INT, destRank, X_LOCAL_START_TAG, MPI_COMM_WORLD);

			endIndex = startIndex + batchSize - 1;
			MPI_Send((void*)&endIndex, 1, MPI_INT, destRank, X_LOCAL_END_TAG, MPI_COMM_WORLD);
		}
	}
	else {
		// Остальные процессы получают от главного необходимую информацию
		MPI_Recv((void*)&myLowerProcRank, 1, MPI_INT, ROOT_PROC, LOWER_PROC_TAG, MPI_COMM_WORLD, &status);
		MPI_Recv((void*)&myUpperProcRank, 1, MPI_INT, ROOT_PROC, UPPER_PROC_TAG, MPI_COMM_WORLD, &status);

		MPI_Recv((void*)&MyStart, 1, MPI_INT, ROOT_PROC, X_LOCAL_START_TAG, MPI_COMM_WORLD, &status);
		MPI_Recv((void*)&MyEnd, 1, MPI_INT, ROOT_PROC, X_LOCAL_END_TAG, MPI_COMM_WORLD, &status);
	}

    MPI_Barrier(MPI_COMM_WORLD);

	if (myRank == ROOT_PROC) {
		std::cout << "Data initiated" << std::endl;
	}

	// Длина области текущего процесса
	int MySize = MyEnd - MyStart + 1;
	// Добавляем два слоя, которые вычисляют соседние процессы, чтобы производить сво вычисления на крайних слоях
	// (либо это константные значения внешних слоёв, если таковых процессов нет)
	MySize += 2;

	// Инициализируем решётку
	double*** myGrid;
	Inic(myGrid, MySize, Node(x_start, hx, MyStart - 1));

	// Если процесс содержит первый внешний слой, где x - const (x = x_start)
	if (myRank == ROOT_PROC) {
		// Записываем краевые значения
		// При i = x_start
		for (int j = 1; j < Ny - 1; j++) {
			for (int k = 1; k < Nz - 1; k++) {
				myGrid[0][j][k] = phi(x_start, Node(y_start, hy, j), Node(z_start, hz, k));
			}
		}
	}

	// Если процесс содержит второй внешний слой, где x - const (x = x_end)
	if (myRank == procCount - 1) {
		// Записываем краевые значения
		// При i = x_end 
		for (int j = 1; j < Ny - 1; j++) {
			for (int k = 1; k < Nz - 1; k++) {
				myGrid[MySize - 1][j][k] = phi(x_end, Node(y_start, hy, j), Node(z_start, hz, k));
			}
		}
	}

    MPI_Barrier(MPI_COMM_WORLD);
	

	if (myRank == ROOT_PROC) {
		std::cout << "Calculation..." << std::endl;
	}

	double startTime = MPI_Wtime();

	// Производим вычисления по методу Якоби
	Jacobi(myGrid, MySize, MyStart, myLowerProcRank, myUpperProcRank);

	double endTime = MPI_Wtime();

	double elapsedTime = endTime - startTime;

	// Считаем общую точность
	double error = getError(myGrid, MySize, Node(x_start, hx, MyStart - 1), ROOT_PROC);
	if (myRank == ROOT_PROC) {
		std::cout << "Error': " << error << std::endl;
		std::cout << "Time: " << elapsedTime << " s." << std::endl;
	}

	MPI_Finalize();

	deleteGrid(myGrid, MySize);
}
