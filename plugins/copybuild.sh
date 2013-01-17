#!/bin/bash
LS=`ls`
for DIR in $LS
do
    if [ -d $DIR ]; then
       if [ $DIR != "mvd_add" ]; then
           cp mvd_add/rebuild.sh $DIR
       fi
    fi
done
