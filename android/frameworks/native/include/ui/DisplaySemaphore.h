/**
  src/tsemaphore.h

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

#ifndef __UI_DISPLAYSEMAPHORE_H_
#define __UI_DISPLAYSEMAPHORE_H_

#include <pthread.h>
#include <utils/RefBase.h>

/** The structure contains the semaphore value, mutex and green light flag
 */

namespace android 
{
    class DisplaySemaphore:public virtual RefBase
    {
        public:
            DisplaySemaphore(unsigned int val);
            ~DisplaySemaphore();

            void down();
            void up();
            void reset();
            void wait();

            /** Signal the condition,if waiting
             *
             * @param tsem the semaphore to signal
             */
            void signal();
        private:
            pthread_cond_t condition;
            pthread_mutex_t mutex;
            unsigned int semval;
    };
}


#endif

