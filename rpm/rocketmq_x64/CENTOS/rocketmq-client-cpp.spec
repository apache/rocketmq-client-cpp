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

Summary: A C++ Client of Apache RocketMQ

Name: rocketmq-client-cpp
Version: 2.2.0
Release: centos
Group: Apache
License: APLv2
Source: https://github.com/apache/rocketmq-client-cpp
URL: http://rocketmq.apache.org/
Distribution: Linux

%define _prefix /usr/local

AutoReqProv: no

%description
A C++ Client of Apache RocketMQ

%prep

pwd

cat /etc/redhat-release|sed -r 's/.* ([0-9]+)\..*/\1/'

OS_VERSION=`cat /etc/redhat-release|sed -r 's/.* ([0-9]+)\..*/\1/'`

echo "OS_VERSION=${OS_VERSION}"


%build

%install
# create dirs
mkdir -p $RPM_BUILD_ROOT%{_prefix}

# create dirs
mkdir -p $RPM_BUILD_ROOT%{_prefix}/lib
mkdir -p $RPM_BUILD_ROOT%{_prefix}/include/rocketmq

# copy files
cp -f ${RPM_SOURCE_DIR}/rocketmq/bin/librocketmq.so     $RPM_BUILD_ROOT%{_prefix}/lib
cp -f ${RPM_SOURCE_DIR}/rocketmq/bin/librocketmq.a     $RPM_BUILD_ROOT%{_prefix}/lib
cp -rf ${RPM_SOURCE_DIR}/rocketmq/include/*             $RPM_BUILD_ROOT%{_prefix}/include/rocketmq

%post
# As '/usr/local/lib' is not included in dynamic libraries on CentOS, it should be made explicit to dlopen() 
echo "/usr/local/lib" > /etc/ld.so.conf.d/librocketmq.x86_64.conf
/sbin/ldconfig

# package information
%files
# set file attribute here
%defattr(-, root, root, 0755)
%{_prefix}/lib
%{_prefix}/include

%define debug_package %{nil}
%define __os_install_post %{nil}
