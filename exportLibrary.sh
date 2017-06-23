#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/workspace/tp-2017-1c-utn-panic/PANICommons/Debug
sudo ldconfig ~/workspace/tp-2017-1c-utn-panic/PANICommons/Debug
exec bash
