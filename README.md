# RocketMQ-Client-CPP
[![License](https://img.shields.io/badge/license-Apache%202-4EB1BA.svg)](https://www.apache.org/licenses/LICENSE-2.0.html)
[![Build Status](https://app.travis-ci.com/apache/rocketmq-client-cpp.svg?branch=master)](https://app.travis-ci.com/apache/rocketmq-client-cpp)
[![CodeCov](https://codecov.io/gh/apache/rocketmq-client-cpp/branch/master/graph/badge.svg)](https://codecov.io/gh/apache/rocketmq-client-cpp)
[![GitHub release](https://img.shields.io/badge/release-download-default.svg)](https://github.com/apache/rocketmq-client-cpp/releases)
![Twitter Follow](https://img.shields.io/twitter/follow/ApacheRocketMQ?style=social)

RocketMQ-Client-CPP is the C/C++ client of Apache RocketMQ, a distributed messaging and streaming platform with low latency, high performance and reliability, trillion-level capacity and flexible scalability.

## Features

- produce messages, including normal and delayed messages, synchronously or asynchronously. 
- consume messages, in cluster or broadcast model, concurrently or orderly
- c and c++ style API.
- cross-platform, all features are supported on Windows, Linux and Mac OS.
- automatically rebalanced, both in producing and consuming process.
- reliability, any downtime broker or name server has no impact on the client.

## Build and Install

### Linux and Mac OS

**note**: make sure the following compile tools or libraries have been installed before running the build script **build.sh**.

- compile tools:
	- gcc-c++ 4.8.2: c++ compiler while need support C++11
	- cmake 2.8.0: build jsoncpp require it
	- automake 1.11.1: build libevent require it
	- autoconf 2.65: build libevent require it
	- libtool 2.2.6: build libevent require it

- libraries:   
	- bzip2-devel 1.0.6: boost depend it
	- zlib-devel

The **build.sh** script will automatically download and build the dependency libraries including libevent, json and boost. It will save libraries under rocketmq-client-cpp folder, and then build both static and shared libraries for rocketmq-client. If the dependent libraries are built failed, you could try to build it manually with sources [libevent 2.0.22](https://github.com/libevent/libevent/archive/release-2.0.22-stable.zip "lib event 2.0.22"), [jsoncpp 0.10.7](https://github.com/open-source-parsers/jsoncpp/archive/0.10.7.zip  "jsoncpp 0.10.7"), [boost 1.58.0](http://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz "boost 1.58.0")

If your host is not available to internet to download the three library source files, you can copy the three library source files (release-2.0.22-stable.zip  0.10.7.zip and boost_1_58_0.tar.gz) to rocketmq-client-cpp root dir, then the build.sh will automatically use the three library source files to build rocketmq-client-cpp:

    sh build.sh

Finally, both librocketmq.a and librocketmq.so are saved in rocketmq-client-cpp/bin. when using them to build application or library, besides rocketmq you should also link with following libraries -lpthread -lz -ldl -lrt. Here is an example:

    g++ -o consumer_example consumer_example.cpp -lrocketmq -lpthread -lz -ldl -lrt

### Windows
**note**: make sure the following compile tools or libraries have been installed before running the build script **win32_build.bat**:

- compile tools:
	- vs2015: libevent,jsoncpp,zlib,boost rocket-client require it
	- git: download source code 
	
The build script will automatically download dependent libraries including libevent json and boost to build shared library:

    win32_build.bat

	
If your host is not available to internet to download the four library source files by build script, you can copy the four library source files 

[zlib-1.2.3-src](https://codeload.github.com/jsj020122/zlib-1.2.3-src/zip/master "zlib-1.2.3-src") Extract to $(rocketmq-client-cpp root dir)/thirdparty/zlib-1.2.3-src 

[libevent-release-2.0.22](https://codeload.github.com/jsj020122/libevent-release-2.0.22/zip/master "libevent-release-2.0.22") Extract to $(rocketmq-client-cpp root dir)/thirdparty/libevent-release-2.0.22

[boost_1_58_0](https://codeload.github.com/jsj020122/boost_1_58_0/zip/master "boost_1_58_0") Extract to  $(rocketmq-client-cpp root dir)/thirdparty/boost_1_58_0

[jsoncpp-0.10.6](https://codeload.github.com/jsj020122/jsoncpp-0.10.6/zip/master "jsoncpp-0.10.6") Extract to  $(rocketmq-client-cpp root dir)/thirdparty/jsoncpp-0.10.6 

And then run following command to build x86 rocketmq-client:

    win32_build.bat build

to build x64 rocketmq-client:

    win32_build.bat build64


