#!/bin/bash
gcc -I../../mvd/include -I../../include -I../shared/include -DMVD_CREATE_TEST mvd_create.c ../shared/src/plugin_log.c -lmvd -o mvd_create
