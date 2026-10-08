#!/bin/sh
set -e
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
# flwriter_flinux: binario 32-bit ya fetcheado desde FLinux (mksquashfs no esta
# disponible alli, este script se corre en el PC). Si no existe, usa el binario
# nativo (caso de correr este script directamente dentro de FLinux).
if [ -f "$SCRIPT_DIR/flwriter_flinux" ]; then
	BIN=$SCRIPT_DIR/flwriter_flinux
else
	BIN=$SCRIPT_DIR/flwriter
fi
TCZ=$SCRIPT_DIR/tcz/flwriter.tcz
STAGE=/tmp/flwriter_tcz_stage

[ -f "$BIN" ] || { echo "ERROR: compilar primero (make, o cross-fetch flwriter_flinux)"; exit 1; }
command -v mksquashfs >/dev/null 2>&1 || tce-load -i squashfs-tools.tcz

rm -rf "$STAGE"
mkdir -p "$STAGE/usr/local/bin"
cp "$BIN" "$STAGE/usr/local/bin/flwriter"
strip "$STAGE/usr/local/bin/flwriter"

mkdir -p "$STAGE/usr/local/share/pixmaps"
[ -f "$SCRIPT_DIR/flwriter48.png" ] && cp "$SCRIPT_DIR/flwriter48.png" "$STAGE/usr/local/share/pixmaps/flwriter.png"

mkdir -p "$STAGE/usr/local/share/applications"
[ -f "$SCRIPT_DIR/flwriter.desktop" ] && cp "$SCRIPT_DIR/flwriter.desktop" "$STAGE/usr/local/share/applications/flwriter.desktop"

mkdir -p "$SCRIPT_DIR/tcz"
mksquashfs "$STAGE" "$TCZ" -b 131072 -noappend -no-progress
md5sum "$TCZ" | awk '{print $1}' > "$TCZ.md5.txt"
rm -rf "$STAGE"

printf 'libX11.tcz
libXext.tcz
libpng.tcz
libjpeg-turbo.tcz
' > "$SCRIPT_DIR/tcz/flwriter.tcz.dep"

echo "TCZ: $TCZ"
ls -lh "$TCZ"
