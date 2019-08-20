:/*
: * Licensed to the Apache Software Foundation (ASF) under one or more
: * contributor license agreements.  See the NOTICE file distributed with
: * this work for additional information regarding copyright ownership.
: * The ASF licenses this file to You under the Apache License, Version 2.0
: * (the "License"); you may not use this file except in compliance with
: * the License.  You may obtain a copy of the License at
: *
: *     http://www.apache.org/licenses/LICENSE-2.0
: *
: * Unless required by applicable law or agreed to in writing, software
: * distributed under the License is distributed on an "AS IS" BASIS,
: * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
: * See the License for the specific language governing permissions and
: * limitations under the License.
: */

if "%1" == "build" (
	call:build
) 
if "%1" == "build64" (
	call:build64
) else (
	call:download
	call:build64
)
goto:eof

:download --download dependency lib
@echo download start
rmdir thirdparty /S /Q
mkdir thirdparty
cd thirdparty
git clone https://github.com/jsj020122/jsoncpp-0.10.6.git
git clone https://github.com/jsj020122/boost_1_58_0.git
git clone https://github.com/jsj020122/libevent-release-2.0.22.git
git clone https://github.com/jsj020122/zlib-1.2.3-src.git
cd ..
@echo download end
goto:eof

:build --build all project
@echo build start
cd thirdparty/boost_1_58_0
@if "%programfiles%"=="" ("set programfiles=E:\Program Files (x86)")
call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\Common7\Tools\vsvars32.bat"
:: amd64

::call ""E:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"" amd64

set  ZLIB_SOURCE="%cd%\zlib-1.2.3-src\src\zlib\1.2.3\zlib-1.2.3\"
::cd boost_1_58_0
call bootstrap.bat
@echo build start.....
bjam.exe address-model=32 --with-serialization --with-atomic --with-log --with-locale --with-iostreams --with-system --with-regex --with-thread --with-date_time --with-chrono --with-filesystem  link=static  threading=multi variant=release runtime-link=shared
cd ../jsoncpp-0.10.6
devenv ./jsoncpp_lib_static.vcxproj  /Rebuild "Debug|x86" /out log.txt
cd ../libevent-release-2.0.22
devenv ./libevent.vcxproj  /Rebuild "Debug|x86" /out log.txt
cd ../../Win32
::cd ./Win32
::devenv ./rocketmq-client-cpp.sln  /Rebuild "Release|x64" /out log.txt
::devenv ./rocketmq-client-cpp.vcxproj /Build "Release|x64" /out log.txt
devenv ./rocketmq-client-cpp.sln /Build "Debug|x86" /out log.txt
::devenv ./rocketmq-client-cpp.vcxproj /Rebuild "Release|x64" /out log.txt
::cd ..
@echo build end
goto:eof



:build64 --build all project
@echo build start
cd thirdparty/boost_1_58_0
@if "%programfiles%"=="" ("set programfiles=E:\Program Files (x86)")
call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\Common7\Tools\vsvars32.bat"
:: amd64

::call ""E:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"" amd64

set  ZLIB_SOURCE="%cd%\zlib-1.2.3-src\src\zlib\1.2.3\zlib-1.2.3\"
set  ZLIB_INCLUDE="%cd%\zlib-1.2.3-src\src\zlib\1.2.3\zlib-1.2.3\"


::cd boost_1_58_0
call bootstrap.bat
@echo build start.....
.\b2 -j8 --with-serialization --with-atomic --with-log --with-locale --with-iostreams --with-system --with-regex --with-thread --with-date_time --with-chrono --with-filesystem --build-type=complete address-model=64 
cd ../jsoncpp-0.10.6
devenv ./jsoncpp_lib_static.vcxproj  /Build "Release|x64" /out log.txt
cd ../libevent-release-2.0.22
devenv ./libevent.vcxproj  /Build "Release|x64" /out log.txt
cd ../../Win32
::cd ./Win32
::devenv ./rocketmq-client-cpp.sln  /Rebuild "Release|x64" /out log.txt
::devenv ./rocketmq-client-cpp.vcxproj /Build "Release|x64" /out log.txt
devenv ./rocketmq-client-cpp.sln /Build "Release|x64" /out log.txt
::devenv ./rocketmq-client-cpp.vcxproj /Rebuild "Release|x64" /out log.txt
::cd ..
@echo build end
goto:eof
