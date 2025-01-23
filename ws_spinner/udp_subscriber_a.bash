#!/usr/bin/bash

source ./ws.env

if [[ $(uname) == 'Linux' ]]; then
    ../build/src/udp_subscriber spinner.properties a
else
    ../build/src/$SPINNER_BUILD/udp_subscriber.exe spinner.properties a
fi

