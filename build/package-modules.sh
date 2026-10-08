#!/bin/bash
# Paquetes de modulos renombrados 6.18.35-flinux-jc64 para flinux-jc64 v2.
# Toma el arbol completo instalado (modstage) y lo divide igual que el
# repositorio oficial: el subconjunto "core" (los 915 .ko.gz que trae el
# corepure64.gz de fabrica) va al core; las 4 extensiones stock
# (alsa/graphics/wireless/ipv6-netfilter) se reempaquetan con el arbol
# renombrado. Sin esto, los .ko stock (vermagic 6.18.35-tinycore64) no
# cargarian en el kernel 6.18.35-flinux-jc64.
set -e
W=$HOME/workspace/flinux-jc-64
V=6.18.35-flinux-jc64
FULL=$W/v2/kernel/modstage/lib/modules/$V
OUT=$W/v2/build/pkg
mkdir -p $OUT

# 1) gzip de todos los .ko y depmod del arbol completo (referencia)
find $FULL -name "*.ko" | while read f; do gzip -n "$f"; done
depmod -b $W/v2/kernel/modstage $V 2>/dev/null || true
find $FULL -name "*.ko.gz" | sed "s|$FULL/||" | sort > $W/v2/build/my-modules-all.txt
wc -l $W/v2/build/my-modules-all.txt

# 2) subconjunto core: mismos paths relativos que los 915 del core oficial
rm -rf $W/v2/kernel/coremods
mkdir -p $W/v2/kernel/coremods/lib/modules/$V
( cd $FULL && while read rel; do
    src=${rel%.ko.gz}.ko.gz
    [ -f "$src" ] && cp --parents "$src" $W/v2/kernel/coremods/lib/modules/$V/
  done < $W/v2/build/stock-core-modules.txt )
# depmod del subconjunto (solo cobertura core)
depmod -b $W/v2/kernel/coremods $V 2>/dev/null || true
n=$(find $W/v2/kernel/coremods -name "*.ko.gz" | wc -l)
echo "core subset: $n modulos (stock: $(wc -l < $W/v2/build/stock-core-modules.txt))"

# 3) extensiones desde el arbol completo, con los ficheros del paquete stock
mkext() { # $1=nombre-ext (sin version) $2=paquete stock
  local name=$1 stock=$2
  rm -rf /tmp/ext-$name
  unsquashfs -f -d /tmp/ext-$name $W/dl/pool64/$stock >/dev/null 2>&1 || true
  # lista de .ko.gz del stock (rutas relativas a lib/modules del ext)
  ( cd /tmp/ext-$name && find . -name "*.ko.gz" | sed 's|^\./||' ) > /tmp/ext-$name.list
  # limpiar modulos y metadata del stock; conservar firmware y resto
  ( cd /tmp/ext-$name && find . -name "*.ko.gz" -delete && rm -f usr/local/lib/modules/*/modules.* )
  local miss=0
  while read rel; do
    local relnew=${rel/tinycore64/flinux-jc64}
    if [ -f "$FULL/${rel#usr/local/lib/modules/$V/}" ] || [ -f "$FULL/$(basename $rel)" ]; then :; fi
    # el .ko en mi arbol: misma ruta relativa bajo kernel/ o drivers/...
    local sub=${rel#usr/local/lib/modules/6.18.35-tinycore64/}
    if [ -f "$FULL/$sub" ]; then
      mkdir -p "/tmp/ext-$name/usr/local/lib/modules/$V/$(dirname $sub)"
      cp "$FULL/$sub" "/tmp/ext-$name/usr/local/lib/modules/$V/$sub"
    else
      echo "  FALTA en mi build: $sub"; miss=$((miss+1))
    fi
  done < /tmp/ext-$name.list
  rm -f $OUT/$name-$V.tcz $OUT/$name-$V.tcz.dep $OUT/$name-$V.tcz.md5.txt
  mksquashfs /tmp/ext-$name $OUT/$name-$V.tcz -noappend -no-xattrs >/dev/null
  [ -f $W/dl/pool64/$stock.dep ] && cp $W/dl/pool64/$stock.dep $OUT/$name-$V.tcz.dep || : > $OUT/$name-$V.tcz.dep
  md5sum $OUT/$name-$V.tcz > $OUT/$name-$V.tcz.md5.txt
  echo "$name-$V.tcz: $(stat -c%s $OUT/$name-$V.tcz) bytes, faltantes=$miss, md5 $(cut -d' ' -f1 $OUT/$name-$V.tcz.md5.txt)"
}

mkext alsa-modules alsa-modules-6.18.35-tinycore64.tcz
mkext graphics graphics-6.18.35-tinycore64.tcz
mkext wireless wireless-6.18.35-tinycore64.tcz
mkext ipv6-netfilter ipv6-netfilter-6.18.35-tinycore64.tcz
echo OK

mkext i2c i2c-6.18.35-tinycore64.tcz
