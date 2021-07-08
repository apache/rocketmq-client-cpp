### Introduction
MQ products in the market supports two styles of API to acknowledge success of message consumption.

1. Rich client SDK manages individually and syncs to broker in batch manner; 
2. On consumption of each message, thin client SDK acknowledge it to broker instantly.

Both of which have famous adopters, Kafka, for example prefers 1 over 2 to achieve best performance and accept SDK 
complexities. SQS/IBM MQ, etc choose choice 2. RocketMQ support both of them.

### Pop SDK
This SDK implements choice of message consumption and acknowledgement. Aka, SDK leaves most of the complexities to 
brokers to enjoy client simplicity and robustness.

### API
To make the transition as smooth as possible, API compatibility is maintained. Namespace for now is changed to rocketmq
from metaq during proof of concept phase.

### How to build
We use [Modern CMake](https://cliutils.gitlab.io/modern-cmake/) to manage and compile this project, therefore, fairly 
easy to compile.
1. Clone this repo `git clone git@gitlab.alibaba-inc.com:shutian.lzh/rocketmq-cpp.git`
2. Build `cd rocketmq-cpp; mkdir _build; cd _build && cmake ..; make`
3. Enjoy

[Google Bazel](https://bazel.build/) is another build tool we supported, you can use `bazel build //...` to build the whole project.

### IDE
[Clangd](https://clangd.llvm.org/) is a really cool code completion tool. Use the following command to generate compile_command.json if you prefer to use it.
```
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
```

### Sanitizer
Clang/GCC has developed cool collection of sanitizer tools: Address, Undefined, Thread...

To run with sanitizer on, 
```
bazel run -c dbg --config=asan //src/test:asan_demo
```

### Example
Talk is cheap, show me the code!!!

Here you go:

1. [ExampleProducer.cpp](example/rocketmq/ExampleProducer.cpp)

1. [ExamplePushConsumer.cpp](example/rocketmq/ExamplePushConsumer.cpp)

### FAQ 
1. Q: Can this pop SDK deploy along with existing Pull SDK?
   
   A: We are currently working on this issue and this feature will be supported soon. Once the development is completed 
      and verified in production, we will get this item updated.
      
1. Q: Is this SDK production ready?
   
   A: To be honest, this SDK is just built and many features are still under active development. However, we take quality
      seriously and various measures are employed to achieve this goal, code review, unit test coverage, compiler
       sanitizers, thread-annotations-to-avoid-concurrent-issues, ...
       