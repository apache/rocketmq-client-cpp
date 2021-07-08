#!/usr/bin/env bash
export no_grpc_proxy=mq.docker.dev,localhost
export GRPC_TRACE=all
export GRPC_VERBOSITY=INFO
export http_proxy=http://30.57.177.111:3128
export https_proxy=http://30.57.177.111:3128