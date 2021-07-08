load("@rules_cc//cc:defs.bzl", "cc_library")
UV_COMMON_SOURCES = [
  "include/uv/unix.h",
  "include/uv/linux.h",
  "include/uv/sunos.h",
  "include/uv/darwin.h",
  "include/uv/bsd.h",
  "include/uv/aix.h",
  "src/idna.h",
  "src/idna.c",
  "src/fs-poll.c",
  "src/heap-inl.h",
  "src/inet.c",
  "src/queue.h",
  "src/threadpool.c",
  "src/timer.c",
  "src/strscpy.h",
  "src/strscpy.c",
  "src/uv-common.c",
  "src/uv-common.h",
  "src/uv-data-getter-setters.c",
  "src/version.c"
]

UV_UNIX_SOURCES = [
  "src/unix/async.c",
  "src/unix/atomic-ops.h",
  "src/unix/core.c",
  "src/unix/dl.c",
  "src/unix/fs.c",
  "src/unix/getaddrinfo.c",
  "src/unix/getnameinfo.c",
  "src/unix/internal.h",
  "src/unix/loop.c",
  "src/unix/loop-watcher.c",
  "src/unix/pipe.c",
  "src/unix/poll.c",
  "src/unix/process.c",
  "src/unix/signal.c",
  "src/unix/spinlock.h",
  "src/unix/stream.c",
  "src/unix/tcp.c",
  "src/unix/thread.c",
  "src/unix/tty.c",
  "src/unix/udp.c",
]

UV_DARWIN_SOURCES = [
  "src/unix/bsd-ifaddrs.c",
  "src/unix/darwin.c",
  "src/unix/darwin-proctitle.c",
  "src/unix/fsevents.c",
  "src/unix/kqueue.c",
  "src/unix/proctitle.c",
]

UV_LINUX_SOURCES = [
    "src/unix/linux-core.c",
    "src/unix/linux-inotify.c",
    "src/unix/linux-syscalls.c",
    "src/unix/linux-syscalls.h",
    "src/unix/procfs-exepath.c",
    "src/unix/proctitle.c",
    "src/unix/sysinfo-loadavg.c",
]

cc_library(
  name = "libuv",
  srcs = UV_COMMON_SOURCES + select({
    "@bazel_tools//src/conditions:darwin": UV_UNIX_SOURCES + UV_DARWIN_SOURCES,
    # FIXME(ofrobots): Figure out the right way to detect linux.
    "//conditions:default": UV_UNIX_SOURCES + UV_LINUX_SOURCES,
  }),
  hdrs = [
    "src/unix/internal.h",
    "include/uv.h",
    "include/uv/darwin.h",
    "include/uv/errno.h",
    "include/uv/threadpool.h",
    "include/uv/tree.h",
    "include/uv/unix.h",
    "include/uv/version.h",
  ],
  includes = ["include", "src"],
  defines = [
    "_GNU_SOURCE",
  ],
  linkopts = [
    "-ldl",
  ],
  visibility = ["//visibility:public"],
)
