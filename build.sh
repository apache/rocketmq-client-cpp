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

basepath=$(
  cd $(dirname $0)
  pwd
)
declare down_dir="${basepath}/tmp_down_dir"
declare build_dir="${basepath}/tmp_build_dir"
declare packet_dir="${basepath}/tmp_packet_dir"
declare install_lib_dir="${basepath}/bin"
declare static_package_dir="${basepath}/tmp_static_package_dir"
declare fname_openssl="openssl*.tar.gz"

declare fname_libevent="libevent*.zip"
declare fname_jsoncpp="jsoncpp*.zip"
declare fname_boost="boost*.tar.gz"
declare fname_openssl_down="openssl-3.0.15.tar.gz"
declare fname_libevent_down="release-2.1.12-stable.zip"
declare fname_jsoncpp_down="0.10.7.zip"
declare fname_boost_down="1.84.0/boost_1_84_0.tar.gz"

PrintParams() {
  echo "=========================================one key build help============================================"
  echo "sh build.sh [no build libevent:noEvent] [no build json:noJson] [no build boost:noBoost] [ execution test:test]"
  echo "usage: sh build.sh noJson noEvent noBoost test"
  echo "=========================================one key build help============================================"
  echo ""
}

if test "$(uname)" = "Linux"; then
  declare cpu_num=$(cat /proc/cpuinfo | grep "processor" | wc -l)
elif test "$(uname)" = "Darwin" ; then
  declare cpu_num=$(sysctl -n machdep.cpu.thread_count)
fi

declare need_build_openssl=1
declare need_build_libevent=1
declare need_build_jsoncpp=1
declare need_build_boost=1
declare enable_asan=0
declare enable_lsan=0
declare verbose=1
declare codecov=0
declare debug=0
declare test=0

pasres_arguments() {
  for var in "$@"; do
    case "$var" in
    noOpenSSL)
      need_build_openssl=0
      ;;
    noEvent)
      need_build_libevent=0
      ;;
    noJson)
      need_build_jsoncpp=0
      ;;
    noBoost)
      need_build_boost=0
      ;;
    asan)
      enable_asan=1
      ;;
    lsan)
      enable_lsan=1
      ;;
    noVerbose)
      verbose=0
      ;;
    codecov)
      codecov=1
      ;;
    debug)
      debug=1
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
  if [ $need_build_openssl -eq 0 ]; then
    echo "no need build openssl lib"
  else
    echo "need build openssl lib"
  fi
  if [ $need_build_jsoncpp -eq 0 ]; then
    echo "no need build jsoncpp lib"
  else
    echo "need build jsoncpp lib"
  fi
  if [ $need_build_libevent -eq 0 ]; then
    echo "no need build libevent lib"
  else
    echo "need build libevent lib"
  fi
  if [ $need_build_boost -eq 0 ]; then
    echo "no need build boost lib"
  else
    echo "need build boost lib"
  fi
  if [ $enable_asan -eq 1 ]; then
    echo "enable asan reporting"
  else
    echo "disable asan reporting"
  fi
  if [ $enable_lsan -eq 1 ]; then
    echo "enable lsan reporting"
  else
    echo "disable lsan reporting"
  fi
  if [ $verbose -eq 0 ]; then
    echo "no need print detail logs"
  else
    echo "need print detail logs"
  fi
  if [ $codecov -eq 1 ]; then
    echo "run unit tests with code coverage"
  else
    echo "run unit tests without code coverage"
  fi
  if [ $debug -eq 1 ]; then
    echo "enable debug"
  else
    echo "disable debug"
  fi
  if [ $test -eq 1 ]; then
    echo "build unit tests"
  else
    echo "without build unit tests"
  fi

  echo "###########################################################################"
  echo ""
}

