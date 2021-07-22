#!/bin/sh
if [ ! -d "build/" ];then
  mkdir build
fi
g++ -Wall -W -Os -Ilinenoise -Iinclude src/main.cpp src/debugger.cpp src/breakpoint.cpp src/ptrace_wrapper.cpp lib/third_party/liblinenoise.a -o build/fdb
g++ -g demos/demo.cpp -o build/demo
