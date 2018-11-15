#!/usr/bin/bash
basepath=$(cd `dirname $0`; pwd)
down_dir="${basepath}/tmp_down_dir"
build_dir="${basepath}/tmp_build_dir"
fname_libevent="release-2.0.22-stable.zip"
fname_jsoncpp="0.10.6.zip"
fname_boost="boost_1_56_0.tar.gz"

function Help()
{
        echo "===============================================onekeybuild help=================================================="
    echo "./onekeybuild.sh [build shared/static lib: 0/1 default:1] [build libevent:0/1 default:1] [build json:0/1 default:1] [build boost:0/1 default:1]"
        echo "usage: ./onekeybuild.sh 1 1 1 1"
    echo "[[build shared or static lib]: 1: build shared library, 0: build static library;  default:1 ]"
    echo "[[build libevent]: 1: need build libevent lib, 0: no need build libevent lib; default:1]"
    echo "[[build json]: 1: need build json lib, 0: no need build json lib; default:1]"
    echo "[[build boost]: 1: need build boost lib, 0: no need build boost lib; default:1]"
        echo "===============================================onekeybuild help=================================================="
        echo ""
}

if [ $# -ne 1 -a $# -ne 4 ];then
        echo "para value number need 1 or 4, please see the help"
        Help
        exit 1
elif [ $# -eq 1 ];then
        if [ "$1" != "0" -a "$1" != "1" ];then
                echo "unsupport para value $1, please see the help"
                Help
                exit 1
        fi
else
        if [ "$1" != "0" -a "$1" != "1" ];then
                echo "unsupport para value $1, please see the help"
                Help
                exit 1
        fi
        if [ "$2" != "0" -a "$2" != "1" ];then
                echo "unsupport para value $2, please see the help"
                Help
                exit 1
        fi
        if [ "$3" != "0" -a "$3" != "1" ];then
                echo "unsupport para value $3, please see the help"
                Help
                exit 1
        fi
        if [ "$4" != "0" -a "$4" != "1" ];then
                echo "unsupport para value $4, please see the help"
                Help
                exit 1
        fi
fi

if [ $# -ge 1 ];then
        build_shared=$1
else
    build_shared=1
fi

if [ $# -ge 2 ];then
    need_build_libevent=$2
else
    need_build_libevent=1
fi

if [ $# -ge 3 ];then
    need_build_jsoncpp=$3
else
    need_build_jsoncpp=1
fi

if [ $# -ge 4 ];then
    need_build_boost=$4
else
    need_build_boost=1
fi

function PrintParams()
{
        echo "###########################################################################"
        echo "build shared:${build_shared}, need_build_libevent: ${need_build_libevent}, need_build_jsoncpp:${need_build_jsoncpp}, need_build_boost:${need_build_boost}"
        echo "###########################################################################"
        echo ""
}

function Prepare()
{
        if [ -e ${down_dir} ]
        then
                echo "${down_dir} is exist"
                cd ${down_dir}
                ls |grep -v ${fname_libevent} |grep -v ${fname_jsoncpp} | grep -v ${fname_boost} |xargs sudo rm -rf
        else
                mkdir -p ${down_dir}
        fi

        if [ -e ${build_dir} ]
        then
                echo "${build_dir} is exist"
                #sudo rm -rf ${build_dir}/*
        else
                mkdir -p ${build_dir}
        fi
}

function BuildLibevent()
{
        if [ "${need_build_libevent}" == "0" ];then
                echo "no need build libevent lib"
                return 0
        fi

        cd ${down_dir}
        if [ -e ${fname_libevent} ]
        then
                echo "${fname_libevent} is exist"
        else
                wget https://github.com/libevent/libevent/archive/${fname_libevent}
        fi
        unzip ${fname_libevent}
        cd libevent-release-2.0.22-stable
        ./autogen.sh

        if [ "${build_shared}" == "1" ];then
                echo "build libevent shared #####################"
                ./configure --disable-openssl --enable-static=no --enable-shared=yes
        else
                echo "build libevent static #####################"
                ./configure --disable-openssl --enable-static=yes --enable-shared=no
        fi
        make
        sudo make install
}


function BuildJsonCPP()
{
        if [ "${need_build_jsoncpp}" == "0" ];then
                echo "no need build jsoncpp lib"
                return 0
        fi

        cd ${down_dir}

        if [ -e ${fname_jsoncpp} ]
        then
                echo "${fname_jsoncpp} is exist"
        else
                wget https://github.com/open-source-parsers/jsoncpp/archive/${fname_jsoncpp}
        fi
        unzip ${fname_jsoncpp}
        cd jsoncpp-0.10.6

        mkdir build; cd build
        if [ "${build_shared}" == "1" ];then
                echo "build jsoncpp shared #####################"
                cmake .. -DBUILD_STATIC_LIBS=OFF -DBUILD_SHARED_LIBS=ON
        else
                echo "build jsoncpp static #####################"
                cmake .. -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF
        fi
        make
        sudo make install
}

function BuildBoost()
{
        if [ "${need_build_boost}" == "0" ];then
                echo "no need build boost lib"
                return 0
        fi

        cd ${down_dir}
        if [ -e ${fname_boost} ]
        then
                echo "${fname_boost} is exist"
        else
                wget http://sourceforge.net/projects/boost/files/boost/1.56.0/${fname_boost}
        fi
        tar -zxvf ${fname_boost}
        cd boost_1_56_0
        ./bootstrap.sh
        if [ "${build_shared}" == "1" ];then
                echo "build boost shared #####################"
                sudo ./b2 --with-atomic --with-thread --with-system --with-chrono --with-date_time --with-log --with-regex --with-serialization --with-filesystem --with-locale --with-iostreams threading=multi link=shared runtime-link=shared install
        else
                echo "build boost static #####################"
                sudo ./b2 --with-atomic --with-thread --with-system --with-chrono --with-date_time --with-log --with-regex --with-serialization --with-filesystem --with-locale --with-iostreams threading=multi link=static runtime-link=static install
        fi
}

function BuildRocketMQClient()
{
        cd ${build_dir}
        if [ "${build_shared}" == "1" ];then
                cmake .. -DBUILD_ROCKETMQ_STATIC=OFF -DBUILD_ROCKETMQ_SHARED=ON -DBoost_USE_STATIC_LIBS=OFF -DBoost_USE_MULTITHREADED=ON -DBoost_USE_STATIC_RUNTIME=ON -DLibevent_USE_STATIC_LIBS=OFF -DJSONCPP_USE_STATIC_LIBS=OFF
        else
                cmake .. -DBUILD_ROCKETMQ_STATIC=ON -DBUILD_ROCKETMQ_SHARED=OFF -DBoost_USE_STATIC_LIBS=ON -DBoost_USE_MULTITHREADED=ON -DBoost_USE_STATIC_RUNTIME=ON -DLibevent_USE_STATIC_LIBS=ON -DJSONCPP_USE_STATIC_LIBS=ON
        fi
        make
        sudo make install
}

PrintParams
Prepare
BuildLibevent
BuildJsonCPP
BuildBoost
BuildRocketMQClient