Prepare() {
  if [ -e ${down_dir} ]; then
    echo "${down_dir} exists"
    #cd ${down_dir}
    #ls |grep -v ${fname_libevent} |grep -v ${fname_jsoncpp} | grep -v ${fname_boost} |xargs rm -rf
  else
    mkdir -p ${down_dir}
  fi

  cd ${basepath}
  if [ -e ${fname_openssl} ]; then
    mv -f ${basepath}/${fname_openssl} ${down_dir}
  fi

  if [ -e ${fname_libevent} ]; then
    mv -f ${basepath}/${fname_libevent} ${down_dir}
  fi

  if [ -e ${fname_jsoncpp} ]; then
    mv -f ${basepath}/${fname_jsoncpp} ${down_dir}
  fi

  if [ -e ${fname_boost} ]; then
    mv -f ${basepath}/${fname_boost} ${down_dir}
  fi

  if [ -e ${build_dir} ]; then
    echo "${build_dir} exists"
    #rm -rf ${build_dir}/*
  else
    mkdir -p ${build_dir}
  fi

  if [ -e ${packet_dir} ]; then
    echo "${packet_dir} exists"
    #rm -rf ${packet_dir}/*
  else
    mkdir -p ${packet_dir}
  fi

  if [ -e ${install_lib_dir} ]; then
    echo "${install_lib_dir} exists"
  else
    mkdir -p ${install_lib_dir}
  fi
}

BuildOpenSSL() {
  if [ $need_build_openssl -eq 0 ]; then
    echo "no need build openssl lib"
    return 0
  fi

  cd ${down_dir}
  if [ -e ${fname_openssl} ]; then
    echo "${fname_openssl} exists"
  else
    wget https://www.openssl.org/source/old/3.0/${fname_openssl_down} -O ${fname_openssl_down} --no-check-certificate
  fi
  tar -zxvf ${fname_openssl} &> unzipopenssl.txt
  if [ $? -ne 0 ]; then
    exit 1
  fi

  openssl_dir=$(ls | grep ^openssl | grep .*[^gz]$)
  cd ${openssl_dir}
  if [ $? -ne 0 ]; then
    exit 1
  fi
  echo "build openssl static #####################"
  if [ $verbose -eq 0 ]; then
    ./config shared CFLAGS=-fPIC CPPFLAGS=-fPIC --prefix=${install_lib_dir} --openssldir=${install_lib_dir} enable-camellia enable-seed enable-rfc3779 enable-cms enable-md2 enable-rc5 nable-weak-ssl-ciphers enable-ssl3 enable-ssl3-method enable-md2 enable-ktls enable-fips &> opensslconfig.txt
  else
    ./config shared CFLAGS=-fPIC CPPFLAGS=-fPIC --prefix=${install_lib_dir} --openssldir=${install_lib_dir} enable-camellia enable-seed enable-rfc3779 enable-cms enable-md2 enable-rc5 enable-weak-ssl-ciphers enable-ssl3 enable-ssl3-method enable-md2 enable-ktls enable-fips
  fi
  if [ $? -ne 0 ]; then
    exit 1
  fi
  if [ $verbose -eq 0 ]; then
    echo "build openssl without detail log."
    make depend &> opensslbuild.txt
    make -j $cpu_num &> opensslbuild.txt
  else
    make depend
    make -j $cpu_num
  fi
  if [ $? -ne 0 ]; then
    exit 1
  fi
  make install
  echo "build openssl success."
}

BuildLibevent() {
  if [ $need_build_libevent -eq 0 ]; then
    echo "no need build libevent lib"
    return 0
  fi

  cd ${down_dir}
  if [ -e ${fname_libevent} ]; then
    echo "${fname_libevent} exists"
  else
    wget https://github.com/libevent/libevent/archive/${fname_libevent_down} -O libevent-${fname_libevent_down}
  fi
  unzip -o ${fname_libevent} &> unziplibevent.txt
  if [ $? -ne 0 ]; then
    exit 1
  fi

  libevent_dir=$(ls | grep ^libevent | grep .*[^zip^txt]$)
  cd ${libevent_dir}
  if [ $? -ne 0 ]; then
    exit 1
  fi
  ./autogen.sh
  if [ $? -ne 0 ]; then
    exit 1
  fi
  echo "build libevent static #####################"
  if [ $verbose -eq 0 ]; then
    ./configure --enable-static=yes --enable-shared=no CFLAGS="-fPIC -I${install_lib_dir}/include" CPPFLAGS="-fPIC -I${install_lib_dir}/include" LDFLAGS="-L${install_lib_dir}/lib" --prefix=${install_lib_dir} &> libeventconfig.txt
  else
    ./configure --enable-static=yes --enable-shared=no CFLAGS="-fPIC -I${install_lib_dir}/include" CPPFLAGS="-fPIC -I${install_lib_dir}/include" LDFLAGS="-L${install_lib_dir}/lib" --prefix=${install_lib_dir}
  fi
  if [ $? -ne 0 ]; then
    exit 1
  fi
  if [ $verbose -eq 0 ]; then
    echo "build libevent without detail log."
    make -j $cpu_num &> libeventbuild.txt
  else
    make -j $cpu_num
  fi
  if [ $? -ne 0 ]; then
    exit 1
  fi
  make install
  echo "build linevent success."
}

