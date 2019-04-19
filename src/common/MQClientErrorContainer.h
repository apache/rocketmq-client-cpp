#pragma once
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

#ifndef MQCLIENTERRORCONTAINER_H_INCLUDE
#define MQCLIENTERRORCONTAINER_H_INCLUDE
#include <string>
#include <exception>

namespace rocketmq {
	class  MQClientErrorContainer {

	public:
		static MQClientErrorContainer * instance();

		void setErr(std::string & str);

		std::string getErr();

	private:
		std::string m_err;
		static MQClientErrorContainer * s_instance;
	};
}

#endif
