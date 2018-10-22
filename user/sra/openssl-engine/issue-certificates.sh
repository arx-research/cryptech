#!/bin/sh -

. ./environment.sh

set -x

openssl req -batch -new -engine pkcs11 -keyform ENGINE -x509 -days 365		\
	-subj "/C=PV/O=Pottsylvanian Ministry of Offense/GN=Fearless/SN=Leader" \
	-key label_leader -out leader.cer

openssl req -batch -new -engine pkcs11 -keyform ENGINE				\
	-subj "/GN=Natasha/SN=Fatale"						\
	-key label_natasha							|
openssl x509 -req -engine pkcs11 -CAkeyform ENGINE -days 60			\
	-set_serial `date +%s` -extfile $OPENSSL_CONF -extensions ext_ee	\
	-CAkey label_leader -CA leader.cer					\
	-out natasha.cer

openssl req -batch -new -engine pkcs11 -keyform ENGINE				\
	-subj "/GN=Boris/SN=Badenov"						\
	-key label_boris							|
openssl x509 -req -engine pkcs11 -CAkeyform ENGINE -days 60			\
	-set_serial `date +%s` -extfile $OPENSSL_CONF -extensions ext_ee	\
	-CAkey label_leader -CA leader.cer					\
	-out boris.cer

openssl req -batch -new -engine pkcs11 -keyform ENGINE				\
	-subj "/GN=Hilary/SN=Pushemoff/CN=localhost"				\
	-key label_boris							|
openssl x509 -req -engine pkcs11 -CAkeyform ENGINE -days 60			\
	-set_serial `date +%s` -extfile $OPENSSL_CONF -extensions ext_ee	\
	-CAkey label_leader -CA leader.cer					\
	-out nogoodnik.cer

openssl verify -verbose -CAfile leader.cer boris.cer natasha.cer nogoodnik.cer
