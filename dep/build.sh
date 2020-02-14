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
