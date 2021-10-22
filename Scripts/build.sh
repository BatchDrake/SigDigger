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

function make_startup_script() {
    # Create startup script
    echo '#!/bin/sh
    SELF=$(readlink -f "$0")
    HERE=${SELF%/*}
    export SUSCAN_CONFIG_PATH="${HERE}/share/suscan/config"
    export LD_LIBRARY_PATH="${HERE}/lib:$LD_LIBRARY_PATH"
    exec "${HERE}"/bin/'"$1"' "$@"' > "$DEPLOYROOT"/"$1"
    return $?
}
try "Creating startup script for SigDigger" make_startup_script SigDigger
try "Creating startup script for suscli" make_startup_script suscli

try "Moving files out of /usr..." mv "$DEPLOYROOT"/usr/* "$DEPLOYROOT"
try "Remove empty /usr..." rmdir "$DEPLOYROOT"/usr
try "Setting permissions to wrapper scripts..." chmod a+x "$DEPLOYROOT"/SigDigger "$DEPLOYROOT"/suscli
echo
echo "Done. SigDigger compiled successfully in $DEPLOYROOT"

