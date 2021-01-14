#!/bin/sh
#
# Copyright (c) 2021 Bernd Walter Computer Technology
# All rights reserved.
#
# PROVIDE: mqtt_client
# REQUIRE: DAEMON
# KEYWORD: FreeBSD
#
# Add the following line to /etc/rc.conf to enable mqtt_client:
#
# mqtt_client_enable="YES"
#

mqtt_client_enable=${mqtt_client_enable-"NO"}

. /etc/rc.subr

name=mqtt_client
rcvar=`set_rcvar`

command=/usr/local/sbin/${name}
pidfile=/var/run/${name}.pid
sig_stop=-KILL

load_rc_config ${name}
run_rc_command "$1"

