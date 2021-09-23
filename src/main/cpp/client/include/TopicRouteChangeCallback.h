#pragma once

#include "absl/strings/string_view.h"

#include "TopicRouteData.h"

ROCKETMQ_NAMESPACE_BEGIN

class TopicRouteChangeCallback {
public:
  virtual void onTopicRouteChange(absl::string_view topic, const TopicRouteDataPtr& topic_route_data) = 0;
};

ROCKETMQ_NAMESPACE_END