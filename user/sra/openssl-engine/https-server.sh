#!/bin/sh -

. ./environment.sh

stunnel -fd 0 <<EOF

engine		= dynamic
engineCtrl	= SO_PATH:${ENGINE_MODULE}
engineCtrl	= ID:pkcs11
engineCtrl	= LOAD
engineCtrl	= MODULE_PATH:${PKCS11_MODULE}
engineCtrl	= PIN:${PKCS11_PIN}
engineCtrl	= INIT

foreground	= yes
pid		=

[https]
accept		= :::4443
cert		= $(pwd)/nogoodnik.cer
engineId	= pkcs11
key		= label_boris
exec		= /usr/sbin/micro-httpd
execargs	= micro-httpd $(pwd)

EOF
