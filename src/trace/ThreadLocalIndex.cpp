#include "ThreadLocalIndex.h"
#include "Logging.h"

#include <cmath>
#include <ctime>
namespace rocketmq {

	thread_local long ThreadLocalIndex::threadLocalIndex = 0;
	thread_local std::default_random_engine ThreadLocalIndex::dre;
/*
	ThreadLocalIndex::ThreadLocalIndex() {
		
		//threadLocalIndex=0;
	}*/

	long ThreadLocalIndex::getAndIncrement() {
		long index = threadLocalIndex;
		if (0 == index) {
			dre.seed(time(0));
			long rand = dre();
			index = std::abs(rand);
		}
        if (index < 0) {
          index = 0;
        }
        threadLocalIndex=index;

        index = std::abs(index + 1);
        if (index < 0)
            index = 0;

        threadLocalIndex=index;
        return index;
    }


    std::string ThreadLocalIndex::toString() {
        return "ThreadLocalIndex{" //+
            //"threadLocalIndex=" + threadLocalIndex +
            //'}'
			;
    }
	


}