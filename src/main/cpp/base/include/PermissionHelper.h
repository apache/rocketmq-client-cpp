#pragma once

namespace rocketmq {
class PermissionHelper {
public:
  static bool canRead(unsigned int mode);

  static bool canWrite(unsigned int mode);

  static bool canInherit(unsigned int mode);

private:
  static const unsigned int READ_;
  static const unsigned int WRITE_;
  static const unsigned int INHERIT_;
};

} // namespace rocketmq