#include "ONSSendCallback.h"

ONS_NAMESPACE_BEGIN

absl::Mutex ONSSendCallback::mutex_;

ONSSendCallback* ONSSendCallback::instance_ = nullptr;

ONS_NAMESPACE_END