#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/usr/local/lib64
scl enable devtoolset-8 -- ./dist-appimage.sh
SIGDIGGER_EMBED_SOAPYSDR=1 scl enable devtoolset-8 -- ./dist-appimage.sh
