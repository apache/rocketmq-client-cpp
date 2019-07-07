#include "ThreadLocalIndex.h"
#include "Logging.h"

#include <cmath>
#include <ctime>
namespace rocketmq {

	thread_local long ThreadLocalIndex::m_threadLocalIndex = 0;
	thread_local std::default_random_engine ThreadLocalIndex::m_dre;

	long ThreadLocalIndex::getAndIncrement() {
		long index = m_threadLocalIndex;
		if (0 == index) {
			m_dre.seed(time(0));
			long rand = m_dre();
			index = std::abs(rand);
		}
        if (index < 0) {
          index = 0;
        }
        m_threadLocalIndex=index;

        index = std::abs(index + 1);
        if (index < 0)
            index = 0;

        m_threadLocalIndex=index;
        return index;
    }


    std::string ThreadLocalIndex::toString() {
        return "ThreadLocalIndex{ threadLocalIndex=}" //+
            //"threadLocalIndex=" + threadLocalIndex +
            //'}'
			;
    }
	


}