#!/bin/bash
cd kern/conf/
./config ASST1
cd ../compile/ASST1
bmake -m ~/os161temp/bmake/mk/ depend
bmake -m ~/os161temp/bmake/mk/ WERROR=
bmake -m ~/os161temp/bmake/mk/ install
cd ../../../
bmake
cd ../root
sys161 kernel
