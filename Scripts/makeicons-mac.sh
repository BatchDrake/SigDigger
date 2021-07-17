#!/bin/bash

ICONPATH=../icons/SigDigger.png

mkdir SigDigger.iconset
sips -z 16 16     "$ICONPATH" --out SigDigger.iconset/icon_16x16.png
sips -z 32 32     "$ICONPATH" --out SigDigger.iconset/icon_16x16@2x.png
sips -z 32 32     "$ICONPATH" --out SigDigger.iconset/icon_32x32.png
sips -z 64 64     "$ICONPATH" --out SigDigger.iconset/icon_32x32@2x.png
sips -z 128 128   "$ICONPATH" --out SigDigger.iconset/icon_128x128.png
sips -z 256 256   "$ICONPATH" --out SigDigger.iconset/icon_128x128@2x.png
sips -z 256 256   "$ICONPATH" --out SigDigger.iconset/icon_256x256.png
sips -z 512 512   "$ICONPATH" --out SigDigger.iconset/icon_256x256@2x.png
sips -z 512 512   "$ICONPATH" --out SigDigger.iconset/icon_512x512.png
sips -z 1024 1024 "$ICONPATH" --out SigDigger.iconset/icon_512x512@2x.png

iconutil -c icns SigDigger.iconset
