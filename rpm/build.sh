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
