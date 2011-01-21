#!/bin/bash

DISPLAY=":1"
if [ "$1" != "" ]; then
DISPLAY="$1"
fi

make &&
cd data && sudo make install && cd .. &&
DISPLAY=$DISPLAY DBUS_SYSTEM_BUS_ADDRESS="tcp:host=neo,port=8000" src/mokomessages