BuildJsonCPP() {
  if [ $need_build_jsoncpp -eq 0 ]; then
    echo "no need build jsoncpp lib"
    return 0
  fi

  cd ${down_dir}

  if [ -e ${fname_jsoncpp} ]; then
    echo "${fname_jsoncpp} exists"
  else
    wget https://github.com/open-source-parsers/jsoncpp/archive/${fname_jsoncpp_down} -O jsoncpp-${fname_jsoncpp_down}
  fi
  unzip -o ${fname_jsoncpp} &> unzipjsoncpp.txt
  if [ $? -ne 0 ]; then
    exit 1
  fi
  jsoncpp_dir=$(ls | grep ^jsoncpp | grep .*[^zip]$)
  cd ${jsoncpp_dir}
  if [ $? -ne 0 ]; then
    exit 1
  fi
  mkdir build
  cd build
  echo "build jsoncpp static ######################"
  if [ $verbose -eq 0 ]; then
    echo "build jsoncpp without detail log."
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir} &> jsoncppbuild.txt
  else
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir}
  fi
  if [ $? -ne 0 ]; then
    exit 1
  fi
  if [ $verbose -eq 0 ]; then
    make -j $cpu_num &> jsoncppbuild.txt
  else
    make -j $cpu_num
  fi
  if [ $? -ne 0 ]; then
    exit 1
  fi
  make install
  echo "build jsoncpp success."
  if [ ! -f ${install_lib_dir}/lib/libjsoncpp.a ]; then
    echo " ./bin/lib directory is not libjsoncpp.a"
    cp ${install_lib_dir}/lib/$(uname -m)-linux-gnu/libjsoncpp.a ${install_lib_dir}/lib/
  fi
}

BuildBoost() {
  if [ $need_build_boost -eq 0 ]; then
    echo "no need build boost lib"
    return 0
  fi

  cd ${down_dir}
  if [ -e ${fname_boost} ]; then
    echo "${fname_boost} exists"
  else
    wget http://sourceforge.net/projects/boost/files/boost/${fname_boost_down} --no-check-certificate
  fi
  tar -zxvf ${fname_boost} &> unzipboost.txt
  boost_dir=$(ls | grep ^boost | grep .*[^gz]$)
  cd ${boost_dir}
  if [ $? -ne 0 ]; then
    exit 1
  fi
  ./bootstrap.sh
  if [ $? -ne 0 ]; then
    exit 1
  fi
  echo "build boost static #####################"
  pwd
  if [ $verbose -eq 0 ]; then
    echo "build boost without detail log."
    ./b2 -j$cpu_num cflags=-fPIC cxxflags=-fPIC --with-atomic --with-thread --with-system --with-chrono --with-date_time --with-log --with-regex --with-serialization --with-filesystem --with-locale --with-iostreams threading=multi link=static release install --prefix=${install_lib_dir} &> boostbuild.txt
  else
    ./b2 -j$cpu_num cflags=-fPIC cxxflags=-fPIC --with-atomic --with-thread --with-system --with-chrono --with-date_time --with-log --with-regex --with-serialization --with-filesystem --with-locale --with-iostreams threading=multi link=static release install --prefix=${install_lib_dir}
  fi
  if [ $? -ne 0 ]; then
    exit 1
  fi
}

