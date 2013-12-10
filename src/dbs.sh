#!/bin/bash
cd kern/conf/
./config ASST0
cd ../compile/ASST0
bmake depend
bmake WERROR=
bmake install
cd ../../../
# bmake
cd ../root
sys161 -w kernel
