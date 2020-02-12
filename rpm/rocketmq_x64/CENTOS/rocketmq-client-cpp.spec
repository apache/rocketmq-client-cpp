
Summary: A C++ Client of Apache RocketMQ

Name: rocketmq
Version: 1.2.5
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
cp -f $RPM_SOURCE_DIR%bin/librocketmq.so     $RPM_BUILD_ROOT%{_prefix}/lib
cp -f $RPM_SOURCE_DIR%bin/librocketmq.a     $RPM_BUILD_ROOT%{_prefix}/lib
cp -rf $RPM_SOURCE_DIR%include/*             $RPM_BUILD_ROOT%{_prefix}/include/rocketmq

# package information
%files
# set file attribute here
%defattr(-, root, root, 0755)
%{_prefix}/lib
%{_prefix}/include

%define debug_package %{nil}
%define __os_install_post %{nil}
