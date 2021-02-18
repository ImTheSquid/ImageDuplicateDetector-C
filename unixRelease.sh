#! /bin/bash

cmake -H. -B./unix -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -s"
cmake --build ./unix --config Release -- -j26
