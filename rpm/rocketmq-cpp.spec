Name: rocketmq-cpp
Version: 1.0.0
Release: %(echo $RELEASE)%{?dist}
Summary: Pop-based RocketMQ C++ SDK
Group: alibaba/application
License: Commercial
%define _prefix /home/admin/rocketmq-cpp

# uncomment below, if your building depend on other packages.

BuildRequires: metaq-cmake = 3.18.3
BuildRequires: gcc49 = 4.9.2

# uncomment below, if depend on other packages

#Requires: package_name = 1.0.0

%description
#descript your rpm package here
This SDK implements pop-based message consumption and acknowledgement. Aka, SDK leaves most of the complexities to
brokers to enjoy client simplicity and robustness.

#%%debug_package
# support debuginfo package, to reduce runtime package size

# prepare your files
%install
# OLDPWD is the dir of rpm_create running
# _prefix is an inner var of rpmbuild,
# can set by rpm_create, default is "/home/a"
# _lib is an inner var, maybe "lib" or "lib64" depend on OS

export CC=/usr/local/gcc-4.9.2/bin/gcc
export CXX=/usr/local/gcc-4.9.2/bin/g++
export http_proxy=http://30.57.177.111:3128
export https_proxy=http://30.57.177.111:3128
export ALL_PROXY=30.57.177.111:3128
export LD_LIBRARY_PATH=/usr/local/gcc-4.9.2/lib64:${LD_LIBRARY_PATH}
# Build
cd $OLDPWD/../; /usr/local/bin/cmake -DCMAKE_INSTALL_PREFIX=${RPM_BUILD_ROOT}/%{_prefix} .; make -j $(nproc); make install

# create a crontab of the package
#echo "
#* * * * * root /home/a/bin/every_min
#3 * * * * ads /home/a/bin/every_hour
#" > %{_crontab}

# package infomation
%files
# set file attribute here
%defattr(-,root,root)
# need not list every file here, keep it as this
%{_prefix}
## create an empy dir

# %dir %{_prefix}/var/log

## need bakup old config file, so indicate here

# %config %{_prefix}/etc/sample.conf

## or need keep old config file, so indicate with "noreplace"

# %config(noreplace) %{_prefix}/etc/sample.conf

## indicate the dir for crontab

#%attr(644,root,root) %{_crondir}/*

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%changelog
* Mon Aug 10 2020 Release version 1.0.0
- add spec of %name
