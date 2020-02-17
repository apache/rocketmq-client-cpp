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
if [[ ! -d ${BASEDIR}/rocketmq_x64/CENTOS/ ]]; then
    echo "Can not find SPEC FILE"
    exit 1
fi
if [[ ! -d /root/rpmbuild/SOURCES/rocketmq/include ]]; then
    mkdir -p /root/rpmbuild/SOURCES/rocketmq
    mkdir -p /root/rpmbuild/SOURCES/rocketmq/include
    mkdir -p /root/rpmbuild/SOURCES/rocketmq/bin
fi
cp -R ${BASEDIR}/../include/*              /root/rpmbuild/SOURCES/rocketmq/include
cp ${BASEDIR}/../bin/librocketmq.so      /root/rpmbuild/SOURCES/rocketmq/bin
cp ${BASEDIR}/../bin/librocketmq.a      /root/rpmbuild/SOURCES/rocketmq/bin
cp ${BASEDIR}/rocketmq_x64/CENTOS/rocketmq-client-cpp.spec      /root/rpmbuild/SPECS

rpmbuild -bb /root/rpmbuild/SPECS/rocketmq-client-cpp.spec

cp /root/rpmbuild/RPMS/x86_64/*.rpm ${BASEDIR}/rocketmq_x64