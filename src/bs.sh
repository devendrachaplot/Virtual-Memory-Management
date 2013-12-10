#!/bin/bash
cd kern/conf/
./config ASST0
cd ../compile/ASST0
bmake depend
bmake WERROR=
bmake install
cd ../../../
#bmake WERROR=
cd ../root
sys161 kernel
