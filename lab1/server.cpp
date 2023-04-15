#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <cstring>
#include <pthread.h>
#include <stdexcept>


// char response[] = "HTTP/1.1 200 OK\r\n"
// "Content-Type: text/html; charset=UTF-8\r\n\r\n"
// "<!DOCTYPE html><html><head><title>Bye-bye baby bye-bye</title>"
// "<style>body { background-color: #111 }"
// "h1 { font-size:4cm; text-align: center; color: black;"
// " text-shadow: 0 0 2mm red}</style></head>"
// "<body><h1>Goodbye, world!</h1></body></html>\r\n";

const char response1[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"Request number ";
const char response2[] = " has been processed\n";
const int max_response_size = sizeof(response1)+sizeof(response2)+10+1;
//max_int32 is 10 digits number

struct thread_arg
{
    int client_fd;
    int connection_num;
};

void get_php_version(char* buff)
{
    char buffer[128];
    FILE* pipe = popen("php --version", "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL){strcat(buff, buffer);};
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
}

void *thread_job(void *arg)
{
    thread_arg* argument = (thread_arg*) arg;
    char response[max_response_size+200];
    char s_num [11];
    char buff[200];
    get_php_version(buff);
    sprintf(s_num, "%d", argument->connection_num);
    strcat(response, response1);
    strcat(response, s_num);
    strcat(response, response2);
    strcat(response, buff);
    write(argument->client_fd, response, strlen(response) * sizeof(char)); /*-1:'\0'*/
    shutdown(argument->client_fd, SHUT_WR);

    sleep(1);
    close(argument->client_fd);
    
    delete argument;
    pthread_exit(NULL);
    return NULL;
}


int __start_thread(int client_fd, int connection_num)
{
    pthread_t thread;
    thread_arg arg;
    arg.client_fd = client_fd;
    arg.connection_num = connection_num;
    int err = pthread_create(&thread, NULL, thread_job, &arg);
    // Если при создании потока произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(err != 0) {
    printf("Cannot create a thread: %s (%d)\n", strerror(err), err);
    exit(-1);
    }
    return 0;
}

int main()
{
  printf("pid: %d, ppid: %d", getpid(), getppid());
  printf("\n");
  int one = 1, client_fd, connection_num=0, rc, ds;
  struct sockaddr_in svr_addr, cli_addr;
  socklen_t sin_len = sizeof(cli_addr);
  pthread_attr_t attr;

   rc = pthread_attr_init(&attr);                                               
   if (rc == -1) {                                                              
      perror("error in pthread_attr_init");                                     
      exit(1);                                                                  
   }
//    ds = 1; //0 - joinable, 1 - detached                                                                      
//    rc = pthread_attr_setdetachstate(&attr, ds);                                
//    if (rc == -1) {                                                              
//       perror("error in pthread_attr_setdetachstate");                           
//       exit(2);                                                                  
//    }
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    err(1, "can't open socket");

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));


  int port = 8081;
  svr_addr.sin_family = AF_INET;
  svr_addr.sin_addr.s_addr = INADDR_ANY;
  svr_addr.sin_port = htons(port);

  if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
    close(sock);
    err(1, "Can't bind");
  }

  listen(sock, 100);
  while (1) {
    client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
    printf("got connection %d\n", connection_num);
    if (client_fd == -1) {
        perror("Can't accept");
        continue;
    }
    pthread_t thread;
    thread_arg *arg = new thread_arg;
    arg->client_fd = client_fd;
    arg->connection_num = ++connection_num;
    for (int i = 0; i < 100; i++)
    {
        int err = pthread_create(&thread, &attr, thread_job, (void*)arg);
        if(err != 0) {
            printf("Cannot create a thread: %s (%d)\n", strerror(err), err);
            sleep(1);
            //exit(-1);
            if (i == 99) exit(1);
            continue;
        }
        break;
    }
    // if (pthread_detach(thread) != 0) {
    //     perror("pthread_detach() error");
    //     exit(4);
    // }
  }
}