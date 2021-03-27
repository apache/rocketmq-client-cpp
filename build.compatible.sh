#!/usr/bin/env bash

# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

basepath=$(
  cd $(dirname $0)
  pwd
)
down_dir="${basepath}/tmp_down_dir"
build_dir="${basepath}/tmp_build_dir"
packet_dir="${basepath}/tmp_packet_dir"
install_lib_dir="${basepath}/bin"
fname_spdlog="spdlog*.zip"
fname_libevent="libevent*.zip"
fname_jsoncpp="jsoncpp*.zip"
fname_spdlog_down="v1.5.0.zip"
fname_libevent_down="release-2.1.11-stable.zip"
fname_jsoncpp_down="0.10.6.zip"

PrintParams() {
  echo "=========================================one key build help============================================"
  echo "sh build.sh [no build spdlog:noLog] [no build libevent:noEvent] [no build json:noJson] [ execution test:test]"
  echo "usage: sh build.sh noLog noJson noEvent test"
  echo "=========================================one key build help============================================"
  echo ""
}

need_build_spdlog=1
need_build_jsoncpp=1
need_build_libevent=1
test=0
verbose=1
codecov=0
cpu_num=4

pasres_arguments() {
  for var in "$@"; do
    case "$var" in
    noLog)
      need_build_spdlog=0
      ;;
    noJson)
      need_build_jsoncpp=0
      ;;
    noEvent)
      need_build_libevent=0
      ;;
    noVerbose)
      verbose=0
      ;;
    codecov)
      codecov=1
      ;;
    test)
      test=1
      ;;
    esac
  done

}
pasres_arguments $@

PrintParams() {
  echo "###########################################################################"

  if [ $need_build_spdlog -eq 0 ]; then
    echo "no need build spdlog lib"
  else
    echo "need build spdlog lib"
  fi

  if [ $need_build_libevent -eq 0 ]; then
    echo "no need build libevent lib"
  else
    echo "need build libevent lib"
  fi

  if [ $need_build_jsoncpp -eq 0 ]; then
    echo "no need build jsoncpp lib"
  else
    echo "need build jsoncpp lib"
  fi

  if [ $test -eq 1 ]; then
    echo "build unit tests"
  else
    echo "without build unit tests"
  fi

  if [ $codecov -eq 1 ]; then
    echo "run unit tests with code coverage"
  fi

  if [ $verbose -eq 0 ]; then
    echo "no need print detail logs"
  else
    echo "need print detail logs"
  fi

  echo "###########################################################################"
  echo ""
}

Prepare() {
  if [ -e ${down_dir} ]; then
    echo "${down_dir} is exist"
  else
    mkdir -p ${down_dir}
  fi

  cd ${basepath}

  if [ -e ${fname_spdlog} ]; then
    mv -f ${basepath}/${fname_spdlog} ${down_dir}
  fi

  if [ -e ${fname_libevent} ]; then
    mv -f ${basepath}/${fname_libevent} ${down_dir}
  fi

  if [ -e ${fname_jsoncpp} ]; then
    mv -f ${basepath}/${fname_jsoncpp} ${down_dir}
  fi

  if [ -e ${build_dir} ]; then
    echo "${build_dir} is exist"
  else
    mkdir -p ${build_dir}
  fi

  if [ -e ${packet_dir} ]; then
    echo "${packet_dir} is exist"
  else
    mkdir -p ${packet_dir}
  fi

  if [ -e ${install_lib_dir} ]; then
    echo "${install_lib_dir} is exist"
  else
    mkdir -p ${install_lib_dir}
  fi
}

BuildSpdlog() {
  if [ $need_build_spdlog -eq 0 ]; then
    echo "no need build spdlog lib"
    return 0
  fi

  if [ -d "${basepath}/bin/include/spdlog" ]; then
    echo "spdlog already exist no need build test"
    return 0
  fi

  cd ${down_dir}
  if [ -e ${fname_spdlog} ]; then
    echo "${fname_spdlog} is exist"
  else
    wget https://github.com/gabime/spdlog/archive/${fname_spdlog_down} -O spdlog-${fname_spdlog_down}
  fi
  unzip -o ${fname_spdlog} >unzipspdlog.txt 2>&1

  spdlog_dir=$(ls -d spdlog* | grep -v zip)
  cd ${spdlog_dir}
  cp -r include ${install_lib_dir}

  echo "build spdlog success."
}

BuildLibevent() {
  if [ $need_build_libevent -eq 0 ]; then
    echo "no need build libevent lib"
    return 0
  fi

  if [ -d "${basepath}/bin/include/event2" ]; then
    echo "spdlog already exist no need build test"
    return 0
  fi

  cd ${down_dir}
  if [ -e ${fname_libevent} ]; then
    echo "${fname_libevent} is exist"
  else
    wget https://github.com/libevent/libevent/archive/${fname_libevent_down} -O libevent-${fname_libevent_down}
  fi
  unzip -o ${fname_libevent} >unziplibevent.txt 2>&1

  libevent_dir=$(ls -d libevent* | grep -v zip)
  cd ${libevent_dir}
  ./autogen.sh
  echo "build libevent static #####################"
  if [ $verbose -eq 0 ]; then
    ./configure --disable-openssl --enable-static=yes --enable-shared=no CFLAGS=-fPIC CPPFLAGS=-fPIC --prefix=${install_lib_dir} >libeventconfig.txt 2>&1
  else
    ./configure --disable-openssl --enable-static=yes --enable-shared=no CFLAGS=-fPIC CPPFLAGS=-fPIC --prefix=${install_lib_dir}
  fi
  if [ $verbose -eq 0 ]; then
    echo "build libevent without detail log."
    make -j $cpu_num >libeventbuild.txt 2>&1
  else
    make -j $cpu_num
  fi
  make install
  echo "build libevent success."
}

