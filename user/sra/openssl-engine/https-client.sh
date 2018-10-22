#!/bin/sh -

exec w3m \
     -o ssl_forbid_method=23 \
     -o ssl_verify_server=true \
     -o ssl_ca_file=$(pwd)/leader.cer \
     https://localhost:4443/
