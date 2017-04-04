/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_private.h"
#include "apr_arch_thread_mutex.h"
#define APR_WANT_MEMFUNC
#include "apr_want.h"
#include <stdio.h>
#if APR_HAS_THREADS

#ifndef HAVE_PTHREAD_MUTEX_TIMEDLOCK
extern int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abs_timeout);
#define SLEEP_TIME_NS      10000000
#define NANOSECS_PER_SEC 1000000000
/*
* A pthread_mutex_timedlock() impl for OSX/macOS, which lacks the
* real thing.
* NOTE: Unlike the real McCoy, won't return EOWNERDEAD, EDEADLK
*       or EOWNERDEAD
*/
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abs_timeout)
{
   int rv;
   struct timespec remaining, ts, tod;
   apr_time_t now;

   remaining = *abs_timeout;
   now = apr_time_now();
   tod.tv_sec = apr_time_sec(now);
   tod.tv_nsec = apr_time_usec(now) * 1000; /* nanoseconds */

   remaining.tv_sec -= tod.tv_sec;
   if (tod.tv_nsec <= remaining.tv_nsec) {
       remaining.tv_nsec -= tod.tv_nsec;
   }
   else {
       remaining.tv_sec--;
       remaining.tv_nsec = (NANOSECS_PER_SEC - (tod.tv_nsec - remaining.tv_nsec));
   }
   while ((rv = pthread_mutex_trylock(mutex)) == EBUSY) {
       ts.tv_sec = 0;
       ts.tv_nsec = (remaining.tv_sec > 0 ? SLEEP_TIME_NS : 
                    (remaining.tv_nsec < SLEEP_TIME_NS ? remaining.tv_nsec : SLEEP_TIME_NS));
       nanosleep(&ts, &ts);
       if (ts.tv_nsec <= remaining.tv_nsec) {
           remaining.tv_nsec -= ts.tv_nsec;
       }
       else {
           remaining.tv_sec--;
           remaining.tv_nsec = (NANOSECS_PER_SEC - (ts.tv_nsec - remaining.tv_nsec));
       }
       if (remaining.tv_sec < 0 || (!remaining.tv_sec && remaining.tv_nsec <= SLEEP_TIME_NS)) {
           return ETIMEDOUT;
       }
   }

   return rv;
}

#endif /* HAVE_PTHREAD_MUTEX_TIMEDLOCK */
#endif /* APR_HAS_THREADS */
