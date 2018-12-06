# RocketMQ-Client-CPP #

RocketMQ-Client-CPP is the C/C++ client of Apache RocketMQ which is a distributed messaging and streaming platform with low latency, high performance and reliability, trillion-level capacity and flexible scalability.

## Features ##

- produce message, include sync and async produce message, timed and delay message. 

- consume message, include concurrency and orderly consume message, broadcast and cluster consume model.

- rebalance, include both produce message and consume message with rebalance.

- C and C++ API, include both C style and C++ style apis.

- across platform, all features are supported on Windows, Linux and Mac OS.

- reliability, based on nameServer snapshot and network disaster recovery strategy, no real-time impact on publish and subscribe when anyone of broker or nameSrv was broken.

## Dependency ##

- [libevent 2.0.22](https://github.com/libevent/libevent/archive/release-2.0.22-stable.zip "libevent 2.0.22")

- [jsoncpp 0.10.6](https://github.com/open-source-parsers/jsoncpp/archive/0.10.6.zip  "jsoncpp 0.10.6")

- [boost 1.58.0](http://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz "boost 1.58.0")

## Documentation ##
doc/rocketmq-cpp_manaual_zh.docx

## Build and Install ##

### Linux and Mac OS ###

**note**: *make sure the following compile tools or libraries with them minimum version number have been installed before run the build script build.sh*

- compile tools:
	- gcc-c++ 4.8.2: c++ compiler while need support C++11
	- cmake 2.8.0: build jsoncpp require it
	- automake 1.11.1: build libevent require it
	- autoconf 2.65: build libevent require it
	- libtool 2.2.6: build libevent require it

- libraries:   
	- bzip2-devel 1.0.6: boost depend it

build.sh script will automatic download and build the dependency libraries include libevent json and boost, the libraries will be saved under rocketmq-client-cpp folder, and then it will build both rocketmq-client static and shared libraries.

if you can't get to internet to download three library source files by build.sh script, you can copy three library source files (release-2.0.22-stable.zip  0.10.6.zip and boost_1_58_0.tar.gz) to rocketmq-client-cpp root dir, then build.sh will auto use these three library source files to build rocketmq-client-cpp.

    sh build.sh

Finally, both librocketmq.a and librocketmq.so are saved in rocketmq-client-cpp/bin. when use them to build application or library, besides rocketmq you also should link with following libraries -lpthread -lz -ldl -lrt.

    g++ -o consumer_example consumer_example.cpp -lrocketmq -lpthread -lz -ldl -lrt

### Windows ###
**note**: *make sure the following compile tools or libraries with them minimum version number have been installed before run the build script win32_build.bat*
- compile tools:
	- vs2015: libevent,jsoncpp,zlib,boost rocket-client require it
	- git: download source code 
	
one key build script will automatic download dependency libraries include libevent json and boost,  execute follow command,then it will build shared library.

    win32_build.bat

	
if can't get internet to download four library source files by build script, you can copy four library source files 

[zlib-1.2.3-src](https://codeload.github.com/jsj020122/zlib-1.2.3-src/zip/master "zlib-1.2.3-src") Extract to $(rocketmq-client root dir)/thirdparty/zlib-1.2.3-src 

[libevent-release-2.0.22](https://codeload.github.com/jsj020122/libevent-release-2.0.22/zip/master "libevent-release-2.0.22") Extract to $(rocketmq-client root dir)/thirdparty/libevent-release-2.0.22

[boost_1_58_0](https://codeload.github.com/jsj020122/boost_1_58_0/zip/master "boost_1_58_0") Extract to  $(rocketmq-client root dir)/thirdparty/boost_1_58_0

[jsoncpp-0.10.6](https://codeload.github.com/jsj020122/jsoncpp-0.10.6/zip/master "jsoncpp-0.10.6") Extract to  $(rocketmq-client root dir)/thirdparty/jsoncpp-0.10.6 

then execute follow command will auto use these library files to build rocketmq-client.

    win32_build.bat build


## Quick Start ##
### tools and commands ###

- sync produce message
```shell
./SyncProducer -g group1 -t topic1 -c test message -n 172.168.1.1:9876
```
- async produce message
```shell
./AsyncProducer -g group1 -t topic -c test message -n 172.168.1.1:9876
```
- send delay message
```shell
./SendDelayMsg -g group1 -t topic -c test message -n 172.168.1.1:9876
```
- sync push consume message
```shell
./PushConsumer -g group1 -t topic -c test message -s sync -n 172.168.1.1:9876 
```
- async push comsume message
```shell
./AsyncPushConsumer -g group1 -t topic -c test message -n 172.168.1.1:9876
```
- orderly sync push consume message
```shell
./OrderlyPushConsumer -g group1 -t topic -c test message -s sync -n 172.168.1.1:9876
```
- orderly async push consume message
```shell
./OrderlyPushConsumer -g group1 -t topic -c test message -n 172.168.1.1:9876
```
- sync pull consume message
```shell
./PullConsumer -g group1 -t topic -c test message -n 172.168.1.1:9876
```
### Parameters for tools ###
```bash
-n	: nameserver addr, format is ip1:port;ip2:port
-i	: nameserver domain name, parameter -n and -i must have one.
-g	: groupName, contains producer groupName and consumer groupName
-t	: message topic
-m	: message count(default value:1)
-c	: message content(default value: only test)
-b	: consume model(default value: CLUSTER)
-a	: set sync push(default value: async)
-r	: setup retry times(default value:5 times)
-u	: select active broker to send message(default value: false)
-d	: use AutoDeleteSendcallback by cpp client(defalut value: false)
-T	: thread count of send msg or consume message(defalut value: system cpu core number)
-v	: print more details information
```
