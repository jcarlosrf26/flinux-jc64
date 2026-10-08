#!/bin/bash
# Normaliza modos dentro de los .tcz propios (raices 700/2770 heredadas del
# workspace dejaban /tmp/tcloop/<pkg> ilegible para tc: find de iconos y
# awk de .dep fallaban en silencio). Dirs 755, ficheros 644, ejecutables 755.
set -e
TR=$HOME/workspace/tools-root/usr/bin
PKG=$HOME/workspace/flinux-jc-64/v2/build/pkg

norm() { # $1 = tcz
  local p=$1 name
  name=$(basename "$p" .tcz)
  rm -rf /tmp/norm-$name
  $TR/unsquashfs -f -d /tmp/norm-$name "$p" >/dev/null 2>&1 || true
  find /tmp/norm-$name -type d -exec chmod 755 {} +
  find /tmp/norm-$name -type f -perm /u=x -exec chmod 755 {} +
  find /tmp/norm-$name -type f ! -perm /u=x -exec chmod 644 {} +
  $TR/mksquashfs /tmp/norm-$name /tmp/$name-new.tcz -noappend -no-xattrs >/dev/null
  rm -rf /tmp/norm-$name
  mv /tmp/$name-new.tcz "$p"
  md5sum "$p" | awk -v n=$(basename "$p") '{print $1"  "n}' > "$p.md5.txt"
  # verificacion: modo de la raiz dentro del squashfs nuevo
  rm -rf /tmp/chk-$name
  $TR/unsquashfs -f -d /tmp/chk-$name "$p" >/dev/null 2>&1 || true
  echo "$name: raiz=$(stat -c %a /tmp/chk-$name) md5=$(cut -d' ' -f1 $p.md5.txt)"
  rm -rf /tmp/chk-$name
}

for p in $PKG/*.tcz; do norm "$p"; done

# flconnect reutilizado: solo si su raiz interna es restrictiva
FC=$HOME/workspace/flinux-jc-64/src/flconnect64/flconnect.tcz
rm -rf /tmp/chk-flconnect
$TR/unsquashfs -f -d /tmp/chk-flconnect "$FC" >/dev/null 2>&1 || true
echo "flconnect(origen): raiz=$(stat -c %a /tmp/chk-flconnect)"
rm -rf /tmp/chk-flconnect
