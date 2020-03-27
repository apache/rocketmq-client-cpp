#!/bin/sh

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


VERSION="2.2.0"
PKG_NAME="rocketmq-client-cpp"
CWD_DIR=$(cd "$(dirname "$0")"; pwd)
DEPLOY_BUILD_HOME=${CWD_DIR}/${PKG_NAME}

# ##====================================================================
#make
# ##====================================================================
# # deploy
rm -rf   ${DEPLOY_BUILD_HOME}
mkdir -p ${DEPLOY_BUILD_HOME}/lib
cp -rf ${CWD_DIR}/../bin/*.a    ${DEPLOY_BUILD_HOME}/lib/
if test "$(uname)" = "Linux"; then
cp -rf ${CWD_DIR}/../bin/*.so   ${DEPLOY_BUILD_HOME}/lib/
fi
if test "$(uname)" = "Darwin"; then
cp -rf ${CWD_DIR}/../bin/*.dylib   ${DEPLOY_BUILD_HOME}/lib/
fi
cp -rf ${CWD_DIR}/../include    ${DEPLOY_BUILD_HOME}/
cp -rf ${CWD_DIR}/../example    ${DEPLOY_BUILD_HOME}/
cp -rf ${CWD_DIR}/../README.md  ${DEPLOY_BUILD_HOME}/
cp -rf ${CWD_DIR}/../LICENSE  ${DEPLOY_BUILD_HOME}/LICENSE
cp -rf ${CWD_DIR}/../NOTICE ${DEPLOY_BUILD_HOME}/NOTICE

cd ${CWD_DIR} && tar -cvzf ./${PKG_NAME}-${VERSION}-bin-release.tar.gz ./${PKG_NAME}  &> /dev/null
rm -rf ${DEPLOY_BUILD_HOME}
# # ##====================================================================
#make clean
