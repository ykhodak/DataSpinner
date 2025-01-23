#!/usr/bin/bash

source ./ws.env

if [[ $(uname) == 'Linux' ]]; then
    ../build/src/tcp_publisher spinner.properties
else
    ../build/src/$SPINNER_BUILD/tcp_publisher.exe spinner.properties      
fi
