#!/bin/bash
# 排除特定文件: 如果想跳过某些头文件（例如 debug.h），可以用 ! -name：

find include -type f -name "*.h" ! -path "include/math/*" ! -path "include/io/*" -exec cat {} + > include.tmp