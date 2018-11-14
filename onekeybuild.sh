#/usr/bin/bash

basepath=$(cd `dirname $0`; pwd)
down_dir="${basepath}/tmp_down_dir"
build_dir="${basepath}/tmp_build_dir"
fname_libevent="release-2.0.22-stable.zip"
fname_jsoncpp="0.10.6.zip"
fname_boost="boost_1_56_0.tar.gz"

#function ParseParams()
#{
#build_rocketmq_static=1
#build_rocketmq_shared=0
#if ($# -ge 1 && $1 -eq 1)
#    need_build_static=0
#    need_build_shared=1
#fi
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

#}

function Help()
{
    echo "build.sh [build libevent:0/1 default:1] [build json:0/1 default:1] [build boost:0/1 default:1]"
    #echo "build.sh [build rocketmq static or shared lib: 0/1 default:0] [build libevent:0/1 default:1] [build json:0/1 default:1] [build boost:0/1 default:1]"
    #echo "[build rocketmq static or shared lib: 0 : build static library, 1: build shared library]"
    echo "[build libevent: 0: need build libevent lib, 1: no need build libevent lib]"
    echo "[build json: 0: need build json lib, 1: no need build json lib]"
    echo "[build boost: 0: need build boost lib, 1: no need build boost lib]"
}

function Prepare()
{
	echo "need_build_libevent: ${need_build_libevent}, need_build_jsoncpp:${need_build_jsoncpp}, need_build_boost:${need_build_boost}"

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
		rm -rf ${build_dir}/*
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
	./configure --disable-openssl --enable-shared=false
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
	cmake .. -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=OFF
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
	sudo ./b2 --with-atomic --with-thread --with-system --with-chrono --with-date_time --with-log --with-regex --with-serialization --with-filesystem --with-locale --with-iostreams threading=multi link=static runtime-link=static install
}

function BuildRocketMQClient()
{
	cd ${build_dir}
	cmake .. -DBUILD_ROCKETMQ_STATIC=ON -DBUILD_ROCKETMQ_SHARED=OFF -DBoost_USE_STATIC_LIBS=ON -DBoost_USE_MULTITHREADED=ON -DBoost_USE_STATIC_RUNTIME=ON -DLibevent_USE_STATIC_LIBS=ON -DJSONCPP_USE_STATIC_LIBS=ON
	make
	sudo make install
}

Prepare
BuildLibevent
BuildJsonCPP
BuildBoost
BuildRocketMQClient
