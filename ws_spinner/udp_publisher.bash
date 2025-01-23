#!/usr/bin/bash

source ./ws.env

if [[ $(uname) == 'Linux' ]]; then
    ../build/src/udp_publisher spinner.properties
else
    ../build/src/$SPINNER_BUILD/udp_publisher.exe spinner.properties      
fi
