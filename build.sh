#!/usr/bin/bash

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

basepath=$(cd `dirname $0`; pwd)
down_dir="${basepath}/tmp_down_dir"
build_dir="${basepath}/tmp_build_dir"
packet_dir="${basepath}/tmp_packet_dir"
install_lib_dir="${basepath}/bin"
fname_libevent="libevent*.zip"
fname_jsoncpp="jsoncpp*.zip"
fname_boost="boost*.tar.gz"
fname_libevent_down="release-2.0.22-stable.zip"
fname_jsoncpp_down="0.10.6.zip"
fname_boost_down="1.58.0/boost_1_58_0.tar.gz"

PrintParams()
{
    echo "=========================================one key build help============================================"
    echo "sh build.sh [no build libevent:noEvent] [no build json:noJson] [no build boost:noBoost] [ execution test:test]"
    echo "usage: sh build.sh noJson noEvent noBoost test"
    echo "=========================================one key build help============================================"
    echo ""
}

need_build_jsoncpp=1
need_build_libevent=1
need_build_boost=1
test=0

pasres_arguments(){
    for var in "$@"
    do
        case "$var" in
                noJson)
                        need_build_jsoncpp=0
                        ;;
                noEvent)
                        need_build_libevent=0
                        ;;
                noBoost)
                        need_build_boost=0
                        ;;
                test)
                       test=1
        esac
    done

}
pasres_arguments $@

PrintParams()
{
    echo "###########################################################################"
    if [ $need_build_libevent -eq 0 ]
    then
        echo "no need build libevent lib"
    else
        echo "need build libevent lib"
    fi

    if [ $need_build_jsoncpp -eq 0 ]
    then
        echo "no need build jsoncpp lib"
    else
        echo "need build jsoncpp lib"
    fi

    if [ $need_build_boost -eq 0 ]
    then
        echo "no need build boost lib"
    else
        echo "need build boost lib"
    fi

    echo "###########################################################################"
    echo ""
}

Prepare()
{
    if [ -e ${down_dir} ]
    then
        echo "${down_dir} is exist"
        #cd ${down_dir}
        #ls |grep -v ${fname_libevent} |grep -v ${fname_jsoncpp} | grep -v ${fname_boost} |xargs rm -rf
    else
        mkdir -p ${down_dir}
    fi

    cd ${basepath}
    if [ -e ${fname_libevent} ]
    then
        mv -f ${basepath}/${fname_libevent} ${down_dir}
    fi

    if [ -e ${fname_jsoncpp} ]
    then
        mv -f ${basepath}/${fname_jsoncpp} ${down_dir}
    fi

    if [ -e ${fname_boost} ]
    then
        mv -f ${basepath}/${fname_boost} ${down_dir}
    fi

    if [ -e ${build_dir} ]
    then
        echo "${build_dir} is exist"
        #rm -rf ${build_dir}/*
    else
        mkdir -p ${build_dir}
    fi

    if [ -e ${packet_dir} ]
    then
        echo "${packet_dir} is exist"
        #rm -rf ${packet_dir}/*
    else
        mkdir -p ${packet_dir}
    fi

    if [ -e ${install_lib_dir} ]
    then
        echo "${install_lib_dir} is exist"
    else
        mkdir -p ${install_lib_dir}
    fi
}

BuildLibevent()
{
    if [ $need_build_libevent -eq 0 ]
    then
        echo "no need build libevent lib"
        return 0
    fi

    cd ${down_dir}
    if [ -e ${fname_libevent} ]
    then
        echo "${fname_libevent} is exist"
    else
        wget https://github.com/libevent/libevent/archive/${fname_libevent_down} -O libevent-${fname_libevent_down}
    fi
    unzip -o ${fname_libevent}
    if [ $? -ne 0 ];then
        exit 1
    fi

    libevent_dir=`ls | grep libevent | grep .*[^zip]$`
    cd ${libevent_dir}
    if [ $? -ne 0 ];then
        exit 1
    fi    
    ./autogen.sh
    if [ $? -ne 0 ];then
        exit 1
    fi
    echo "build libevent static #####################"    
    ./configure --disable-openssl --enable-static=yes --enable-shared=no CFLAGS=-fPIC CPPFLAGS=-fPIC --prefix=${install_lib_dir}
    if [ $? -ne 0 ];then
        exit 1
    fi    
    make
    if [ $? -ne 0 ];then
        exit 1
    fi
    make install
}


