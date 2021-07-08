#include "absl/synchronization/mutex.h"
#include "rocketmq/AsyncCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

class AwaitPullCallback : public PullCallback {
public:
  explicit AwaitPullCallback(PullResult& pull_result)
      : pull_result_(pull_result), completed_(false), has_failure_(false) {}

  void onSuccess(const PullResult& pull_result) override;

  void onException(const MQException& e) override;

  bool await();

private:
  PullResult& pull_result_;
  absl::Mutex mtx_;
  absl::CondVar cv_;
  bool completed_;
  bool has_failure_;
};

ROCKETMQ_NAMESPACE_END