BuildRocketMQClient() {
  cd ${build_dir}
  echo "============start to build rocketmq client cpp.========="
  local ROCKETMQ_CMAKE_FLAG=""
  if [ $test -eq 1 ]; then
    if [ $codecov -eq 1 ]; then
      ROCKETMQ_CMAKE_FLAG=$ROCKETMQ_CMAKE_FLAG" -DRUN_UNIT_TEST=ON -DCODE_COVERAGE=ON"
    else
      ROCKETMQ_CMAKE_FLAG=$ROCKETMQ_CMAKE_FLAG" -DRUN_UNIT_TEST=ON -DCODE_COVERAGE=OFF"
    fi
  else
      ROCKETMQ_CMAKE_FLAG=$ROCKETMQ_CMAKE_FLAG" -DRUN_UNIT_TEST=OFF -DCODE_COVERAGE=OFF"
  fi
  if [ $enable_asan -eq 1 ]; then
      ROCKETMQ_CMAKE_FLAG=$ROCKETMQ_CMAKE_FLAG" -DENABLE_ASAN=ON"
  else
      ROCKETMQ_CMAKE_FLAG=$ROCKETMQ_CMAKE_FLAG" -DENABLE_ASAN=OFF"
  fi
  if [ $enable_lsan -eq 1 ]; then
      ROCKETMQ_CMAKE_FLAG=$ROCKETMQ_CMAKE_FLAG" -DENABLE_LSAN=ON"
  else
      ROCKETMQ_CMAKE_FLAG=$ROCKETMQ_CMAKE_FLAG" -DENABLE_LSAN=OFF"
  fi
  if [ $debug -eq 1 ]; then
      ROCKETMQ_CMAKE_FLAG=$ROCKETMQ_CMAKE_FLAG" -DCMAKE_BUILD_TYPE=Debug"
  else
      ROCKETMQ_CMAKE_FLAG=$ROCKETMQ_CMAKE_FLAG" -DCMAKE_BUILD_TYPE=Release"
  fi
  cmake .. $ROCKETMQ_CMAKE_FLAG
  if [ $verbose -eq 0 ]; then
    echo "build rocketmq without detail log."
    make -j $cpu_num &> buildclient.txt
  else
    make -j $cpu_num
  fi
  if [ $? -ne 0 ]; then
    echo "build error....."
    exit 1
  fi
  #sudo make install
  PackageRocketMQStatic
}

BuildGoogleTest() {
  if [ $test -eq 0 ]; then
    echo "no need build google test lib"
    return 0
  fi
  if [ -f ./bin/lib/libgtest.a ]; then
    echo "GTest already exists, no need build"
    return 0
  fi
  cd ${down_dir}
  if [ -e release-1.8.1.tar.gz ]; then
    echo "${fname_boost} exists"
  else
    wget https://github.com/abseil/googletest/archive/release-1.8.1.tar.gz
  fi
  if [ ! -d "googletest-release-1.8.1" ]; then
    tar -zxvf release-1.8.1.tar.gz &> googletest.txt
  fi
  cd googletest-release-1.8.1
  mkdir -p build
  cd build
  echo "build googletest static #####################"
  if [ $verbose -eq 0 ]; then
    echo "build googletest without detail log."
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir} &> googletestbuild.txt
  else
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir}
  fi
  if [ $? -ne 0 ]; then
    exit 1
  fi
  if [ $verbose -eq 0 ]; then
    make -j $cpu_num &> gtestbuild.txt
  else
    make -j $cpu_num
  fi
  if [ $? -ne 0 ]; then
    exit 1
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
  if [ $? -ne 0 ]; then
    echo "############# unit test failed  ###########"
    exit 1
  fi
  echo "############# unit test  finish  ###########"
}

PackageRocketMQStatic() {
  echo "############# Start package static rocketmq library. #############"
  if test "$(uname)" = "Linux"; then
    #packet libevent,jsoncpp,boost,rocketmq,Signature to one librocketmq.a
    cp -f ${basepath}/libs/signature/lib/libSignature.a ${install_lib_dir}/lib
    ar -M <${basepath}/package_rocketmq.mri
    cp -f librocketmq.a ${install_lib_dir}
  elif test "$(uname)" = "Darwin" ; then
    mkdir -p ${static_package_dir}
    cd ${static_package_dir}
    cp -f ${basepath}/libs/signature/lib/libSignature.a .
    cp -f ${install_lib_dir}/lib/lib*.a .
    cp -f ${install_lib_dir}/librocketmq.a .
    echo "Md5 Hash RocketMQ Before:"
    md5sum librocketmq.a
    local dir=`ls *.a | grep -E 'gtest|gmock'`
    for i in $dir
    do
      rm -rf $i
    done
    libtool -no_warning_for_no_symbols -static -o librocketmq.a *.a
    echo "Md5 Hash RocketMQ After:"
    md5sum librocketmq.a
    echo "Try to copy $(pwd)/librocketmq.a to ${install_lib_dir}/"
    cp -f librocketmq.a  ${install_lib_dir}/
    cd ${basepath}
    rm -rf ${static_package_dir}
  fi
  echo "############# Package static rocketmq library success.#############"
}

PrintParams
Prepare
BuildOpenSSL
BuildLibevent
BuildJsonCPP
BuildBoost
BuildGoogleTest
BuildRocketMQClient
ExecutionTesting