BuildJsonCPP()
{
    if [ $need_build_jsoncpp -eq 0 ];then
        echo "no need build jsoncpp lib"
        return 0
    fi

    cd ${down_dir}

    if [ -e ${fname_jsoncpp} ]
    then
        echo "${fname_jsoncpp} is exist"
    else
        wget https://github.com/open-source-parsers/jsoncpp/archive/${fname_jsoncpp_down} -O jsoncpp-${fname_jsoncpp_down}
    fi
    unzip -o ${fname_jsoncpp}
    if [ $? -ne 0 ];then
        exit 1
    fi
    jsoncpp_dir=`ls | grep ^jsoncpp | grep .*[^zip]$`
    cd ${jsoncpp_dir}
    if [ $? -ne 0 ];then
        exit 1
    fi
    mkdir build; cd build
    echo "build jsoncpp static ######################"
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir}
    if [ $? -ne 0 ];then
        exit 1
    fi    
    make
    if [ $? -ne 0 ];then
        exit 1
    fi
    make install

    if [ ! -f ${install_lib_dir}/lib/libjsoncpp.a ]
    then
        echo " ./bin/lib directory is not libjsoncpp.a"
        cp ${install_lib_dir}/lib/x86_64-linux-gnu/libjsoncpp.a ${install_lib_dir}/lib/
    fi


}

BuildBoost()
{
    if [ $need_build_boost -eq 0 ];then
        echo "no need build boost lib"
        return 0
    fi

    cd ${down_dir}
    if [ -e ${fname_boost} ]
    then
        echo "${fname_boost} is exist"
    else
        wget http://sourceforge.net/projects/boost/files/boost/${fname_boost_down}
    fi
    tar -zxvf ${fname_boost}
    boost_dir=`ls | grep boost | grep .*[^gz]$`
    cd ${boost_dir}
    if [ $? -ne 0 ];then
        exit 1
    fi
    ./bootstrap.sh
    if [ $? -ne 0 ];then
        exit 1
    fi    
    echo "build boost static #####################"
    ./b2 cflags=-fPIC cxxflags=-fPIC --with-atomic --with-thread --with-system --with-chrono --with-date_time --with-log --with-regex --with-serialization --with-filesystem --with-locale --with-iostreams threading=multi link=static runtime-link=static release install --prefix=${install_lib_dir}
    if [ $? -ne 0 ];then
        exit 1
    fi
}

BuildRocketMQClient()
{
    cd ${build_dir}
    if [ $test -eq 0 ];then
        cmake ..
    else
        cmake .. -DRUN_UNIT_TEST=ON
    fi
    make
    if [ $? -ne 0 ];then
        exit 1
    fi        
    #sudo make install
    PackageRocketMQStatic
}

BuildGoogleTest()
{
    if [ $test -eq 0 ];then
        echo "no need build google test lib"
        return 0
    fi

    if [ -f ./bin/lib/libgtest.a ]
    then
        echo "libgteest already exist no need build test"
        return 0
    fi

    cd ${down_dir}
    if [ -e release-1.8.1.tar.gz ]
    then
        echo "${fname_boost} is exist"
    else
        wget https://github.com/abseil/googletest/archive/release-1.8.1.tar.gz
    fi
    if [ ! -d "googletest-release-1.8.1" ];then
        tar -zxvf release-1.8.1.tar.gz
    fi
    cd googletest-release-1.8.1
    mkdir build; cd build
    echo "build googletest static #####################"
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir}
    if [ $? -ne 0 ];then
        exit 1
    fi    
    make
    if [ $? -ne 0 ];then
        exit 1
    fi
    make install

    if [ ! -f ${install_lib_dir}/lib/libgtest.a ]
    then
        echo " ./bin/lib directory is not libgtest.a"
        cp ${install_lib_dir}/lib64/lib* ${install_lib_dir}/lib
    fi
}

ExecutionTesting()
{
    if [ $test -eq 0 ];then
        echo "Do not execution test"
        return 0
    fi
    echo "##################  test  start  ###########"
    cd ${basepath}/test/bin
    if [ ! -d ../log ]; then
       mkdir ../log
    fi
    for files in `ls -F`
    do
        ./$files > "../log/$files.txt" 2>&1

        if [ $? -ne 0 ]; then
            echo "$files erren"
            cat ../log/$files.txt
            return 0
        fi
        erren=`grep "FAILED TEST" ../log/$files.txt`
        
        if [ -n "$erren" ]; then
            echo "##################  find erren ###########"
            cat ../log/$files.txt

            echo "##################  end ExecutionTesting ###########"
            return
        else
            echo "$files success"
        fi
    done
    echo "##################  test  end  ###########"
}


PackageRocketMQStatic()
{
    #packet libevent,jsoncpp,boost,rocketmq,Signature to one librocketmq.a
    cp -f ${basepath}/libs/signature/lib/libSignature.a ${install_lib_dir}/lib
    ar -M < ${basepath}/package_rocketmq.mri
    cp -f librocketmq.a ${install_lib_dir}
}


PrintParams
Prepare
BuildLibevent
BuildJsonCPP
BuildBoost
BuildGoogleTest
BuildRocketMQClient
ExecutionTesting
