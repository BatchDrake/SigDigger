#!/bin/bash
scl enable devtoolset-8 -- ./dist-appimage.sh
SIGDIGGER_EMBED_SOAPYSDR=1 scl enable devtoolset-8 -- ./dist-appimage.sh
