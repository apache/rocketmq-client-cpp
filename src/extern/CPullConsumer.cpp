// /*
//  * Licensed to the Apache Software Foundation (ASF) under one or more
//  * contributor license agreements.  See the NOTICE file distributed with
//  * this work for additional information regarding copyright ownership.
//  * The ASF licenses this file to You under the Apache License, Version 2.0
//  * (the "License"); you may not use this file except in compliance with
//  * the License.  You may obtain a copy of the License at
//  *
//  *     http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
//  */
// #include "c/CPullConsumer.h"

// #include <cstring>

// #include "CErrorContainer.h"
// #include "ClientRPCHook.h"
// #include "DefaultMQPullConsumer.h"
// #include "Logging.h"

// using namespace rocketmq;

// CPullConsumer* CreatePullConsumer(const char* groupId) {
//   if (groupId == NULL) {
//     return NULL;
//   }
//   auto* defaultMQPullConsumer = new DefaultMQPullConsumer(groupId);
//   return reinterpret_cast<CPullConsumer*>(defaultMQPullConsumer);
// }

// int DestroyPullConsumer(CPullConsumer* consumer) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   delete reinterpret_cast<DefaultMQPullConsumer*>(consumer);
//   return OK;
// }

// int StartPullConsumer(CPullConsumer* consumer) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   try {
//     reinterpret_cast<DefaultMQPullConsumer*>(consumer)->start();
//   } catch (std::exception& e) {
//     CErrorContainer::setErrorMessage(e.what());
//     return PULLCONSUMER_START_FAILED;
//   }
//   return OK;
// }

// int ShutdownPullConsumer(CPullConsumer* consumer) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   reinterpret_cast<DefaultMQPullConsumer*>(consumer)->shutdown();
//   return OK;
// }

// int SetPullConsumerGroupID(CPullConsumer* consumer, const char* groupId) {
//   if (consumer == NULL || groupId == NULL) {
//     return NULL_POINTER;
//   }
//   reinterpret_cast<DefaultMQPullConsumer*>(consumer)->setGroupName(groupId);
//   return OK;
// }

// const char* GetPullConsumerGroupID(CPullConsumer* consumer) {
//   if (consumer == NULL) {
//     return NULL;
//   }
//   return reinterpret_cast<DefaultMQPullConsumer*>(consumer)->getGroupName().c_str();
// }

// int SetPullConsumerNameServerAddress(CPullConsumer* consumer, const char* namesrv) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   reinterpret_cast<DefaultMQPullConsumer*>(consumer)->setNamesrvAddr(namesrv);
//   return OK;
// }

// // Deprecated
// int SetPullConsumerNameServerDomain(CPullConsumer* consumer, const char* domain) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   return NOT_SUPPORT_NOW;
// }

// int SetPullConsumerSessionCredentials(CPullConsumer* consumer,
//                                       const char* accessKey,
//                                       const char* secretKey,
//                                       const char* channel) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   auto rpcHook = std::make_shared<ClientRPCHook>(SessionCredentials(accessKey, secretKey, channel));
//   reinterpret_cast<DefaultMQPullConsumer*>(consumer)->setRPCHook(rpcHook);
//   return OK;
// }

// int SetPullConsumerLogPath(CPullConsumer* consumer, const char* logPath) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   // Todo, This api should be implemented by core api.
//   //((DefaultMQPullConsumer *) consumer)->setInstanceName(instanceName);
//   return OK;
// }

// int SetPullConsumerLogFileNumAndSize(CPullConsumer* consumer, int fileNum, long fileSize) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   DEFAULT_LOG_ADAPTER->setLogFileNumAndSize(fileNum, fileSize);
//   return OK;
// }

// int SetPullConsumerLogLevel(CPullConsumer* consumer, CLogLevel level) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   DEFAULT_LOG_ADAPTER->set_log_level((LogLevel)level);
//   return OK;
// }

