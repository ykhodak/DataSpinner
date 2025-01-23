#!/usr/bin/bash

source ./ws.env

if [[ $(uname) == 'Linux' ]]; then
    ../build/src/dds/fastdds_pubsub subscriber spinner.properties a
else
    ../build/src/dds/$SPINNER_BUILD/fastdds_pubsub.exe subscriber spinner.properties a
fi
