# RocketMQ-Client-CPP #

RocketMQ-Client-CPP is the C/C++ client of Apache RocketMQ which is a distributed messaging and streaming platform with low latency, high performance and reliability, trillion-level capacity and flexible scalability.

## Features ##

- produce message, include sync and asynce produce message, timed and delay message. 

- consume message, include concurrency and orderly consume message, broadcast and cluster consume model.

- rebalance, include both produce message and consume message with rebalance.

- C and C++ API, include both C style and C++ style apis.

- across platform, all features are supported on both windows and linux system.

## Dependency ##
- [libevent 2.0.22](https://github.com/libevent/libevent/archive/release-2.0.22-stable.zip "libevent 2.0.22")

- [jsoncpp 0.10.6](https://github.com/open-source-parsers/jsoncpp/archive/0.10.6.zip  "jsoncpp 0.10.6")

- [boost 1.56.0](http://sourceforge.net/projects/boost/files/boost/1.56.0/boost_1_56_0.tar.gz "boost 1.56.0")

## Documentation ##
doc/rocketmq-cpp_manaual_zh.docx

## Build and Install ##

### Linux platform ###

**note**: *make sure the following compile tools or libraries with them minimum version number have been installed before run the build script build.sh*

- compile tools:
	- gcc-c++ 4.8.2: jsoncpp,boost rocket-client require it, need support C++11
	- cmake 2.8.0: jsoncpp,rocketmq-client require it
	- automake 1.11.1: libevent require it
	- libtool 2.2.6: libevent require it

- libraries:   
	- bzip2-devel 1.0.6: boost dependcy it

one key build script will automatic build the dependency libraries include libevent json and boost, then it will build rocketmq-client static and shared library.

if can't get internet to download three library source files by build script, you can copy three library source files (release-2.0.22-stable.zip  0.10.6.zip and boost_1_56_0.tar.gz) to rocketmq-client root dir, then build.sh will auto use these library files to build rocketmq-client.

    sudo sh build.sh

then there are librocketmq.a and librocketmq.so in /usr/local/lib. for use them to build application you should link with following libraries: -lrocketmq -lpthread -lz -ldl -lrt.

    g++ -o consumer_example consumer_example.cpp -L. -lrocketmq -lpthread -lz -ldl -lrt

### Windows platform: ###
#### Dependency Installation
1. install [libevent 2.0.22](https://github.com/libevent/libevent/archive/release-2.0.22-stable.zip "libevent 2.0.22")
extract libevent to C:/libevent
open Virtual Studio command line tools, go to dir: C:/libevent
execute cmd: nmake /f Makefile.nmake
cp libevent.lib, libevent_extras.lib and libevent_core.lib to C:/libevent/lib

1. install [jsoncpp 0.10.6](https://github.com/open-source-parsers/jsoncpp/archive/0.10.6.zip "jsoncpp 0.10.6")
extract jsoncpp to C:/jsoncpp
download [cmake windows tool](https://cmake.org/files/v3.9/cmake-3.9.3-win64-x64.zip "cmake windows tool") and extract
run cmake-gui.exe, choose your source code dir and build dir, then click generate which will let you choose Virtual Studio version
open project by VirtualStudio, and build jsoncpp, and jsoncpp.lib will be got

1. install [boost 1.56.0](http://sourceforge.net/projects/boost/files/boost/1.56.0/boost_1_56_0.tar.gz "boost 1.56.0")
according to following discription: http://www.boost.org/doc/libs/1_56_0/more/getting_started/windows.html
following build options are needed to be set when run bjam.exe: msvc architecture=x86 address-model=64 link=static runtime-link=static stage
all lib will be generated except boost_zlib:
download [zlib source](http://gnuwin32.sourceforge.net/downlinks/zlib-src-zip.php "zlib source") and extract to directory C:/zlib
run cmd:bjam.exe msvc architecture=x86 address-model=64 link=static runtime-link=static --with-iostreams -s ZLIB_SOURCE=C:\zlib\src\zlib\1.2.3\zlib-1.2.3 stage

#### Make and Install
run cmake-gui.exe, choose your source code dir and build dir, then click generate which will let you choose VirtualStudio version
if generate project solution fails, change BOOST_INCLUDEDIR/LIBEVENT_INCLUDE_DIR/JSONCPP_INCLUDE_DIR in CMakeList.txt, according to its real install path
open&build&run project by VirtualStudio

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
