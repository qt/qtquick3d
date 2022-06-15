#!/bin/sh
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# printProperty(): prints a key-value pair from given key and cmd list.
# If running cmd fails, or does not produce any stdout, nothing is printed.
# Arguments: $1: key, $2: cmd, $3: optional, field specification as to cut(1) -f
printProperty ()
{
    key=$1
    val=`{ eval $2 ; } 2>/dev/null`
    [ -n "$3" ] && val=`echo $val | tr -s '[:blank:]' '\t' | cut -f$3`
    [ -n "$val" ] && echo $key: $val
}

# printEnvVar(): prints a key-value pair from given environment variable name.
# key is printed as "Env_<varname>".
# If the variable is undefined, nothing is printed.
# Arguments: $1: varname

printEnvVar ()
{
    key=Env_$1
    val=`eval 'echo $'$1`
    [ -n "$val" ] && echo $key: $val
}


# printOnOff(): prints a key-value pair from given environment variable name.
# If variable is defined, value is printed as "<key>-On"; otherwise "<key>-Off".
# Arguments: $1: key $2: varname

printOnOff ()
{
    key=$1
    val=`eval 'echo $'$2`
    if [ -z "$val" ] ; then
        val=Off
    else
        val=On
    fi
    echo $key: $key-$val
}

# ------------

printProperty Uname              "uname -a"

printProperty WlanMAC            "ifconfig wlan0 | grep HWaddr" 5

printEnvVar QMLSCENE_DEVICE
