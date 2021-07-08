### 前言
C++是一门复杂的开发语言. 我们需要遵从一些约定, 以降低使用它的复杂度, 提升工程的可读性. 目前业界最广为接受的约定无疑是Google的
[C++ Style Guide](https://google.github.io/styleguide/cppguide.html)了, 本项目尽量遵从这个约定.

尽量, 主要是出于考虑商业产品API的一致性. 我们需要和现有[SDK API](http://gitlab.alibaba-inc.com/middleware/rocketmq-client4cpp)保持一致.
这样, 我们就有一些妥协的结果. 以异常为例, 在实现内部, 我们不适用异常. 但在DefaultMQProducer这个层面, 为了保持已有API兼容, 我们需
要将返回值转义为异常.

### 编译工具
项目使用CMake和Bazel两种管理工具.


### Define the following environment variables
export no_grpc_proxy=mq.docker.dev
export GRPC_TRACE=all
export GRPC_VERBOSITY=DEBUG
export http_proxy=http://30.57.177.111:3128
export https_proxy=http://30.57.177.111:3128