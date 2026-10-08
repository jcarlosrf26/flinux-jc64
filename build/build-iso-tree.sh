#!/bin/bash
# Paso (b) del repack v2: sustituye el arbol de modulos del core ya
# personalizado (core-custom.gz, salida de repack_core_v2a.py) por el arbol
# renombrado 6.18.35-flinux-jc64 (kernel/coremods) y re-empaqueta el cpio.
set -e
W=$HOME/workspace/flinux-jc-64
V=6.18.35-flinux-jc64
rm -rf /tmp/core-v2 && mkdir -p /tmp/core-v2
( cd /tmp/core-v2 && zcat $W/v2/build/core-custom.gz | cpio -idm --no-absolute-filenames 2>/dev/null || true )
rm -rf /tmp/core-v2/lib/modules/6.18.35-tinycore64
cp -a $W/v2/kernel/coremods/lib/modules/$V /tmp/core-v2/lib/modules/
ls /tmp/core-v2/lib/modules/
# El core oficial trae lib/modules/$V/kernel.tclocal ->
# /usr/local/lib/modules/$V/kernel/ : depmod/modprobe (busybox) indexan
# y cargan por esa ruta los modulos de las extensiones *-KERNEL
# (b43, snd-hda-intel, i915, wireguard...). Sin el symlink, todo
# modulo de extension da "module X not found in modules.dep" y ni el
# wifi, ni el audio, ni wireguard cargan. (Detectado 2026-10-08.)
ln -sfn /usr/local/lib/modules/$V/kernel/ /tmp/core-v2/lib/modules/$V/kernel.tclocal
# modos del arbol de modulos como el core oficial (755/644): el stageo
# en el workspace deja 2770/640 y el usuario tc no puede leer
# modules.dep/modules.alias (modinfo y depmod de usuario fallan).
find /tmp/core-v2/lib/modules -type d -exec chmod 755 {} +
find /tmp/core-v2/lib/modules -type f -exec chmod 644 {} +
# la raiz del initramfs debe ser 0755 como el core oficial; la extraccion
# ciega hereda el modo del directorio de stageo (0770) y el login de tc
# no puede atravesar '/' -> "can't change directory to '/home/tc'"
chmod 0755 /tmp/core-v2
chmod 0775 /tmp/core-v2/opt
( cd /tmp/core-v2 && find . -print0 | cpio --null -o -H newc 2>/dev/null | gzip -9 ) > $W/v2/build/corepure64-v2.gz
ls -la $W/v2/build/corepure64-v2.gz

# --- ensamblado ISO ---
ISO=$W/v2/isoroot
rm -rf $ISO && mkdir -p $ISO/boot/isolinux $ISO/boot $ISO/cde/optional
cp -a $W/isoroot/boot/isolinux/. $ISO/boot/isolinux/
cp $ISO/boot/isolinux/isolinux.cfg /tmp/isolinux-v2-check.txt
cp $W/v2/kernel/linux-6.18.35/arch/x86/boot/bzImage /tmp/vmlinuz64-v2
cp /tmp/vmlinuz64-v2 $ISO/boot/vmlinuz64
cp $W/v2/build/corepure64-v2.gz $ISO/boot/corepure64.gz
cp $W/v2/build/onboot-v2.lst $ISO/cde/onboot.lst

# pool opcional: cierre completo (closure-v2.txt), origenes por prioridad:
# 1) mis paquetes v2/build/pkg, 2) flconnect reutilizado, 3) pool64 oficial
while read p; do
  [ -z "$p" ] && continue
  # los .dep oficiales traen el marcador -KERNEL: tce lo sustituye por
  # uname -r al cargar; aqui lo resolvemos ya al nombre renombrado
  case "$p" in *-KERNEL.tcz) p=${p/KERNEL/6.18.35-flinux-jc64};; esac
  if [ -f $W/v2/build/pkg/$p ]; then src=$W/v2/build/pkg/$p
  elif [ "$p" = flconnect.tcz ]; then src=$W/src/flconnect64/flconnect.tcz
  elif [ -f $W/dl/pool64/$p ]; then src=$W/dl/pool64/$p
  else echo "SIN ORIGEN: $p"; continue; fi
  cp $src $ISO/cde/optional/$p
  for ext in dep md5.txt; do
    if [ -f $src.$ext ]; then cp $src.$ext $ISO/cde/optional/$p.$ext
    elif [ -f $W/dl/pool64/$p.$ext ]; then cp $W/dl/pool64/$p.$ext $ISO/cde/optional/$p.$ext
    fi
  done
  [ -f $ISO/cde/optional/$p.dep ] || : > $ISO/cde/optional/$p.dep
  md5sum $ISO/cde/optional/$p | awk -v n=$p '{print $1"  "n}' > $ISO/cde/optional/$p.md5.txt
done < $W/v2/build/closure-v2.txt
echo "optional: $(ls $ISO/cde/optional/*.tcz | wc -l) paquetes, $(du -sh $ISO/cde/optional | cut -f1)"

ls -la $ISO/boot/vmlinuz64 $ISO/boot/corepure64.gz; cat $ISO/cde/onboot.lst | head -3

# el workspace impone 640/2750 (root:nogroup): dentro de la ISO esos modos
# dejan los .dep/.tcz ilegibles para tc y la carga de dependencias muere
# en silencio (awk no puede hacer getline de los .dep)
find $ISO -type d -exec chmod 755 {} +
find $ISO -type f -exec chmod 644 {} +
stat -c "%a" $ISO/cde/optional/fltv.tcz.dep $ISO/cde/onboot.lst
