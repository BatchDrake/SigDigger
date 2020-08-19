#!/bin/bash
#
#  build.sh: Build SigDigger locally
#
#  Copyright (C) 2020 Gonzalo José Carracedo Carballal
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program.  If not, see
#  <http://www.gnu.org/licenses/>
#
#

. dist-common.sh

build

try "Removing unneeded development files..." rm -Rfv "$DEPLOYROOT"/usr/include "$DEPLOYROOT"/usr/bin/suscan.status "$DEPLOYROOT"/usr/lib/pkgconfig

# Create startup script
echo '#!/bin/sh
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export SUSCAN_CONFIG_PATH="${HERE}/share/suscan/config"
export LD_LIBRARY_PATH="${HERE}/lib:$LD_LIBRARY_PATH"
exec "${HERE}"/bin/SigDigger "$@"' > "$DEPLOYROOT"/SigDigger

try "Moving files out of /usr..." mv "$DEPLOYROOT"/usr/* "$DEPLOYROOT"
try "Remove empty /usr..." rmdir "$DEPLOYROOT"/usr
try "Setting permissions to wrapper script..." chmod a+x "$DEPLOYROOT"/SigDigger
echo
echo "Done. SigDigger compiled succesfully in $DEPLOYROOT"

