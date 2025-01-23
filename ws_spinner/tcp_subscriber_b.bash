#!/usr/bin/bash

source ./ws.env

if [[ $(uname) == 'Linux' ]]; then
    ../build/src/tcp_subscriber spinner.properties b
else
    ../build/src/$SPINNER_BUILD/tcp_subscriber.exe spinner.properties b
fi

