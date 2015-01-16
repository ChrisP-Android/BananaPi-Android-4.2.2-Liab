/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

#ifndef __MESSAGE_H_8X3E__
#define __MESSAGE_H_8X3E__

#include <pthread.h>

//cts test may send 1000 messages.
//we must make sure message queue longer than 10000.

#define MAX_MESSAGE_ELEMENTS 20000

typedef struct message_t message_t;
struct message_t
{
	int 				id;
	int 				command;
//	int                 priority;
	int 				para0;
	void* 				data;
	struct message_t*	next;
};

typedef struct message_queue_t
{
	message_t* 		head;
	message_t* 		tail;
	int 			message_count;
	pthread_mutex_t mutex;
}message_queue_t;

int  message_create(message_queue_t* message);
void message_destroy(message_queue_t* msg_queue);
void flush_message(message_queue_t* msg_queue);
int  put_message(message_queue_t* msg_queue, message_t *msg_in);
int  get_message(message_queue_t* msg_queue, message_t *msg_out);
int  get_message_count(message_queue_t* message);

#endif
