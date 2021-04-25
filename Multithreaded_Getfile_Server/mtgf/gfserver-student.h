/*
 *  This file is for use by students to define anything they wish.  It is used by the gf server implementation
 */
#ifndef __GF_SERVER_STUDENT_H__
#define __GF_SERVER_STUDENT_H__

#include "gf-student.h"
#include "gfserver.h"
#include "content.h"
#include "steque.h"
#include <pthread.h>

#define BUFSIZE 630
#define QUEUESIZE 5

void* thread_handler(void* arg);
int buffer_write(gfcontext_t* ctx, void* buffstepper, int bytes_read);
// int buffer_write(gfcontext_t* ctx, void* buffstepper, int bytes_read);

#endif // __GF_SERVER_STUDENT_H__
