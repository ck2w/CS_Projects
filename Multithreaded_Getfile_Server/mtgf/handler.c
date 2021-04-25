#include "gfserver-student.h"
#include "gfserver.h"
#include "content.h"

#include "workload.h"


pthread_mutex_t mutex;
pthread_cond_t c_cons;  // queue not empty
pthread_cond_t c_prod;  // queue not full
steque_t* request_steque;
int steque_len;

struct thread_info{    
    const char *path;
    gfcontext_t *ctx;
	ssize_t file_size;    
};

typedef struct thread_info thread_info;

int init_threads(int nthreads){

    request_steque = malloc(sizeof(steque_t));        
    steque_init(request_steque);

	pthread_cond_init(&c_cons, NULL);
    pthread_cond_init(&c_prod, NULL);
	pthread_mutex_init(&mutex, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);    
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (int i = 0; i < nthreads; ++i) {        
        pthread_t thread;
        pthread_create(&thread, &attr, thread_handler, NULL);            
    }   
    pthread_attr_destroy(&attr);

    return EXIT_SUCCESS;
}


gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void *arg){

    thread_info* t_info;
    t_info = malloc(sizeof(thread_info));

    t_info->path = (char*)path;
    t_info->ctx = *ctx;
    t_info->file_size = 0;
	*ctx = NULL;

    pthread_mutex_lock(&mutex);
    while (steque_len >= BUFSIZE){
        pthread_cond_wait(&c_prod, &mutex);
    }

    steque_enqueue(request_steque, (void *)t_info);
	steque_len++;
	pthread_cond_signal(&c_cons);
    pthread_mutex_unlock(&mutex);

	return gfh_failure;
}


void* thread_handler(void* arg){

    thread_info* t_info;
    ssize_t bytes_read;
    size_t total_bytes_read = 0;
    char buffer[BUFSIZE] = {0};
	int fd;
	struct stat file_stat;
	int file_len;

    for(;;){

        pthread_mutex_lock(&mutex);
        while (steque_len == 0){
            pthread_cond_wait(&c_cons, &mutex);
        }

		t_info = (thread_info*)steque_pop(request_steque);
		steque_len--;
		pthread_cond_signal(&c_prod);
        pthread_mutex_unlock(&mutex);

		fd = content_get(t_info->path);

		fstat(fd, &file_stat);
		file_len = file_stat.st_size;
		gfs_sendheader(&(t_info->ctx), GF_OK, (size_t) file_len);

		for (;;){
			bytes_read = pread(fd, buffer, BUFSIZE, total_bytes_read);
			if (bytes_read<=0) break;

			buffer_write(t_info->ctx, buffer, bytes_read);
			total_bytes_read += bytes_read;

			buffer[0] = '\0';
		}
        free(t_info);
        buffer[0] = '\0';
        total_bytes_read = 0;
    }

    return NULL;
}

int buffer_write(gfcontext_t* ctx, void* buffstepper, int bytes_read){
	ssize_t bytes_complete = 0;

    for (;;){
        if (bytes_read == 0) 
			break;
        buffstepper += bytes_complete;
        bytes_complete = gfs_send(&ctx, buffstepper, bytes_read);
        bytes_read -= bytes_complete;
    }
	return EXIT_SUCCESS;
}
