GTEST_DIR = ../gtest


USER_DIR = ../src


CPPFLAGS += -isystem $(GTEST_DIR)/include


CXXFLAGS += -g -Wextra -pthread -Wall -Wno-deprecated -fPIC -fno-strict-aliasing -std=c++11 -m64 -O0 -DNDEBUG

ROCKETMQ_HOME=../..
ROCKETMQ_BIN_INCLUDE=../../bin/include

INCLUDE= ${ROCKETMQ_BIN_INCLUDE}  ${ROCKETMQ_BIN_INCLUDE}/jsoncpp  ${ROCKETMQ_HOME}/include
WL_OBJ=-Wl,-rpath,../../bin/lib:../../bin ../../bin/librocketmq.so -ldl -lpthread -lrt -lz ../../libs/signature/lib/libSignature.a -ldl -lpthread -lrt -lz ../../bin/lib/libjsoncpp.a ../../bin/lib/libevent.a ../../bin/lib/libevent_core.a ../../bin/lib/libevent_extra.a ../../bin/lib/libevent_pthreads.a ../../bin/lib/libboost_atomic.a ../../bin/lib/libboost_thread.a ../../bin/lib/libboost_system.a ../../bin/lib/libboost_chrono.a ../../bin/lib/libboost_date_time.a ../../bin/lib/libboost_log.a ../../bin/lib/libboost_log_setup.a ../../bin/lib/libboost_regex.a ../../bin/lib/libboost_serialization.a ../../bin/lib/libboost_filesystem.a ../../bin/lib/libboost_locale.a ../../bin/lib/libboost_iostreams.a 
ROCKETMQ_OBJ=

TESTS = producer_test


GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

all: $(TESTS)

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o

GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

Producer.o : $(USER_DIR)/Producer.cc ${INCLUDE} $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I${ROCKETMQ_BIN_INCLUDE}  -I${ROCKETMQ_BIN_INCLUDE}/jsoncpp  -I${ROCKETMQ_HOME}/include -I$(GTEST_HEADERS) -c $(USER_DIR)/Producer.cc


producer_test : Producer.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS)   $^ -o $@ -L../../bin/lib $(WL_OBJ)
