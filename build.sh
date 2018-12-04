#!/usr/bin/bash
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

function Help()
{
    echo "=========================================one key build help============================================"
    echo "sh build.sh [build libevent:0/1 default:1] [build json:0/1 default:1] [build boost:0/1 default:1]"
    echo "usage: sh build.sh 1 1 1"
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
    echo "build jsoncpp static #####################"
    cmake .. -DCMAKE_CXX_FLAGS=-fPIC -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=${install_lib_dir}
    if [ $? -ne 0 ];then
        exit 1
    fi    
    make
    if [ $? -ne 0 ];then
        exit 1
    fi
    make install
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

function BuildRocketMQClient()
{
    cd ${build_dir}
    cmake ..
    make
    if [ $? -ne 0 ];then
        exit 1
    fi        
    #sudo make install
    PackageRocketMQStatic
}

function PackageRocketMQStatic()
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
BuildRocketMQClient