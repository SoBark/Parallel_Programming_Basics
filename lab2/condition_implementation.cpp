#include <pthread.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

#define err_exit(code, str) { std::cerr << str << ": " << strerror(code) \
<< std::endl; exit(EXIT_FAILURE); }
#define STORAGE_MIN 10
#define STORAGE_MAX 20

/* doubly linked list implementation of */
/* queue for blocked threads */ 
struct proc{
    proc* front = NULL; 
    proc* back = NULL; 
    pthread_t thread;
    pthread_mutex_t mutex;
};

class condition_c{
    public:
    void init()
    {
        int err;
        err = pthread_mutex_init(&mutex, NULL);
        if(err != 0)
            err_exit(err, "Cannot initialize condition variable");
    }

     void wait(pthread_mutex_t* mx)
     {
        int err;
        proc *this_thread;
        err = pthread_mutex_lock(&mutex); /* protect the queue */
        if(err != 0){
            err_exit(err, "Cannot lock mutex");}
        this_thread = enqueue();
        err = pthread_mutex_unlock(&mutex); /* protect the queue */
        if(err != 0){
            err_exit(err, "Cannot unlock mutex");}

        //waiting thread clean for himself
        err = pthread_mutex_unlock(mx);
        if(err != 0){
            err_exit(err, "Cannot unlock mutex");}
        err = pthread_mutex_lock(&(this_thread->mutex));
        if(err != 0){
            err_exit(err, "Cannot lock mutex");}
        err = pthread_mutex_unlock(&(this_thread->mutex));
        if(err != 0){
            err_exit(err, "Cannot unlock mutex");}
        pthread_mutex_destroy(&(this_thread->mutex));
        pthread_mutex_destroy(&(this_thread->mutex));
        delete(this_thread);
        return;
     }

     void signal()
     {
        int err;
        proc *some_thread;
        err = pthread_mutex_lock(&mutex); /* protect the queue */
        if(err != 0){
            err_exit(err, "Cannot lock mutex");}
        some_thread = dequeue();
        err = pthread_mutex_unlock(&mutex); /* protect the queue */
        if(err != 0){
            err_exit(err, "Cannot unlock mutex");}
        if (some_thread == NULL) return;
        err = pthread_mutex_unlock(&(some_thread->mutex));
        if(err != 0){
            err_exit(err, "Cannot unlock mutex");}
        return;
     }

     void broadcast()
     {
        int err;
        proc* p_iterator;
        err = pthread_mutex_lock(&mutex); /* protect the queue */
        if(err != 0){
            err_exit(err, "Cannot lock mutex");}
        if (current == NULL)
        {
            err = pthread_mutex_unlock(&mutex);
            if(err != 0){
                err_exit(err, "Cannot unlock mutex");}
            return;
        }
        while(current != NULL)
        {
            p_iterator = dequeue();
            if (p_iterator == NULL) break;
            err = pthread_mutex_unlock(&(p_iterator->mutex));
            if(err != 0){
                err_exit(err, "Cannot unlock mutex");}
        }
        err = pthread_mutex_unlock(&mutex);
        if(err != 0){
            err_exit(err, "Cannot unlock mutex");}
        return;
     }
     
    private:
    proc* current = NULL;
    pthread_mutex_t mutex; /*protects queue */
    proc * enqueue(){
        proc* p_iterator;
        int err;
        //proc* p_front = NULL;
        if(current == NULL) //first thread in queue
        {
            current = new proc;
            p_iterator = current;
        }
        else //several threads in queue
        {
            //find last added thread to queue
            while (p_iterator->back != NULL)
                p_iterator = p_iterator->back;
            p_iterator->back = new proc;
            p_iterator->back->front = p_iterator;
            p_iterator = p_iterator->back;

        }
        p_iterator->thread = pthread_self();
        err = pthread_mutex_init(&(p_iterator->mutex), NULL);
        if(err != 0)
            err_exit(err, "Cannot initialize mutex");
        err = pthread_mutex_lock(&(p_iterator->mutex));
        if(err != 0)
            err_exit(err, "Cannot lock mutex");
        return p_iterator;
    };

    proc* dequeue(){
        if (current == NULL)
            return NULL;
        proc* p_iterator;
        p_iterator = current;
        current = current->back;
        if (current != NULL)
        {
            current->front = NULL;
        }
        return p_iterator;
    }
};


/* Разделяемый ресурс */
int storage = STORAGE_MIN;

pthread_mutex_t mutex;
condition_c condition;


/* Функция потока потребителя */
void *consumer(void *args)
{
	puts("[CONSUMER] thread started");
	int toConsume = 0;
	
	while(1)
	{
		pthread_mutex_lock(&mutex);
		/* Если значение общей переменной меньше максимального, 
		 * то поток входит в состояние ожидания сигнала о достижении
		 * максимума */
		while (storage < STORAGE_MAX)
		{
			condition.wait(&mutex);
		}
		toConsume = storage-STORAGE_MIN;
		printf("[CONSUMER] storage is maximum, consuming %d\n", \
			toConsume);
		/* "Потребление" допустимого объема из значения общей 
		 * переменной */
		storage -= toConsume;
		printf("[CONSUMER] storage = %d\n", storage);
		pthread_mutex_unlock(&mutex);
	}
	
	return NULL;
}

/* Функция потока производителя */
void *producer(void *args)
{
	puts("[PRODUCER] thread started");
	
	while (1)
	{
		usleep(200000);
		pthread_mutex_lock(&mutex);
		/* Производитель постоянно увеличивает значение общей переменной */
		++storage;
		printf("[PRODUCER] storage = %d\n", storage);
		/* Если значение общей переменной достигло или превысило
		 * максимум, поток потребитель уведомляется об этом */
		if (storage >= STORAGE_MAX)
		{
			puts("[PRODUCER] storage maximum");
			condition.signal();
		}
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int res = 0;
	pthread_t thProducer, thConsumer;
	
	pthread_mutex_init(&mutex, NULL);
	condition.init();
	
	res = pthread_create(&thProducer, NULL, producer, NULL);
	if (res != 0)
	{
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}
	
	res = pthread_create(&thConsumer, NULL, consumer, NULL);
	if (res != 0)
	{
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}
	
	pthread_join(thProducer, NULL);
	pthread_join(thConsumer, NULL);
	
	return EXIT_SUCCESS;
}