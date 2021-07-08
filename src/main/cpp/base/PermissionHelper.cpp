#include "PermissionHelper.h"

using namespace rocketmq;

const unsigned int PermissionHelper::READ_ = 0x1u << 2u;

const unsigned int PermissionHelper::WRITE_ = 0x1u << 1u;

const unsigned int PermissionHelper::INHERIT_ = 0x1u << 0u;

bool PermissionHelper::canRead(unsigned int mode) { return (mode & READ_) == READ_; }

bool PermissionHelper::canWrite(unsigned int mode) { return (mode & WRITE_) == WRITE_; }

bool PermissionHelper::canInherit(unsigned int mode) { return (mode & INHERIT_) == INHERIT_; }