#include "FilterExpression.h"

ROCKETMQ_NAMESPACE_BEGIN

bool FilterExpression::accept(const MQMessageExt& message) const {

  switch (type_) {
  case ExpressionType::TAG: {
    if (WILD_CARD_TAG == content_) {
      return true;
    } else {
      return message.getTags() == content_;
    }
  }

  case ExpressionType::SQL92: {
    // Server should have strictly filtered.
    return true;
  }
  }
  return true;
}

const char* FilterExpression::WILD_CARD_TAG = "*";

ROCKETMQ_NAMESPACE_END