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
) else (
	call:download
	call:build
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
cd thirdparty
@if "%programfiles%"=="" ("set programfiles=c:\Program Files (x86)")
call "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\Common7\Tools\vsvars32.bat"
set  ZLIB_SOURCE="%cd%\zlib-1.2.3-src\src\zlib\1.2.3\zlib-1.2.3\"
cd boost_1_58_0
call bootstrap.bat
@echo build start.....
bjam.exe --with-serialization --with-atomic --with-log --with-locale --with-iostreams --with-system --with-regex --with-thread --with-date_time --with-chrono --with-filesystem  link=static  threading=multi variant=release runtime-link=shared
cd ../jsoncpp-0.10.6
devenv ./jsoncpp_lib_static.vcxproj  /Rebuild "Release|x86" /out log.txt
cd ../libevent-release-2.0.22
devenv ./libevent.vcxproj  /Rebuild "Release|x86" /out log.txt
cd ../../Win32
devenv ./rocketmq-client-cpp.sln  /Rebuild "Release|x86" /out log.txt
cd ..
@echo build end
goto:eof

