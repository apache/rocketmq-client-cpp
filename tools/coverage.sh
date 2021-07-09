#!/usr/bin/env bash
TOOLS_DIR=$(dirname "$0")
WORKSPACE_DIR=$(dirname "$TOOLS_DIR")
cd $WORKSPACE_DIR
bazel coverage --config=remote_cache //src/test/cpp/ut/...
genhtml bazel-out/_coverage/_coverage_report.dat