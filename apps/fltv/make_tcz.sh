#!/bin/sh
set -e
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
BIN=$SCRIPT_DIR/bin/fltv
TCZ=$SCRIPT_DIR/tcz/fltv.tcz
STAGE=/tmp/fltv_tcz_stage

[ -f "$BIN" ] || { echo "ERROR: compilar primero (make native)"; exit 1; }
command -v mksquashfs >/dev/null 2>&1 || tce-load -i squashfs-tools.tcz

rm -rf "$STAGE"
mkdir -p "$STAGE/usr/local/bin"
cp "$BIN" "$STAGE/usr/local/bin/fltv"
strip "$STAGE/usr/local/bin/fltv"
cat > "$STAGE/usr/local/bin/fltv-open" << 'EOF'
#!/bin/sh
DISPLAY=:0 XAUTHORITY=/home/tc/.Xauthority fltv &
EOF
chmod +x "$STAGE/usr/local/bin/fltv-open"

mkdir -p "$STAGE/usr/local/share/pixmaps"
[ -f "$SCRIPT_DIR/assets/fltv48.png" ] && cp "$SCRIPT_DIR/assets/fltv48.png" "$STAGE/usr/local/share/pixmaps/fltv.png"

mkdir -p "$STAGE/usr/local/share/applications"
[ -f "$SCRIPT_DIR/fltv.desktop" ] && cp "$SCRIPT_DIR/fltv.desktop" "$STAGE/usr/local/share/applications/fltv.desktop"

mkdir -p "$SCRIPT_DIR/tcz"
mksquashfs "$STAGE" "$TCZ" -b 131072 -noappend -no-progress
(cd "$SCRIPT_DIR/tcz" && md5sum fltv.tcz > fltv.tcz.md5.txt)
rm -rf "$STAGE"

printf 'curl.tcz
json-c.tcz
mplayer-cli.tcz
' > "$SCRIPT_DIR/tcz/fltv.tcz.dep"

echo "TCZ: $TCZ"
ls -lh "$TCZ"
