#!/bin/sh
gcc -Wall -D_FILE_OFFSET_BITS=64 jsonfsx.c $(pkg-config fuse json-c --cflags --libs) -o jsonfsx