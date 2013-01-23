#!/bin/bash
LIST=`ls`
for DIR in $LIST
do
    if [ -d $DIR ]; then
       cd $DIR
       if [ -e "rebuild.sh" ]; then
           ./rebuild.sh
       fi
       cd ..
    fi
done 
