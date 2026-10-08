#!/bin/bash
# Empaqueta las apps FL nativas x86_64 para flinux-jc64 v2.
# Patrón: layout del .tcz original de 32 bits (FLinux) + binarios 64 bits
# recién compilados; intérprete fijado a /lib/ld-linux-x86-64.so.2
# (CorePure64 no tiene /lib64). mksquashfs -no-xattrs como la familia.
set -e
W=$HOME/workspace/flinux-jc-64
ORIG=$HOME/workspace/flinux-jc/isoroot-v15fix/cde/optional
OUT=$W/v2/build/pkg
SRC=$W/v2/src
mkdir -p $OUT
cd $OUT

fixinterp() { patchelf --set-interpreter /lib/ld-linux-x86-64.so.2 "$1"; }

stage_from_orig() { # $1=pkg
  rm -rf st-$1
  unsquashfs -f -d st-$1 $ORIG/$1.tcz >/dev/null 2>/dev/null || true
  rm -rf st-$1/usr/local/lib32
}

mktcz() { # $1=pkg $2..=lineas .dep
  local pkg=$1; shift
  rm -f $pkg.tcz $pkg.tcz.dep $pkg.tcz.md5.txt
  mksquashfs st-$pkg $pkg.tcz -noappend -no-xattrs >/dev/null
  printf '%s\n' "$@" > $pkg.tcz.dep
  md5sum $pkg.tcz | awk '{print $1}' > $pkg.tcz.md5.txt
  echo "$pkg.tcz: $(stat -c%s $pkg.tcz) bytes md5 $(cat $pkg.tcz.md5.txt) dep=[$*]"
}

# --- FLFM (4 binarios) ---
stage_from_orig flfm
for b in flfm flfm-apps-hotkey flfm-hotkey flfm-volwatch; do
  cp $SRC/flfm2026/bin/$b st-flfm/usr/local/bin/$b; chmod 755 st-flfm/usr/local/bin/$b
  fixinterp st-flfm/usr/local/bin/$b
done
mktcz flfm fltk-1.3.tcz

# --- FLWriter ---
stage_from_orig flwriter
cp $SRC/flwrite/flwriter st-flwriter/usr/local/bin/flwriter
chmod 755 st-flwriter/usr/local/bin/flwriter
fixinterp st-flwriter/usr/local/bin/flwriter
mktcz flwriter fltk-1.3.tcz

# --- FLRadio (ALSA + mpg123 en runtime) ---
stage_from_orig flradio
cp /tmp/flradio64 st-flradio/usr/local/bin/flradio
chmod 755 st-flradio/usr/local/bin/flradio
fixinterp st-flradio/usr/local/bin/flradio
mktcz flradio fltk-1.3.tcz alsa.tcz mpg123.tcz

# --- FLTV (lista de canales: la de la v1.5 va en el skel, no en el paquete) ---
stage_from_orig fltv
cp $SRC/fltv/bin/fltv st-fltv/usr/local/bin/fltv
chmod 755 st-fltv/usr/local/bin/fltv
fixinterp st-fltv/usr/local/bin/fltv
mktcz fltv fltk-1.3.tcz curl.tcz json-c.tcz

# --- FLTube (layout del paquete 64 bits del pool v1.0; binario nuevo) ---
rm -rf st-fltube
unsquashfs -f -d st-fltube $W/dl/pool64/fltube.tcz >/dev/null 2>/dev/null || true
cp $SRC/fltube/build/fltube st-fltube/usr/local/bin/fltube
chmod 755 st-fltube/usr/local/bin/fltube
strip st-fltube/usr/local/bin/fltube
fixinterp st-fltube/usr/local/bin/fltube
mktcz fltube fltk-1.3.tcz curl.tcz python3.14.tcz

echo "--- ELF check ---"
for p in flfm flwriter flradio fltv fltube; do
  b=$(find st-$p/usr/local/bin -type f | head -1)
  echo "$p: $(file -b $b | cut -c1-70)"
done
