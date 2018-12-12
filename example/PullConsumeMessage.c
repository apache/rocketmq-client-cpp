/*
* Licensed to the Apache Software Foundation (ASF) under one or more
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

#include <stdio.h>

#include "CPullConsumer.h"
#include "CCommon.h"
#include "CMessageExt.h"
#include "CPullResult.h"
#include "CMessageQueue.h"

#ifdef _WIN32
#include <windows.h>
#else

#include <unistd.h>
#include <memory.h>

#endif

void thread_sleep(unsigned milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);  // takes microseconds
#endif
}

int main(int argc, char *argv[]) {
    int i = 0, j = 0;
    int ret = 0;
    int size = 0;
    CMessageQueue *mqs = NULL;
    printf("PullConsumer Initializing....\n");
    CPullConsumer *consumer = CreatePullConsumer("Group_Consumer_Test");
    SetPullConsumerNameServerAddress(consumer, "172.17.0.2:9876");
    StartPullConsumer(consumer);
    printf("Pull Consumer Start...\n");
    for (i = 1; i <= 5; i++) {
        printf("FetchSubscriptionMessageQueues : %d times\n", i);
        ret = FetchSubscriptionMessageQueues(consumer, "T_TestTopic", &mqs, &size);
        if(ret != OK) {
            printf("Get MQ Queue Failed,ErrorCode:%d\n", ret);
        }
        printf("Get MQ Size:%d\n", size);
        for (j = 0; j < size; j++) {
            int noNewMsg = 0;
            long long tmpoffset = 0;
            printf("Pull Message For Topic:%s,Queue:%s,QueueId:%d\n", mqs[j].topic, mqs[j].brokerName, mqs[j].queueId);
            do {
                int k = 0;
                CPullResult pullResult = Pull(consumer, &mqs[j], "*", tmpoffset, 32);
                if (pullResult.pullStatus != E_BROKER_TIMEOUT) {
                    tmpoffset = pullResult.nextBeginOffset;
                }
                printf("PullStatus:%d,MaxOffset:%lld,MinOffset:%lld,NextBegainOffset:%lld", pullResult.pullStatus,
                       pullResult.maxOffset, pullResult.minOffset, pullResult.nextBeginOffset);
                switch (pullResult.pullStatus) {
                    case E_FOUND:
                        printf("Get Message Size:%d\n", pullResult.size);
                        for (k = 0; k < pullResult.size; ++k) {
                            printf("Got Message ID:%s,Body:%s\n", GetMessageId(pullResult.msgFoundList[k]),GetMessageBody(pullResult.msgFoundList[k]));
                        }
                        break;
                    case E_NO_MATCHED_MSG:
                        noNewMsg = 1;
                        break;
                    default:
                        noNewMsg = 0;
                }
                ReleasePullResult(pullResult);
                thread_sleep(100);
            } while (noNewMsg == 0);
            thread_sleep(1000);
        }
        thread_sleep(2000);
        ReleaseSubscriptionMessageQueue(mqs);
    }
    thread_sleep(5000);
    ShutdownPullConsumer(consumer);
    DestroyPullConsumer(consumer);
    printf("PullConsumer Shutdown!\n");
    return 0;
}
