#!/usr/bin/bash

source ./ws.env

if [[ $(uname) == 'Linux' ]]; then
    ../build/src/admin_client spinner.properties a
else
    ../build/src/$SPINNER_BUILD/admin_client.exe spinner.properties a
fi

