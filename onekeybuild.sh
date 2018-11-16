#!/usr/bin/bash
basepath=$(cd `dirname $0`; pwd)
down_dir="${basepath}/tmp_down_dir"
build_dir="${basepath}/tmp_build_dir"
packet_dir="${basepath}/tmp_packet_dir"
sys_lib_dir="/usr/local/lib"
bin_dir="${basepath}/bin"
fname_libevent="release-2.0.22-stable.zip"
fname_jsoncpp="0.10.6.zip"
fname_boost="boost_1_56_0.tar.gz"

function Help()
{
    echo "=========================================one key build help============================================"
    echo "./onekeybuild.sh [build libevent:0/1 default:1] [build json:0/1 default:1] [build boost:0/1 default:1]"
    echo "usage: ./onekeybuild.sh 1 1 1"
    echo "[[build libevent]: 1: need build libevent lib, 0: no need build libevent lib; default:1]"
    echo "[[build json]: 1: need build json lib, 0: no need build json lib; default:1]"
    echo "[[build boost]: 1: need build boost lib, 0: no need build boost lib; default:1]"
    echo "=========================================one key build help============================================"
    echo ""
}

if [ $# -eq 3 ];then
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
elif [ $# -gt 0 ];then
    echo "the number of parameter must 0 or 3, please see the help"
    Help
    exit 1
fi

if [ $# -ge 1 ];then
    need_build_libevent=$1
else
    need_build_libevent=1
fi

if [ $# -ge 2 ];then
    need_build_jsoncpp=$2
else
    need_build_jsoncpp=1
fi

if [ $# -ge 3 ];then
    need_build_boost=$3
else
    need_build_boost=1
fi

function PrintParams()
{
    echo "###########################################################################"
    echo "need_build_libevent: ${need_build_libevent}, need_build_jsoncpp:${need_build_jsoncpp}, need_build_boost:${need_build_boost}"
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
        sudo rm -rf ${build_dir}/*
    else
        mkdir -p ${build_dir}
    fi
	
    if [ -e ${packet_dir} ]
    then
        echo "${packet_dir} is exist"
        sudo rm -rf ${packet_dir}/*
    else
        mkdir -p ${packet_dir}
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

    echo "build libevent static #####################"
    ./configure --disable-openssl --enable-static=yes --enable-shared=no CFLAGS=-fPIC CPPFLAGS=-fPIC
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
	echo "build jsoncpp static #####################"
	cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF
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
    echo "build boost static #####################"
    sudo ./b2 cflags=-fPIC cxxflags=-fPIC --with-atomic --with-thread --with-system --with-chrono --with-date_time --with-log --with-regex --with-serialization --with-filesystem --with-locale --with-iostreams threading=multi link=static runtime-link=static release install
}

function BuildRocketMQClient()
{
    cd ${build_dir}
    cmake ..
    make
    sudo make install
	
    PackageRocketMQStatic
}

function PackageRocketMQStatic()
{
    #packet libevent,jsoncpp,boost,rocketmq,Signature to one librocketmq.a
    ar -M < ${basepath}/package_rocketmq.mri
    sudo rm -rf ${sys_lib_dir}/librocketmq.a
    sudo rm -rf ${sys_lib_dir}/libSignature.a
    cp -f librocketmq.a ${bin_dir}
    sudo cp -f librocketmq.a ${sys_lib_dir}/
}

PrintParams
Prepare
BuildLibevent
BuildJsonCPP
BuildBoost
BuildRocketMQClient
