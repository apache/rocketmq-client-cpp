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
BASEDIR=$(dirname "$0")
if [[ ! -d ${BASEDIR}/rocketmq_amd64/usr/local/include/rocketmq ]]; then
    mkdir -p ${BASEDIR}/rocketmq_amd64/usr/local/include/rocketmq
fi

if [[ ! -d ${BASEDIR}/rocketmq_amd64/usr/local/lib ]]; then
    mkdir -p ${BASEDIR}/rocketmq_amd64/usr/local/lib
fi

cp -R ${BASEDIR}/../include/*              ${BASEDIR}/rocketmq_amd64/usr/local/include/rocketmq
cp ${BASEDIR}/../bin/librocketmq.so      ${BASEDIR}/rocketmq_amd64/usr/local/lib/
cp ${BASEDIR}/../bin/librocketmq.a      ${BASEDIR}/rocketmq_amd64/usr/local/lib/

VERSION=`cat ${BASEDIR}/rocketmq_amd64/DEBIAN/control | grep Version | awk -F ':' '{print $2}'| sed 's/^ *//'`
dpkg-deb --build ${BASEDIR}/rocketmq_amd64 rocketmq-client-cpp-${VERSION}.amd64.deb
