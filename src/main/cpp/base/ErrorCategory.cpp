#include "rocketmq/ErrorCategory.h"

ROCKETMQ_NAMESPACE_BEGIN

std::string ErrorCategory::message(int code) const {
  ErrorCode ec = static_cast<ErrorCode>(code);
  switch (ec) {
  case ErrorCode::Success:
    return "Success";

  case ErrorCode::IllegalState:
    return "Client state illegal. Forgot to call start()?";

  case ErrorCode::BadConfiguration:
    return "Bad configuration.";

  case ErrorCode::BadRequest:
    return "Message is ill-formed. Check validity of your topic, tag, "
           "etc";

  case ErrorCode::Unauthorized:
    return "Authentication failed. Possibly caused by invalid credentials.";

  case ErrorCode::Forbidden:
    return "Authenticated user does not have privilege to perform the "
           "requested action";

  case ErrorCode::NotFound:
    return "Topic not found, which should be created through console or "
           "administration API before hand.";

  case ErrorCode::RequestTimeout:
    return "Timeout when connecting, reading from or writing to brokers.";

  case ErrorCode::PayloadTooLarge:
    return "Message body is too large.";

  case ErrorCode::PreconditionRequired:
    return "State of dependent procedure is not right";

  case ErrorCode::TooManyRequest:
    return "Quota exchausted. The user has sent too many requests in a given "
           "amount of time.";

  case ErrorCode::UnavailableForLegalReasons:
    return "A server operator has received a legal demand to deny access to "
           "a resource or to a set of resources that "
           "includes the requested resource.";

  case ErrorCode::HeaderFieldsTooLarge:
    return "The server is unwilling to process the request because either an "
           "individual header field, or all the header fields collectively, "
           "are too large";

  case ErrorCode::InternalServerError:
    return "Server side interval error";

  case ErrorCode::NotImplemented:
    return "The server either does not recognize the request method, or it "
           "lacks the ability to fulfil the request.";

  case ErrorCode::BadGateway:
    return "The server was acting as a gateway or proxy and received an "
           "invalid response from the upstream server.";

  case ErrorCode::ServiceUnavailable:
    return "The server cannot handle the request (because it is overloaded "
           "or down for maintenance). Generally, this "
           "is a temporary state.";

  case ErrorCode::GatewayTimeout:
    return "The server was acting as a gateway or proxy and did not receive "
           "a timely response from the upstream "
           "server.";

  case ErrorCode::ProtocolVersionNotSupported:
    return "The server does not support the protocol version used in the "
           "request.";

  case ErrorCode::InsufficientStorage:
    return "The server is unable to store the representation needed to "
           "complete the request.";

  default:
    return "Not-Implemented";
  }
}

ROCKETMQ_NAMESPACE_END