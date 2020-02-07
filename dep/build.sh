BASEDIR=$(dirname "$0")
if [[ ! -d ${BASEDIR}/rocketmq_amd64/usr/local/include/ ]]; then
    mkdir -p ${BASEDIR}/rocketmq_amd64/usr/local/include/
fi

if [[ ! -d ${BASEDIR}/rocketmq_amd64/usr/local/lib ]]; then
    mkdir -p ${BASEDIR}/rocketmq_amd64/usr/local/lib
fi

cp -R ${BASEDIR}/../include/*              ${BASEDIR}/rocketmq_amd64/usr/local/include/
cp ${BASEDIR}/../bin/librocketmq.so      ${BASEDIR}/rocketmq_amd64/usr/local/lib/
cp ${BASEDIR}/../bin/librocketmq.a      ${BASEDIR}/rocketmq_amd64/usr/local/lib/

dpkg-deb --build ${BASEDIR}/rocketmq_amd64
