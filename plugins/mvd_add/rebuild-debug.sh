#!/bin/bash
gcc -DMVD_ADD_TEST -I../shared/include -I/usr/local/include -I. -I../../include -I../../mvd/include -g ../shared/src/*.c *.c -lmvd -licuuc -o mvd_add
