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
@echo off
if not exist "thirdparty/jsoncpp-0.10.6" (
	@echo thirdparty not found,git clone thirdparty first.
	call:download
)

set VS160COMNTOOLS=e:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\

if "%1" == "build" (
	call:build
) else (
	if "%1" == "build64" (
		call:build64
	) else (
		@echo no build type specified,default buit default target:build64
		call:build64
	)
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
:echo %date:~0,4%%date:~5,2%%date:~8,2%%time:~0,2%%time:~3,2%%time:~6,2%>thirdparty.stat
cd ..
@echo download thirdparty end
goto:eof

:build --build all project
@echo build x86 start
@echo build tool setup

if exist "%VS160COMNTOOLS%/vcvars32.bat" (
	@echo use build tool:vs2019
	call "%VS160COMNTOOLS%/vcvars32.bat"
) else (
if exist "%VS150COMNTOOLS%vsvars32.bat" (
 	@echo use build tool:vs2017
	call "%VS150COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
 ) else (
	if exist "%VS140COMNTOOLS%vsvars32.bat" (
		@echo use build tool:vs2015
		call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
	)	else (
		@echo supported toolchains not found!please install vs2015 or vs2017
		goto:eof
	)
)




if exist "%VS160COMNTOOLS%/vcvars32.bat" (
	@echo use build tool:vs2019
	call "%VS160COMNTOOLS%/vcvars32.bat"
) else (
	if exist "%VS150COMNTOOLS%vsvars32.bat" (
		@echo use build tool:vs2017
		call "%VS150COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
	) else (
		if exist "%VS140COMNTOOLS%vsvars32.bat" (
			@echo use build tool:vs2015
			call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
		) else (
			@echo supported toolchains not found!please install vs2015 or vs2017
			goto:eof
		)
	)
)



@echo build thirdparty
set  ZLIB_SOURCE="%cd%\thirdparty\zlib-1.2.3-src\src\zlib\1.2.3\zlib-1.2.3\"
set  ZLIB_INCLUDE="%cd%\thirdparty\zlib-1.2.3-src\src\zlib\1.2.3\zlib-1.2.3\"
cd thirdparty/boost_1_58_0
call bootstrap.bat
@echo build start.....
b2.exe  --build-type=complete -sZLIB_BINARY=zlib -sZLIB_SOURCE=%ZLIB_SOURCE% -sZLIB_INCLUDE=%ZLIB_INCLUDE% address-model=32 --with-zlib --with-serialization --with-atomic --with-log --with-locale --with-iostreams --with-system --with-regex --with-thread --with-date_time --with-chrono --with-filesystem  link=static  threading=multi variant=release runtime-link=static

cd ../jsoncpp-0.10.6
devenv ./jsoncpp_lib_static.vcxproj  /Rebuild "Release|x86" /out log.txt
cd ../libevent-release-2.0.22
devenv ./libevent.vcxproj  /Rebuild "Release|x86" /out log.txt
cd ../../Win32
::cd ./Win32
::devenv ./rocketmq-client-cpp.sln  /Rebuild "Release|x64" /out log.txt
::devenv ./rocketmq-client-cpp.vcxproj /Build "Release|x64" /out log.txt
@echo build rocketmq-client-cpp
devenv ./rocketmq-client-cpp.sln /Build "Release|x86" /out log.txt
::devenv ./rocketmq-client-cpp.vcxproj /Rebuild "Release|x64" /out log.txt
::cd ..
@echo build end
goto:eof



:build64 --build all project
@echo build x86_64 start
@echo build tool setup

if exist "%VS160COMNTOOLS%/vcvarsx86_amd64.bat" (
	@echo use build tool:vs2019
	call "%VS160COMNTOOLS%/vcvarsx86_amd64.bat"
) else (
	if exist "%VS150COMNTOOLS%vsvars32.bat" (
		@echo use build tool:vs2017
		call "%VS150COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
	) else (
		if exist "%VS140COMNTOOLS%vsvars32.bat" (
			@echo use build tool:vs2015
			call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
		) else (
			@echo supported toolchains not found!please install vs2015 or vs2017
			goto:eof
		)
	)
)

@echo build thirdparty
set ZLIB_SOURCE=%cd%\thirdparty\zlib-1.2.3-src\src\zlib\1.2.3\zlib-1.2.3\
set ZLIB_INCLUDE=%cd%\thirdparty\zlib-1.2.3-src\src\zlib\1.2.3\zlib-1.2.3\
set ZLIB_LIBPATH=%cd%\thirdparty\zlib-1.2.3-src\src\zlib\1.2.3\zlib-1.2.3\

cd thirdparty/boost_1_58_0
call bootstrap.bat
@echo build start.....
b2.exe  --build-type=complete -sZLIB_SOURCE=%ZLIB_SOURCE% -sZLIB_INCLUDE=%ZLIB_INCLUDE% address-model=64 --with-serialization --with-atomic --with-log --with-locale --with-iostreams --with-system --with-regex --with-thread --with-date_time --with-chrono --with-filesystem  link=static  threading=multi variant=debug runtime-link=static
cd ../jsoncpp-0.10.6
devenv ./jsoncpp_lib_static.vcxproj  /Build "Release|x64" /out log.txt
cd ../libevent-release-2.0.22
devenv ./libevent.vcxproj  /Build "Release|x64" /out log.txt
cd ../../Win32
::cd ./Win32
::devenv ./rocketmq-client-cpp.sln  /Rebuild "Release|x64" /out log.txt
::devenv ./rocketmq-client-cpp.vcxproj /Build "Release|x64" /out log.txt
@echo build rocketmq-client-cpp
devenv ./rocketmq-client-cpp.sln /Build "Release|x64" /out log.txt
::devenv ./rocketmq-client-cpp.vcxproj /Rebuild "Release|x64" /out log.txt
::cd ..
@echo build end
goto:eofho build end
goto:eof