#pragma once
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/ExpressionType.h"
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

/**
 * Server supported message filtering expression. At present, two types are supported: tag and SQL92.
 */
struct FilterExpression {
  explicit FilterExpression(std::string expression, ExpressionType expression_type = ExpressionType::TAG)
      : content_(std::move(expression)), type_(expression_type), version_(std::chrono::steady_clock::now()) {
    if (ExpressionType::TAG == type_ && content_.empty()) {
      content_ = WILD_CARD_TAG;
    }
  }

  bool accept(const MQMessageExt& message) const;

  std::string content_;
  ExpressionType type_;
  std::chrono::steady_clock::time_point version_;

  static const char* WILD_CARD_TAG;
};

ROCKETMQ_NAMESPACE_END