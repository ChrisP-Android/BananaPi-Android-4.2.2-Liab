/**
  src/tsemaphore.c

  Implements a simple inter-thread semaphore so not to have to deal with IPC
  creation and the like.

  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include "DisplaySemaphore.h"
#include <cutils/log.h>

#define LOG_NDEBUG 0

/** Initializes the semaphore at a given value
 *
 * @param tsem the semaphore to initialize
 * @param val the initial value of the semaphore
 *
 */
namespace android
{
    /** Initializes the semaphore at a given value
     *
     * @param tsem the semaphore to initialize
     * @param val the initial value of the semaphore
     *
     */
    DisplaySemaphore::DisplaySemaphore(unsigned int val)
    {
        pthread_cond_init(&condition, NULL);
        pthread_mutex_init(&mutex, NULL);
        semval = val;
    }

    /** Destroy the semaphore
     *
     * @param tsem the semaphore to destroy
     */
    DisplaySemaphore::~DisplaySemaphore()
    {
        pthread_cond_destroy(&condition);
        pthread_mutex_destroy(&mutex);
    }

    /** Decreases the value of the semaphore. Blocks if the semaphore
     * value is zero.
     *
     * @param tsem the semaphore to decrease
     */
    void DisplaySemaphore::down()
    {
        pthread_mutex_lock(&mutex);
        while (semval == 0)
        {
            pthread_cond_wait(&condition, &mutex);
        }
        semval--;
        pthread_mutex_unlock(&mutex);
    }

    /** Increases the value of the semaphore
     *
     * @param tsem the semaphore to increase
     */
    void DisplaySemaphore::up()
    {
        pthread_mutex_lock(&mutex);
        semval++;
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
    }

    /** Reset the value of the semaphore
     *
     * @param tsem the semaphore to reset
     */
    void DisplaySemaphore::reset()
    {
        pthread_mutex_lock(&mutex);
        semval=0;
        pthread_mutex_unlock(&mutex);
    }

    /** Wait on the condition.
     *
     * @param tsem the semaphore to wait
     */
    void DisplaySemaphore::wait()
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condition, &mutex);
        pthread_mutex_unlock(&mutex);
    }

    /** Signal the condition,if waiting
     *
     * @param tsem the semaphore to signal
     */
    void DisplaySemaphore::signal()
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
    }
}