// int FetchSubscriptionMessageQueues(CPullConsumer* consumer, const char* topic, CMessageQueue** mqs, int* size) {
//   if (consumer == NULL) {
//     return NULL_POINTER;
//   }
//   unsigned int index = 0;
//   CMessageQueue* temMQ = NULL;
//   std::vector<MQMessageQueue> fullMQ;
//   try {
//     reinterpret_cast<DefaultMQPullConsumer*>(consumer)->fetchSubscribeMessageQueues(topic, fullMQ);
//     *size = fullMQ.size();
//     // Alloc memory to save the pointer to CPP MessageQueue, and the MessageQueues may be changed.
//     // Thus, this memory should be released by users using @ReleaseSubscribeMessageQueue every time.
//     temMQ = (CMessageQueue*)malloc(*size * sizeof(CMessageQueue));
//     if (temMQ == NULL) {
//       *size = 0;
//       *mqs = NULL;
//       return MALLOC_FAILED;
//     }
//     auto iter = fullMQ.begin();
//     for (index = 0; iter != fullMQ.end() && index <= fullMQ.size(); ++iter, index++) {
//       strncpy(temMQ[index].topic, iter->topic().c_str(), MAX_TOPIC_LENGTH - 1);
//       strncpy(temMQ[index].brokerName, iter->broker_name().c_str(), MAX_BROKER_NAME_ID_LENGTH - 1);
//       temMQ[index].queueId = iter->queue_id();
//     }
//     *mqs = temMQ;
//   } catch (MQException& e) {
//     *size = 0;
//     *mqs = NULL;
//     CErrorContainer::setErrorMessage(e.what());
//     return PULLCONSUMER_FETCH_MQ_FAILED;
//   }
//   return OK;
// }

// int ReleaseSubscriptionMessageQueue(CMessageQueue* mqs) {
//   if (mqs == NULL) {
//     return NULL_POINTER;
//   }
//   free((void*)mqs);
//   mqs = NULL;
//   return OK;
// }

// CPullResult Pull(CPullConsumer* consumer,
//                  const CMessageQueue* mq,
//                  const char* subExpression,
//                  long long offset,
//                  int maxNums) {
//   CPullResult pullResult;
//   memset(&pullResult, 0, sizeof(CPullResult));
//   MQMessageQueue messageQueue(mq->topic, mq->brokerName, mq->queueId);
//   PullResult cppPullResult;
//   try {
//     cppPullResult =
//         reinterpret_cast<DefaultMQPullConsumer*>(consumer)->pull(messageQueue, subExpression, offset, maxNums);
//   } catch (std::exception& e) {
//     CErrorContainer::setErrorMessage(e.what());
//     cppPullResult.set_pull_status(BROKER_TIMEOUT);
//   }

//   if (cppPullResult.pull_status() != BROKER_TIMEOUT) {
//     pullResult.maxOffset = cppPullResult.max_offset();
//     pullResult.minOffset = cppPullResult.min_offset();
//     pullResult.nextBeginOffset = cppPullResult.next_begin_offset();
//   }

//   switch (cppPullResult.pull_status()) {
//     case FOUND: {
//       pullResult.pullStatus = E_FOUND;
//       pullResult.size = cppPullResult.msg_found_list().size();
//       PullResult* tmpPullResult = new PullResult(cppPullResult);
//       pullResult.pData = tmpPullResult;
//       // Alloc memory to save the pointer to CPP MQMessageExt, which will be release by the CPP SDK core.
//       // Thus, this memory should be released by users using @ReleasePullResult
//       pullResult.msgFoundList = (CMessageExt**)malloc(pullResult.size * sizeof(CMessageExt*));
//       for (size_t i = 0; i < cppPullResult.msg_found_list().size(); i++) {
//         auto msg = tmpPullResult->msg_found_list()[i];
//         pullResult.msgFoundList[i] = reinterpret_cast<CMessageExt*>(&msg);
//       }
//       break;
//     }
//     case NO_NEW_MSG: {
//       pullResult.pullStatus = E_NO_NEW_MSG;
//       break;
//     }
//     case NO_MATCHED_MSG: {
//       pullResult.pullStatus = E_NO_MATCHED_MSG;
//       break;
//     }
//     case OFFSET_ILLEGAL: {
//       pullResult.pullStatus = E_OFFSET_ILLEGAL;
//       break;
//     }
//     case BROKER_TIMEOUT: {
//       pullResult.pullStatus = E_BROKER_TIMEOUT;
//       break;
//     }
//     default:
//       pullResult.pullStatus = E_NO_NEW_MSG;
//       break;
//   }
//   return pullResult;
// }

// int ReleasePullResult(CPullResult pullResult) {
//   if (pullResult.size == 0 || pullResult.msgFoundList == NULL || pullResult.pData == NULL) {
//     return NULL_POINTER;
//   }
//   if (pullResult.pData != NULL) {
//     try {
//       delete ((PullResult*)pullResult.pData);
//     } catch (std::exception& e) {
//       CErrorContainer::setErrorMessage(e.what());
//       return NULL_POINTER;
//     }
//   }
//   free((void*)pullResult.msgFoundList);
//   pullResult.msgFoundList = NULL;
//   return OK;
// }