BuildJsonCPP() {
  if [ $need_build_jsoncpp -eq 0 ]; then
    echo "no need build jsoncpp lib"
    return 0
  fi

  if [ -d "${basepath}/bin/include/jsoncpp" ]; then
    echo "jsoncpp already exist no need build test"
    return 0
  fi

  cd ${down_dir}
  if [ -e ${fname_jsoncpp} ]; then
    echo "${fname_jsoncpp} is exist"
  else
    wget https://github.com/open-source-parsers/jsoncpp/archive/${fname_jsoncpp_down} -O jsoncpp-${fname_jsoncpp_down}
  fi
  unzip -o ${fname_jsoncpp} >unzipjsoncpp.txt 2>&1
  jsoncpp_dir=$(ls -d jsoncpp* | grep -v zip)
  cd ${jsoncpp_dir}
  mkdir -p build
  cd build
  echo "build jsoncpp static ######################"
  if [ $verbose -eq 0 ]; then
    echo "build jsoncpp without detail log."
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir} >jsoncppbuild.txt 2>&1
  else
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir}
  fi
  if [ $verbose -eq 0 ]; then
    make -j $cpu_num >jsoncppbuild.txt 2>&1
  else
    make -j $cpu_num
  fi
  make install
  echo "build jsoncpp success."
  if [ ! -f ${install_lib_dir}/lib/libjsoncpp.a ]; then
    echo " ./bin/lib directory is not libjsoncpp.a"
    cp ${install_lib_dir}/lib/x86_64-linux-gnu/libjsoncpp.a ${install_lib_dir}/lib/
  fi
}

BuildRocketMQClient() {
  cd ${build_dir}
  echo "============start to build rocketmq client cpp.========="
  if [ $test -eq 0 ]; then
    cmake -DLibevent_USE_STATIC_LIBS=ON -DJSONCPP_USE_STATIC_LIBS=ON -DBUILD_ROCKETMQ_STATIC=ON -DBUILD_ROCKETMQ_SHARED=OFF ..
  else
    if [ $codecov -eq 1 ]; then
      cmake .. -DLibevent_USE_STATIC_LIBS=ON -DJSONCPP_USE_STATIC_LIBS=ON -DBUILD_ROCKETMQ_STATIC=ON -DBUILD_ROCKETMQ_SHARED=OFF -DRUN_UNIT_TEST=ON -DCODE_COVERAGE=ON
    else
      cmake .. -DLibevent_USE_STATIC_LIBS=ON -DJSONCPP_USE_STATIC_LIBS=ON -DBUILD_ROCKETMQ_STATIC=ON -DBUILD_ROCKETMQ_SHARED=OFF -DRUN_UNIT_TEST=ON
    fi
  fi
  if [ $verbose -eq 0 ]; then
    echo "build rocketmq without detail log."
    make -j $cpu_num >buildclient.txt 2>&1
  else
    make -j $cpu_num
  fi
  #sudo make install
}

BuildGoogleTest() {
  if [ $test -eq 0 ]; then
    echo "no need build google test lib"
    return 0
  fi

  if [ -f ./bin/lib/libgtest.a ]; then
    echo "libgtest already exist no need build test"
    return 0
  fi

  cd ${down_dir}
  if [ -e release-1.10.0.tar.gz ]; then
    echo "${fname_boost} is exist"
  else
    wget https://github.com/abseil/googletest/archive/release-1.10.0.tar.gz
  fi
  if [ ! -d "googletest-release-1.10.0" ]; then
    tar -zxvf release-1.10.0.tar.gz >googletest.txt 2>&1
  fi
  cd googletest-release-1.10.0
  mkdir build
  cd build
  echo "build googletest static #####################"
  if [ $verbose -eq 0 ]; then
    echo "build googletest without detail log."
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir} >googletestbuild.txt 2>&1
  else
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir}
  fi
  if [ $verbose -eq 0 ]; then
    make -j $cpu_num >gtestbuild.txt 2>&1
  else
    make -j $cpu_num
  fi
  make install

  if [ ! -f ${install_lib_dir}/lib/libgtest.a ]; then
    echo " ./bin/lib directory is not libgtest.a"
    cp ${install_lib_dir}/lib64/lib* ${install_lib_dir}/lib
  fi
}

ExecutionTesting() {
  if [ $test -eq 0 ]; then
    echo "Build success without executing unit tests."
    return 0
  fi
  echo "############# unit test  start  ###########"
  cd ${build_dir}
  if [ $verbose -eq 0 ]; then
    ctest
  else
    ctest -V
  fi
  echo "############# unit test  finish  ###########"
}

PrintParams
Prepare
BuildSpdlog
BuildLibevent
BuildJsonCPP
BuildGoogleTest
BuildRocketMQClient
ExecutionTesting
