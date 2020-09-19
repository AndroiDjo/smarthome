#!/bin/sh

### Custom user script
### Called after internal WAN up/down action
### $1 - WAN action (up/down)
### $2 - WAN interface name (e.g. eth3 or ppp0)
### $3 - WAN IPv4 address

# WAN action should be up
[ ! "x$1" == "xup" ] && exit 0

runWithDelay () {
sleep 10
logger -t $(basename $0) "started [$@]"

externalip=$(wget -qO- https://ipecho.net/plain ; echo)
if [ "$1" = "$externalip" ]
then
        logger -t $(basename $0) "ip is white $1"
else
        logger -t $(basename $0) "ip is grey. wan = $1 , external = $externalip"
        logger -t $(basename $0) "restarting wan..."
        restart_wan
fi
}

runWithDelay $3
