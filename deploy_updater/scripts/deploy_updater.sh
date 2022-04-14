#!/bin/bash

# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/bin
FACEBOOC_ROOT=$(dirname "$SCRIPT")/../../
cd $FACEBOOC_ROOT

docker pull zxc25077667/facebooc:latest
docker-compose down
docker-compose up -d
