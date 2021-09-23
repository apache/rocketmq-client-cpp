#include "absl/synchronization/mutex.h"

#include "rocketmq/AsyncCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

class AwaitPullCallback : public PullCallback {
public:
  explicit AwaitPullCallback(PullResult& pull_result) : pull_result_(pull_result) {}

  void onSuccess(const PullResult& pull_result) override;

  void onException(const MQException& e) override;

  bool await();

  bool hasFailure() const { return has_failure_; }

  bool isCompleted() const { return completed_; }

private:
  PullResult& pull_result_;
  absl::Mutex mtx_;
  absl::CondVar cv_;
  bool completed_{false};
  bool has_failure_{false};
};

ROCKETMQ_NAMESPACE_END