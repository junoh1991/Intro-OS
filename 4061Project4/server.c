/* csci4061 S2016 Assignment 4 
* section: one_digit_number 
* date: mm/dd/yy 
* names: Name of each member of the team (for partners)
* UMN Internet ID, Student ID (xxxxxxxx, 4444444), (for partners)
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "util.h"
#include "unistd.h"

#define MAX_THREADS 100
#define MAX_QUEUE_SIZE 100
#define MAX_REQUEST_LENGTH 1024
//Structure for a single request.
typedef struct request
{
    int     m_socket;
    char    m_szRequest[MAX_REQUEST_LENGTH];
} request_t;

// Mutex and ring buffer declaration
pthread_mutex_t request_access = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t slot_availble = PTHREAD_COND_INITIALIZER;
pthread_cond_t request_availble = PTHREAD_COND_INITIALIZER;
int dispatcher_count;
int worker_count;
int request_count;
int numb_dispatcher, numb_worker;
int ringbuffer;
char* root_dir;
request_t request_array[MAX_QUEUE_SIZE];

void * dispatch(void * arg)
{
    int fd;
    while(1)
    {
        pthread_mutex_lock(&request_access);
        while (request_count== ringbuffer)
            pthread_cond_wait(&slot_availble, &request_access);
        fd = accept_connection();
        if (fd < 0)
            continue;
        request_array[dispatcher_count].m_socket = fd;
        if (get_request(fd, request_array[dispatcher_count].m_szRequest) < 0)
            continue;
        dispatcher_count = (dispatcher_count + 1) % ringbuffer;
        request_count++;
        pthread_cond_signal(&request_availble);
        pthread_mutex_unlock(&request_access);
    }
    return NULL;
}

void * worker(void * arg)
{
    int fd;
    char *request;
    char *content_type;
    int read_count;
    FILE *fp;
    int file_size;
    char error_message[100];
    char *dot;
    int index;
    while(1)
    {
        pthread_mutex_lock(&request_access);
        while (request_count == 0)
            pthread_cond_wait(&request_availble, &request_access);
        request = request_array[request_count].m_szRequest;
        fd = request_array[request_count].m_socket;
        request_count--;
        worker_count = (worker_count + 1) % ringbuffer;
        pthread_cond_signal(&slot_availble);
        pthread_mutex_unlock(&request_access);

        fp = fopen(request, "r");
        if (fp == NULL)
            return_error(fd, error_message);
        dot = strchr(request, '.');
        if (strcmp(dot, ".html") == 0)
            content_type = "text/html";
        else if (strcmp(dot, ".gif") == 0)
            content_type = "image/gif";
        else if (strcmp(dot, ".jpg") == 0)
            content_type = "image/jpeg";
        else
            content_type = "text/plain";        
        
        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        rewind(fp);
        char transmit_buffer[file_size];
        fread(transmit_buffer, file_size, 1, fp);  
        return_result(fd, content_type, transmit_buffer, file_size);
    }    

    return NULL;
}

int main(int argc, char **argv)
{
    int port;
    int i;
    pthread_t dispatcher_threads[numb_dispatcher];
    pthread_t worker_threads[numb_worker];

    //Error check first.
    if(argc != 6 && argc != 7)
    {
        printf("usage: %s port path num_dispatcher num_workers queue_length [cache_size]\n", argv[0]);
        return -1;
    }
    // Parse the arguments when there is no error to the arguments.
    port = atoi(argv[1]);
    numb_dispatcher = atoi(argv[3]);
    numb_worker = atoi(argv[4]);
    ringbuffer = atoi(argv[5]);
    request_count = 0;
    dispatcher_count = 0;
    worker_count = 0;
    root_dir = argv[2];

    init(port);
    for(i = 0; i < numb_dispatcher; i ++) // create dispather threads;
        pthread_create(&dispatcher_threads[i], NULL, dispatch, NULL);
    for(i = 0; i < numb_worker; i ++)
        pthread_create(&worker_threads[i], NULL, worker, NULL);


    pause();
    printf("Call init() first and make a dispather and worker threads\n");
    return 0;
}
