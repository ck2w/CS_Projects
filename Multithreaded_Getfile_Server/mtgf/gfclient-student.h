/*
 *  This file is for use by students to define anything they wish.  It is used by the gf client implementation
 */
#ifndef __GF_CLIENT_STUDENT_H__
#define __GF_CLIENT_STUDENT_H__

#include "workload.h"
#include "gfclient.h"
#include "gf-student.h"
#include "steque.h"
 #include <pthread.h>
 
#define BUFSIZE 630
#define QUEUESIZE 5


pthread_t *threadspool = NULL;
pthread_mutex_t mutex;
pthread_cond_t c_cons;  // queue not empty
pthread_cond_t c_prod;  // queue not full


void* handle_request_file(void* arg); // create threadspool, other init
pthread_t* threadspool_create(int nthreads); // task handler

 #endif // __GF_CLIENT_STUDENT_H__