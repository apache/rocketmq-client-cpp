# RocketMQ-Client-CPP #

RocketMQ-Client-CPP is the C/C++ client of Apache RocketMQ which is a distributed messaging and streaming platform with low latency, high performance and reliability, trillion-level capacity and flexible scalability.

## Features ##

- produce message, include sync and asynce produce message, timed and delay message. 

- consume message, include concurrency and orderly consume message, broadcast and cluster consume model.

- rebalance, include both produce message and consume message with rebalance.

- C and C++ API, include both C style and C++ style apis.

- across platform, all features are supported on both windows and linux system.

## Dependency ##
- libevent 2.0.22

- jsoncpp 0.10.6

- boost 1.56.0

## Documentation ##
doc/rocketmq-cpp_manaual_zh.docx

## Build and Install ##

### Linux platform ###

**note**: *make sure the following compile tools or libraries with the indicated minimum version number have been installed before install dependency libraries*

- compile tools:
	- **gcc-c++ 4.8.2**: jsoncpp,boost rocket-client require it, need support C++11
	- **cmake 2.8.0**: jsoncpp,rocketmq-client require it
	- **automake 1.11.1**: libevent require it
	- **libtool 2.2.6**: libevent require it

- library:
	- **bzip2-devel 1.0.6**: boost dependcy it

#### Dependency Installation ####

1. install [libevent 2.0.22](https://github.com/libevent/libevent/archive/release-2.0.22-stable.zip "libevent 2.0.22")
```shell
./autogen.sh
./configure CFLAGS=-fPIC CPPFLAGS=-fPIC --disable-openssl --enable-static=yes --enable-shared=no
make
sudo make install
```

2. install [jsoncpp 0.10.6](https://github.com/open-source-parsers/jsoncpp/archive/0.10.6.zip  "jsoncpp 0.10.6")
```shell
mkdir build; cd build
cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF
make
sudo make install
```

3. install [boost 1.56.0](http://sourceforge.net/projects/boost/files/boost/1.56.0/boost_1_56_0.tar.gz "boost 1.56.0")
```shell
./bootstrap.sh
sudo ./b2 cflags=-fPIC cxxflags=-fPIC --with-atomic --with-thread --with-system --with-chrono --with-date_time --with-log --with-regex --with-serialization --with-filesystem --with-locale --with-iostreams threading=multi link=static runtime-link=static release install
```

#### 2. Make and Install ####
```shell
mkdir build; cd build
cmake .. -DBUILD_ROCKETMQ_STATIC=ON -DBUILD_ROCKETMQ_SHARED=ON -DBoost_USE_STATIC_LIBS=ON -	 DBoost_USE_MULTITHREADED=ON -DBoost_USE_STATIC_RUNTIME=ON -DLibevent_USE_STATIC_LIBS=ON -DJSONCPP_USE_STATIC_LIBS=ON
make
sudo make install
```
when user need static library, we can package all the static library of libevent jsoncpp boost rocketmq and signature to one static library, so user can only link to librocketmq.a, no need to link libevent jsoncpp and boost again. create a file named packet_rocketmq.mri with the content as follows:

```shell
create librocketmq.a
addlib /usr/local/lib/libboost_chrono.a
addlib /usr/local/lib/libboost_date_time.a
addlib /usr/local/lib/libboost_filesystem.a
addlib /usr/local/lib/libboost_iostreams.a
addlib /usr/local/lib/libboost_locale.a
addlib /usr/local/lib/libboost_log.a
addlib /usr/local/lib/libboost_log_setup.a
addlib /usr/local/lib/libboost_regex.a
addlib /usr/local/lib/libboost_serialization.a
addlib /usr/local/lib/libboost_system.a
addlib /usr/local/lib/libboost_thread.a
addlib /usr/local/lib/libboost_wserialization.a
addlib /usr/local/lib/libevent.a
addlib /usr/local/lib/libevent_core.a
addlib /usr/local/lib/libevent_extra.a
addlib /usr/local/lib/libevent_pthreads.a
addlib /usr/local/lib/libjsoncpp.a
addlib /usr/local/lib/librocketmq.a
addlib /usr/local/lib/libSignature.a
save
end
```
    
then execute the flowing command:

```shell
ar -M < package_rocketmq.mri
sudo rm -rf /usr/local/lib/librocketmq.a
sudo rm -rf /usr/local/lib/libSignature.a
cp -f librocketmq.a ./bin
sudo cp -f librocketmq.a /usr/local/lib
```

librocketmq.a and librocketmq.so will be exist in local folder bin and system folder /usr/local/lib

### Windows platform: ###
#### Dependency Installation
1. install [libevent 2.0.22](https://github.com/libevent/libevent/archive/release-2.0.22-stable.zip "libevent 2.0.22")
extract libevent to C:/libevent
open Virtual Studio command line tools, go to dir: C:/libevent
execute cmd: nmake /f Makefile.nmake
cp libevent.lib, libevent_extras.lib and libevent_core.lib to C:/libevent/lib

2. install [jsoncpp 0.10.6](https://github.com/open-source-parsers/jsoncpp/archive/0.10.6.zip "jsoncpp 0.10.6")
extract jsoncpp to C:/jsoncpp
download [cmake windows tool](https://cmake.org/files/v3.9/cmake-3.9.3-win64-x64.zip "cmake windows tool") and extract
run cmake-gui.exe, choose your source code dir and build dir, then click generate which will let you choose Virtual Studio version
open project by VirtualStudio, and build jsoncpp, and jsoncpp.lib will be got

3. install [boost 1.56.0](http://sourceforge.net/projects/boost/files/boost/1.56.0/boost_1_56_0.tar.gz "boost 1.56.0")
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
-m	: message count (default value:1)
-c	: message content (default value: only test)
-b	: consume model (default value: CLUSTER)
-a	: set sync push (default value: async)
-r	: setup retry times (default value:5 times)
-u	: select active broker to send message (default value: false)
-d	: use AutoDeleteSendcallback by cpp client (defalut value: false)
-T	: thread count of send msg or consume message (defalut value: system cpu core number)
-v	: print more details information
```
