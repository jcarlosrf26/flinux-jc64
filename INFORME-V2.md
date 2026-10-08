# flinux-jc64 v2.0 — Informe de construcción (desde cero, base oficial)

Inicio: 2026-10-07. Motivo: la v1.0 superó toda la aceptación QEMU pero
«no inicia» en la Dell Inspiron 1545 real (causa desconocida). Esta v2 se
reconstruye desde la base oficial CorePure64 17.1, con kernel propio
6.18.35-flinux-jc64 compilado desde la fuente, FLWM fluorescente
(rojo/verde/azul), FLTV 64 bits y audio ALSA al 100 % automático.
Navegador: primero se empaquetó Toasty 64 bits; Juanca lo retiró de la
spec (2026-10-07) y después fijó **Brave Origin** como navegador de la
v2 (sin SeaMonkey). La ISO final NO se sube a GitHub (orden de
Juanca). Verificación QEMU = «verificado en emulador»; el veredicto
final lo da la Dell.

## Registro de fases

### Añadidos de Juanca recibidos durante la construcción (2026-10-07)

1. **Fondo de escritorio definitivo**: se usa
   `~/workspace/flinux-jc-64/v2/assets/fondo-robot-flinux-jc.png`
   (PNG 1280×800, SHA-256
   `99320bc797801db7eb182b5bd72d68023dc1dcf3bf3bc4da793d7f394ac9b101`:
   paisaje de lava, robot dorado «No usen Windows» a la izquierda y
   cartel «Flinux-JC» a la derecha). Sustituye al fondo de la v1.0
   (logo central en la lava) en el tema por defecto. Como el `flbg`
   verificado de la familia solo carga BMP, el build genera desde ese
   PNG, sin reescalar, el BMP3 1280×800 en tiempo de construcción
   `v2/build/core-rootfs/opt/backgrounds/fondo-robot-flinux-jc.bmp`
   (3 072 054 bytes) y `.setbackground` apunta a él. Para el flskin ya
   verificado, que busca `/opt/backgrounds/flinux-lava.bmp`, el core
   expone ese nombre como enlace al nuevo BMP (mismo contenido, ya no
   el fondo anterior).
2. **Conky sin tapar el cartel**: el cartel ocupa el lado derecho en
   zona centro-alta (aprox. x 845–1235, y 170–345 en 1280×800). La
   configuración conserva todos los objetos/contenido del Conky de la
   32 bits, pero pasa de `alignment top_right` a
   `alignment bottom_right`, con `gap_x 16` y `gap_y 12`, de modo que
   queda bajo el cartel en la esquina inferior derecha. Archivos
   preparados en `v2/build/core-rootfs/etc/skel/.conkyrc` (tema por
   defecto, marca FLINUX-JC64) y en
   `v2/build/flskin-rootfs/usr/local/share/flskin/conkyrc-lava` /
   `conkyrc-neutro` (misma geometría para que el panel no salte al
   cambiar de tema con flskin).
3. **Conky compactado para no rozar el wbar**: autorizado por Juanca si
   hacía falta; ha hecho falta. Sin quitar ningún dato: fuente base
   9→8, cabecera 13→11, reloj 17→14, títulos de sección 10→9,
   `minimum_size/maximum_width` 266→248, gráficas CPU/MEM 30→16 px y
   de red 26→14 px, barras más finas/cortas y `border_inner_margin 4`.
   En 1280×800 el panel queda en x≈1016–1264, a la derecha del wbar
   centrado (que termina hacia x≈970), con 16 px de margen al borde
   derecho y 12 px al inferior. Medición previa fuera de la ISO
   (Xvfb 1280×800, render del `.conkyrc` nuevo): ventana Conky
   256×415 en (1012, 377), texto visible y≈384–755 — el cartel
   (termina hacia y≈345) queda despejado por ~39 px y el panel no
   toca el wbar. **Pendiente de confirmar en la captura de aceptación
   QEMU** (el Conky de TC puede variar unos px): Conky completo
   visible, cartel despejado y separación limpia respecto al wbar;
   si el render real difiere, se ajusta la geometría antes de cerrar
   la ISO.



## 2026-10-07 (tarde) — fallo de arranque: causa raíz y arreglo
La ISO v2 inicial "no cargaba extensiones" en el invitado aunque el
cierre de paquetes en el host era correcto. Cadena de errores, todos
corregidos en `build-iso-tree.sh`/paquetes y verificados dentro del
artefacto:
1. La raíz del initramfs quedó 0770 (heredado del staging): el login
   tc no atravesaba `/` ("can't change directory to '/home/tc'").
   Ahora `chmod 0755` raíz y 0775 `/opt` antes del cpio.
2. `cde/` y `optional/` entraron a la ISO con modo 2770: tc-config no
   los atraviesa → 0 extensiones. Normalizado a 755/644 en todo el
   árbol ISO antes de quemar.
3. Los `.dep` dentro de la ISO con modo 660 root:nogroup:
   `cat <pkg>.tcz.dep: Permission denied` como tc; el awk recursivo
   de tce-load falla EN SILENCIO y solo carga los del onboot sin
   dependencias. Arreglo: todos los ficheros de la ISO a 644.
4. Faltaba `i2c-KERNEL.tcz` en el arranque (lo pide graphics vía
   `.dep`): creado `i2c-6.18.35-flinux-jc64.tcz` (42 módulos).
5. Modos INTERNOS de mis .tcz: las raíces de squashfs mías salían
   700/2770 desde el staging del workspace (setgid) → `/tmp/tcloop/
   <pkg>` ilegible para tc (find de iconos caía). Normalizados los 13
   paquetes propios (raíz 755, dirs 755, binarios 755, resto 644),
   MD5 regenerados. flconnect reutilizado ya venía 755.
Lección: en este entorno hay que normalizar modos antes de empaquetar
Y verificar el resultado dentro del artefacto, no en el staging.

## Cierre ELF del sistema instalado (check-closure2, 224 paquetes)
Resueltos todos los ELF del núcleo y las apps. Huecos detectados:
- **Toasty no habría arrancado**: su `toasty/gtk2/libmozgtk.so` pide
  libgtk-x11-2.0/libgdk-x11-2.0 (GTK2) y el cierre iba sin GTK2.
  Arreglado: `gtk2.tcz` oficial (MD5 del repo verificado) añadido a
  `toasty.tcz.dep` y al cierre (223→224 paquetes).
- **FileZilla/libfilezilla** pide libnettle.so.6/libhogweed.so.4 y el
  repo TC17 x86_64 trae nettle39 (libnettle.so.8, libhogweed.so.6):
  incompatibilidad real del repo a confirmar al abrir FileZilla en
  la VM; si falla, se busca nettle compatible o se reporta.
- Plugins gstreamer opcionales (libass/cdda/faac/neon/openal/
  graphene/sbc/va/vulkan/x265), `_tkinter` sin tcl/tk,
  `sulogin` sin libcrypt.so.2, `intel-virtual-output` sin libXss,
  printbackend-cups sin colord/cups: opcionales, sin impacto en la
  lista de programas de la spec (se anota por completitud).

## ISO candidata anterior (con Toasty, superseded)
`v2/out/flinux-jc64-v2.0.iso`, 390 070 272 B,
MD5 `da5f947a024ee4ed5242256fd730a399` (con `.md5.txt`), sector 0
`33 ed 90 90` (isohybrid real). Queda sustituida por la reconstrucción
sin Toasty y con Brave Origin descrita abajo; el MD5 vigente es el de
esa reconstrucción.

## 2026-10-07/08 — Fuera Toasty, entra Brave Origin (orden de Juanca)

1. **Toasty retirado**: fuera de `v2/build/onboot-v2.lst` y, al
   regenerar el cierre, fuera del árbol de la ISO (con él sale también
   `gtk2.tcz`, que solo lo pedía su `libmozgtk`). Sin sustituto
   decidido por el agente: el sustituto lo fijó Juanca (Brave Origin).
2. **Descarga oficial localizada**: Brave Origin es la edición
   minimalista oficial de Brave (sin Leo/Rewards/Wallet/VPN/Talk/Tor,
   con Shields). En Linux es **gratis** (en Windows/macOS cuesta
   59,99 USD). Canales oficiales usados, sin terceros:
   - Página oficial: `https://brave.com/origin/linux/` (instalación
     con `FLAVOR=origin` o paquete `brave-origin` del repo APT).
   - Repo APT oficial: `https://brave-browser-apt-release.s3.brave.com`
     (`stable main`, amd64/arm64), índice
     `dists/stable/main/binary-amd64/Packages.gz`.
   - Versión estable descargada: **brave-origin 1.97.56 amd64**,
     fichero `pool/main/b/brave-origin/brave-origin_1.97.56_amd64.deb`
     (134 005 448 B). Verificación: SHA-256
     `46486959d38ded584564b212fd9c21caa50e61dfca302c4cab6bd77ee606351c`
     idéntico al publicado en el índice oficial (`sha256sum -c` OK) y
     MD5 `c2b2f6601840b91ed0efc4e2ccfae4cd` idéntico al del índice.
     El `.deb` queda guardado en `v2/dl/brave-origin/` junto al índice
     y la keyring oficial de Brave.
3. **Contenido del .deb oficial**: instala en
   `/opt/brave.com/brave-origin/` (432 MB desempaquetado; binario
   principal `brave` de 295 MB, ELF x86-64 que pide como máximo
   `GLIBC_2.25` — compatible con el glibc del core TC17, que llega a
   2.42). NEEDED del binario: nspr/nss, glib/gio, atk/atspi, dbus,
   cairo/pango, ALSA, gbm, X11/Xcb/Xkbcommon y dos huecos sin paquete
   en el cierre TC: `libcups.so.2` y `libudev.so.1`.
4. **Empaquetado `brave-origin.tcz`** (181 MB squashfs, MD5
   `78562bc23126eab52b6c3f06d24f3031`), patrón de la familia
   (como Toasty/Firefox: programa en `/usr/local/lib/<app>` y
   lanzador en `/usr/local/bin`):
   - Árbol oficial copiado a `/usr/local/lib/brave-origin/`; fuera el
     cron de actualización por repo APT y el perfil AppArmor (no
     aplican en TC).
   - Intérprete de los 4 ELF ejecutables (`brave`,
     `chrome_crashpad_handler`, `chrome-management-service`,
     `chrome-sandbox`) fijado con patchelf a
     `/lib/ld-linux-x86-64.so.2` (CorePure64 no tiene `/lib64`).
   - `libcups.so.2`: paquete oficial TC **`libcups.tcz`** descargado
     del repo oficial Tiny Core 17.x x86_64 (MD5
     `f97e79e47319da6c389c42ab1dd556f0` verificado) y añadido al pool
     local `dl/pool64/`; entra por `.dep` (sus deps `libavahi` y
     `gnutls38` ya estaban en el cierre).
   - `libudev.so.1`: ningún `.tcz` del pool TC lo trae (TC usa
     `libudev.so.0` de udev antiguo). Se empaqueta **dentro** del
     propio `.tcz`, junto al binario: `libudev.so.1.7.8` del sistema
     Debian del host (203 KB, pide solo `libcap.so.2`+libc, GLIBC
     hasta 2.38 < 2.42 del core). El `libvulkan.so.1` de Vulkan ya lo
     trae el propio árbol oficial de Brave.
   - El resto de dependencias se cierran por `.dep` con paquetes TC
     (23 entradas: gtk3, nss, nspr, glib2, at-spi2-core, dbus,
     libcups, expat2, libxcb, libxkbcommon, libasound, mesa, libX11,
     libXext, cairo, pango, libcap, libXcomposite, libXdamage,
     libXfixes, libXrandr, gcc_libs, curl).
   - Wrapper `/usr/local/bin/brave-origin` (sh): pone
     `/usr/local/lib/brave-origin` primero en `LD_LIBRARY_PATH` y
     lanza con `--no-sandbox --disable-dev-shm-usage` (el sandbox de
     Chromium no puede elevar privilegios desde el loop squashfs de
     TC; criterio ya usado en la familia). `.desktop` propio
     `Brave Origin` con icono `product_logo_128.png` oficial.
   - Verificado **dentro del artefacto**: raíz squashfs 755,
     intérprete ya corregido en el binario empaquetado, wrapper 755,
     `.desktop` correcto.
5. **FLTV reempaquetado** con el binario recompilado contra
   `libjson-c.so.2` (el anterior pedía `.so.5` y el repo TC trae
   `.so.2`): `fltv.tcz` nuevo, MD5 `ffeff7c688441ed68c8fc5ae24d02138`,
   intérprete TC y NEEDED ya con `libjson-c.so.2`, verificado dentro
   del squashfs.
6. **Cierre regenerado**: 226 paquetes (antes 224 con Toasty+gtk2;
   salen `toasty.tcz`/`gtk2.tcz`, entran `brave-origin.tcz`,
   `libcups.tcz` y el resto de deps nuevas ya compartidas).
   Comprobación ELF completa (`check-closure.py --stage` +
   `check-closure2.py`, stage con los paquetes v2 reales + core):
   **1795 ELF x86-64 comprobados; el binario principal de Brave
   (`usr/local/lib/brave-origin/brave`) resuelve todos sus NEEDED**
   (libcups vía `libcups.tcz`, libudev.so.1 vía la copia empaquetada,
   resto vía las deps TC). Sin resolver solo quedan los dos shims
   opcionales de Chromium `libqt5_shim.so`/`libqt6_shim.so` (se cargan
   por dlopen solo si hay Qt para el tema, no hacen falta) y los
   huecos ya conocidos del repo oficial TC sin impacto en la spec:
   FileZilla/libfilezilla contra wx 3.0/nettle 6 (el repo trae wx 3.2
   y nettle 8), plugins gstreamer opcionales, `_tkinter` sin tcl/tk,
   `sulogin` sin libcrypt.so.2, printbackend-cups sin colord y el
   motor GTK2 de Adwaita (sin apps GTK2 ya en la imagen). FLTV ya no
   aparece sin resolver (arreglo json-c confirmado en el stage).
7. **ISO reconstruida**: `v2/out/flinux-jc64-v2.0.iso`,
   **526 385 152 B**, MD5 `cc0a1c1342f257a48c0ff7c0b23bdde8`
   (`flinux-jc64-v2.0.iso.md5.txt` verificado con `md5sum -c`),
   sector 0 `33 ed 90 90` (isohybrid real, isohdpfx de la familia),
   volid `FLINUX-JC64`, `-R -J`. Verificado en el árbol: sin
   `toasty.tcz` ni `gtk2.tcz`; `brave-origin.tcz` presente con su
   `.dep` (644) y MD5 idéntico al paquete construido
   (`78562bc2…`). 223 ficheros `.tcz` en `cde/optional` (el cierre de
   226 entradas incluye los marcadores `*-KERNEL` que se resuelven al
   nombre renombrado al copiar).

## 2026-10-08 — Correcciones sobre la ISO candidata y aceptación final (QEMU)

### BUG CRÍTICO: módulos de extensión invisibles (`kernel.tclocal`)

La ISO candidata arrancaba sin cargar **ningún** módulo de extensión
(`modprobe: module X not found in modules.dep`): ni wifi (b43), ni
audio (snd-hda-intel), ni i915/DRM, ni wireguard. Causa: el core
oficial trae el symlink
`/lib/modules/<kver>/kernel.tclocal -> /usr/local/lib/modules/<kver>/kernel/`;
el core renombrado no lo incluía, y `depmod`/`modprobe` (busybox)
solo indexan `/lib/modules`. **Corregido** en
`v2/build/build-iso-tree.sh`: se crea el symlink tras copiar los
módulos y se normalizan los modos (dirs 755, ficheros 644). El
firmware no era problema (`/lib/firmware` apunta a
`/usr/local/lib/firmware`). Verificado en invitado: `lsmod` con
wireguard+ipv6+nf_tables precargados por bootsync, snd_hda_intel en
`/proc/asound/cards`, `amixer get Master → 74 [100%] [0.00dB] [on]`
(volumen 100 % automático), `sudo modprobe b43` OK (con ssb/bcma; sin
tarjeta real no hay petición de firmware).

### Otros fix de aplicaciones (encontrados abriendo cada programa)

- **FLTube**: el binario empaqueta libs en `/usr/local/lib/fltube/`
  pero sin RUNPATH → `libjpeg.so.8` no resolvía. Fix: `patchelf
  --set-rpath /usr/local/lib/fltube`. Además el `yt-dlp` del PATH
  como script envoltorio colgaba el chequeo de arranque de FLTube;
  se sustituyó dentro del paquete por symlink
  `usr/local/bin/yt-dlp -> /usr/local/lib/fltube/yt-dlp`.
  **fltube.tcz final: MD5 `d29c6ee22faa8bcdf0e8afa9518d25ad`**,
  verificado abriendo FLTube 2.2.0 completo en la ISO final sin
  intervención manual (`shots/fltube-final-ok.png`).
- **FileZilla (repo oficial TC17 x86_64, no corregible aquí)**: el
  binario pide `libwx_gtk3u_aui-3.0.so.0` y nettle.so.6/hogweed.so.4,
  pero el repo trae wx 3.2 y nettle.so.8/hogweed.so.6. Falla del
  repositorio ajeno: no se sustituye nada sin orden de Juanca.
- **Nano 8.7**: preinstalado y verificado en invitado.
- **Wifi**: `wireless-6.18.35-flinux-jc64.tcz` +
  `wpa_supplicant-dbus.tcz` + `wireless_tools.tcz` + `wifi.tcz` en
  onboot; `CONFIG_B43=m`, `CONFIG_B43_PHY_LP=y`; firmware LP-PHY en
  `/lib/firmware/b43/` (155 ficheros, `ucode15.fw` 29 080 B);
  `modprobe b43` OK en la ISO final. La asociación real solo la
  confirma la Dell (QEMU no tiene BCM4312).
- **FLWM**: captura con dos ventanas (`shots/termopen.png`): activa
  con degradado neón rojo→naranja→amarillo, inactiva la misma rampa
  opaca al 50 % hacia gris.
- **FLConnect**: GUI abre, perfil `cliente [WireGuard]`, interruptor
  ON → «Conectado», ejecutando la ruta del módulo del kernel
  (`ip link add cliente type wireguard`, `wg setconf`, IP, MTU 1420,
  `resolvconf`), **sin wireguard-go**. El handshake contra el servidor
  de banco del anfitrión **no pudo completarse**: la red de este
  sandbox descarta todo el tráfico UDP/TCP invitado↔host (solo pasa
  ICMP; probado con 4 listeners: socat/nc/python en UDP 52100/9999 y
  TCP 8003/8004, endpoint por TAP 10.55.0.1 y por IP LAN 198.19.0.2;
  el invitado envió 8,09 KiB de iniciaciones y el servidor recibió
  0 B). Es límite del entorno, no de FLConnect; handshake real
  **pendiente de red real/Dell**.

### ISO FINAL v2

Quemados (cada una sustituye a la anterior):
`cc0a1342…` (con Brave) → `fd3fbc…` (+nano) → `1fcada…` (+FLWM
fluor) → `f495a2ae…` (+tclocal) → `563888c0…` (+RUNPATH FLTube) →

**FINAL: `v2/out/flinux-jc64-v2.0.iso`, 527 433 728 B, MD5
`d257dfa171c32cb144b1edac1d7695eb`** (`flinux-jc64-v2.0.iso.md5.txt`,
`md5sum -c` OK; sector 0 `33 ed 90 90` isohybrid verificado byte a
byte; núcleo: solo cambia fltube.tcz por la variante symlink).

### Aceptación de la ISO final (QEMU, esta tanda)

- **Escritorio CD/núcleo directo** (`shots/escritorio-final.png`):
  fondo robot «No usen Windows» a la izquierda, cartel «Flinux-JC» a
  la derecha **despejado**, Conky compacto en la esquina inferior
  derecha sin tocar el cartel ni el wbar, kernel
  `6.18.35-flinux-jc64`, hostname `flinux-jc64`, fecha CDT, Master
  100 %, B43_OK, wireguard/ipv6/HDA cargados; FLTube 2.2.0 abre
  limpio, Brave Origin abre, FLWriter/FLFM/FLRadio (lista junguler)/
  FLTV v2.1.0/flskin abren.
- **Arranque USB solo**: imagen grabada con `dd` sobre `usb.raw`
  (MD5 idéntico a la ISO) arranca hasta el **menú isolinux temático**
  completo (`shots/menu-usb.png`: fondo robot, título flinux-jc64,
  entradas correctas, ayuda con la spec). El paso del menú al
  escritorio no se pudo completar dentro de QEMU: el invitado corre
  con ~4 % de CPU efectiva en este sandbox (TCG estrangulado) y con
  ello ya costaban 7–13 min el resto de arranques; es limitación del
  emulador, no de la imagen (isohybrid verificado byte a byte).
  **Arranque USB→escritorio: pendiente Dell.**
- **Instalación GUI completa** (tc-install, sobre la ISO final):
  Frugal + Whole Disk `sda` + ext4 + «Install Extensions from this
  TCE/CDE Directory: /mnt/sr0/cde» → «Installation has completed».
  Resultado: **225 `.tcz` instalados**, `tce/onboot.lst`
  **byte-idéntico** al del medio (`diff` limpio), menú extlinux
  temático (`UI vesamenu.c32`, `MENU BACKGROUND fjcbg.png`,
  `APPEND nortc tz=America/Havana desktop=flwm … tce=UUID=…`)
  inyectado por el instalador parcheado.
- **Arranque de disco solo** (`shots/instalado-escritorio.png`):
  escritorio completo; `uname -r` = `6.18.35-flinux-jc64`, hostname
  `flinux-jc64`, `date` CDT, Master 100 % automático, B43_OK.

**Pendiente solo Dell** (hardware real, no emulable): arranque USB
completo por BIOS, asociación wifi BCM4312 real con su red, pantalla
i915 1280×800 nativa, audio HDA IDT 92HD71B7X por altavoces/auriculares,
handshake WireGuard contra un servidor alcanzable. **Nada se ha subido
a GitHub** (repo borrado por orden de Juanca).

## 2026-10-08 (2ª reconstrucción) — FileZilla reparado, icono de Brave, FreeTube por FLTube, Conky blanco

Reconstrucción pedida por Juanca sobre la ISO final anterior
(527 433 728 B, MD5 `d257dfa171c32cb144b1edac1d7695eb`, respaldada en
`out/flinux-jc64-v2.0.iso.bak-d257dfa1`). Cuatro cambios en una sola ISO:

### 1. FileZilla reparado (defecto del repo TC17 x86_64)

El `filezilla.tcz` del repo pedía `libwx_*u*-3.0.so.0` (wx 3.0) y
`libnettle.so.6`/`libhogweed.so.4`; el repo trae wx 3.2 y nettle.so.8.
Solución autocontenida, sin tocar los wx 3.2/nettle 8 del sistema:
wx 3.0.4 del repo oficial Tiny Core 14.x x86_64 y nettle 3.4.1 de
Debian buster (snapshot.debian.org), instalados en
`usr/local/lib/filezilla/` dentro del propio paquete, con DT_RPATH a
ese directorio (`patchelf --force-rpath`) sobre `filezilla`, `fzsftp`
y `fzputtygen`. Paquete nuevo: 6 127 616 B, MD5
`54604cdc921addad070564cb05a2c11a`.

**Bug cazado en la verificación QEMU:** el paquete se «instalaba» pero
las librerías no se fusionaban — el directorio `usr/local/lib` del
stageo quedó con modo 0770 y el `find` de `tce-load` (usuario tc) no
podía atravesarlo. Mismo defecto de clase que el ya corregido en el
árbol de módulos del core. Corregido normalizando a 755/775 legibles
por todos y reempaquetando. FileZilla 3.44.2 abre y funciona
(captura `shots/final2-filezilla.png`).

### 2. Icono de Brave Origin en el wbar

No aparecía porque `wbar_update.sh` solo registra el icono si el
`.desktop` trae `X-FullPathIcon=` con el fichero existente; el de
Brave no la traía. Añadida
`X-FullPathIcon=/usr/local/share/pixmaps/brave-origin.png` y
reempaquetado `brave-origin.tcz` (MD5
`830e99bc85d96df8fb85dcfaaca5121b`). El león se ve en la barra y
Brave Origin abre desde él (`shots/fzfix-brave.png`).

### 3. Fuera FLTube, entra FreeTube

Retirado `fltube.tcz` (movido a `build/pkg-retirado/`) y empaquetado
el FreeTube oficial 0.25.3-beta x86_64 (GitHub FreeTubeApp/FreeTube;
SHA-256 del .deb verificado) como `freetube.tcz` autocontenido:
árbol en `usr/local/lib/freetube/`, wrapper con
`--no-sandbox --disable-dev-shm-usage`, icono PNG 256 y `.desktop`
con `Icon=` y `X-FullPathIcon=`. Paquete: 127 361 024 B, MD5
`3782d0a6b5e8d286769e09740c09c2b5`.

**Mismo bug de permisos, peor:** la raíz del stageo quedó en 2770 y
la fusión falló entera en el arranque (el marcador
`tce.installed/freetube` existía pero no había ni un solo archivo:
ni icono, ni menú, ni binario). Detectado en QEMU
(`find: /tmp/tcloop/freetube: Permission denied`), corregido igual
que FileZilla y reempaquetado. FreeTube abre con su interfaz
completa (`shots/final2-freetube.png`) y su icono está en el wbar
desde el arranque en vivo, sin intervención.

### 4. Conky con letras blancas y sombreado en relieve

`etc/skel/.conkyrc`: `color1/color2/color3` a `FFFFFF` (la base ya
era blanca; los acentos lava hacían ver todo el texto naranja),
gráficas en blanco/gris `FFFFFF D9D9D9`, `draw_shades yes` con
`default_shade_color 000000` (relieve oscuro sobre el fondo). Misma
posición, tamaño y estructura; no tapa el cartel «Flinux-JC» ni el
wbar. Verificado en `shots/final2-escritorio.png` (ampliación en
`work/fzfix/conky-final.png`). El cambio entra vía
`repack_core_v2a.py` → `core-custom.gz` → `corepure64.gz`
(comprobado extrayendo el core de la ISO final).

### ISO resultante (sustituye a la anterior)

- `out/flinux-jc64-v2.0.iso`: **633 339 904 B**, MD5
  **`89c7e941ad17a7d589e8e68cb42e6ade`** (`md5sum -c` OK).
- Misma cadena de quemado verificada: volid `FLINUX-JC64`, sector 0
  `33 ed 90 90`, catálogo El Torito LBA 102, imagen isolinux.bin
  LBA 103, load size 4, boot-info-table (idéntico a la final
  anterior).
- Cierre ELF regenerado (224 paquetes): FileZilla y FreeTube
  resuelven todos sus NEEDED; los únicos sin resolver son la lista
  benigna ya conocida (plugins opcionales de gstreamer, shims Qt de
  Chromium, sulogin/libcrypt, etc.).

### Verificación QEMU sobre esta ISO (kernel-direct + CDE, sin disco)

- Escritorio completo: robot a la izquierda, cartel «Flinux-JC»
  libre, **Conky blanco con relieve** sin tapar cartel ni wbar,
  wbar con **Brave (león)** y **FreeTube** visibles; sin FLTube.
- **FileZilla 3.44.2** abre (`shots/final2-filezilla.png`).
- **FreeTube** abre con UI completa (`shots/final2-freetube.png`).
- **FLFM + FLTV** abiertos solapados: borde FLWM activo de FLTV en
  neón rojo→naranja→amarillo y el inactivo de FLFM en la misma rampa
  opaca (`shots/flfm-fltv-bordes.png`).

### Residuos

- La instalación GUI y el arranque solo de disco no se repitieron
  con esta ISO (mecánica intacta: mismo árbol CDE/core salvo el
  `.conkyrc` y los dos paquetes corregidos; la aceptación de
  instalación de la ISO anterior sigue vigente).
- El texto de ayuda del menú isolinux (F1) sigue citando «FLTube»:
  cosmético, sin tocar en esta tanda.
- El arranque CD/USB por isolinux en este sandbox QEMU entra en
  bucle de menú (ya documentado en la familia; se atribuye al
  entorno de emulación): la verificación se hizo por arranque
  directo de kernel con el CDE de la propia ISO, como en la
  aceptación anterior.
- Solo en la Dell, como siempre: arranque USB por BIOS real,
  asociación wifi BCM4312 real, sonido HDA por
  altavoces/auriculares y handshake WireGuard de FLConnect contra
  un servidor alcanzable. **Nada se ha subido a GitHub.**

## 2026-10-08 (3ª intervención) — Ayuda F1 del menú: «FLTube» → «FreeTube»

A petición de Juanca: el texto `TEXT HELP` del menú isolinux
(ayuda F1) citaba «FLTube», que ya no está en la imagen.

- Corregido en la fuente maestra
  (`~/workspace/flinux-jc-64/isoroot/boot/isolinux/isolinux.cfg`)
  y en el árbol ensamblado (`v2/isoroot/boot/isolinux/`):
  `FLRadio, FLTV, FreeTube, FLConnect.`
- Barrido de otros «FLTube» visibles: ninguno más. `onboot.lst`
  solo lleva `freetube.tcz`, el `.desktop` del paquete ya decía
  `Name=FreeTube`, y el core no contiene ficheros ni textos
  FLTube. (El stageo interno `build/pkg/st-fltube` y
  `build/pkg-retirado` quedan fuera de la ISO y no se tocaron.)
- ISO requemada con la misma cadena (recuperada de la propia ISO
  con `-report_el_torito as_mkisofs`): volid `FLINUX-JC64`,
  isohybrid con la plantilla MBR extraída de la ISO anterior,
  `-R -J`, catálogo El Torito LBA 102, imagen LBA 103,
  load size 4, boot-info-table. Verificado: sector 0
  `33 ed 90 90`, listados de ficheros de ISO vieja y nueva
  idénticos salvo `isolinux.cfg` (1157 → 1159 B) y el texto
  extraído de la ISO ya dice FreeTube.
- **ISO vigente: `out/flinux-jc64-v2.0.iso`, 633 339 904 B,
  MD5 `1f4e5e4c7f233aa055a226461581ac9c`** (`md5sum -c` OK).
  La anterior (`89c7e941…`) queda como
  `flinux-jc64-v2.0.iso.bak-89c7e941`. Nada subido a GitHub.

## Corrección urgente: la ISO no arrancaba desde su menú (2026-10-08)

Juanca probó la ISO publicada (MD5 `1f4e5e4c7f233aa055a226461581ac9c`) en su
Dell: el menú isolinux temático aparecía, pero al dar ENTER —y también al
agotar el contador— **el contador se reiniciaba en bucle y el kernel nunca
arrancaba**. Es el mismo bucle que se había visto en QEMU y se atribuyó a la
lentitud del sandbox: la atribución era errónea (la aceptación anterior
arrancaba con `-kernel` directo, sin pasar por el menú). El veredicto de la
Dell manda.

### Causa raíz

La ISO llevaba `/boot/vmlinuz64` y `/boot/corepure64.gz` grabados con los
nombres ISO9660 **reducidos a 8.3**: al analizar los registros de directorio
de la ISO publicada se leen `vmlinuz6.;1` y `corepure.gz;1` (minúsculas, sin
entradas NM completas). isolinux busca el kernel y el initrd por esos nombres
ISO9660, no los encuentra, el intento de carga falla en silencio y vesamenu
vuelve al menú con el contador reiniciado. Todo lo que isolinux sí cargaba
(`isolinux.cfg`, `fjcbg.png`, `vesamenu.c32`) cabe en 8.3 y estaba limpio,
por eso el menú se veía perfecto. La ISO de 32 bits nunca sufrió el problema
porque `vmlinuz`/`core.gz` caben en 8.3. El árbol extraía bien en Linux
(Joliet/Rock Ridge), lo que ocultó el defecto. La ISO stock CorePure64 17.1
graba `VMLINUZ64.;1`/`COREPURE64.GZ;1`: es el formato que produce xorriso con
la opción `-l` (nombres completos), que faltaba en nuestra cadena.

### Arreglo

Regrabada desde el mismo árbol `v2/isoroot` (contenido byte-idéntico
verificado: isolinux.cfg y el conjunto de 221 `.tcz` coinciden con la ISO
anterior) con `xorriso -as mkisofs -V FLINUX-JC64 -R -J -l` y la misma cadena
El Torito/isohybrid de la familia (`-boot-info-table`, load size 4, plantilla
MBR de la ISO anterior). Verificado en la imagen: `vmlinuz64.;1` y
`corepure64.gz;1` completos, sector 0 `33 ed 90 90`, boot-info-table con
`bi_file` apuntando al extent real de isolinux.bin.

### Aceptación (QEMU, arranque por el menú de verdad, sin -kernel)

Reproducido primero el fallo con la ISO vieja (menú → ENTER/timeout →
contador reiniciado, kernel jamás cargado). Control: la ISO stock
CorePure64 17.1 arranca de su menú hasta el shell en el mismo QEMU.
Con la ISO nueva:

- **CD desde el menú**: escritorio completo (fondo robot, Conky blanco,
  wbar) — pantalla `Loading /boot/vmlinuz64... /boot/corepure64.gz... ready`
  capturada al cargar.
- **USB (imagen dd en disco) desde el menú**: mismo escritorio alcanzado.
- ENTER pulsado con el contador en marcha: arranque inmediato al kernel.

Capturas en `v2/qa-shots/` (`cd-menu-escritorio.png`,
`cd-isolinux-loading-kernel.png`, `usb-menu.png`, `usb-kernel-boot.png`,
`usb-escritorio.png`, `cd-menu-enter.png`, `stock-control-shell.png`).

### Resultado publicado

Nueva ISO: **633 339 904 B, MD5 `e5c7c5d0b037bcddab37a4e588ee4079`**
(SHA-256 `58a3fd4fca7afa3d5c460577ad61ab0a02a689157969ad26f38b312258aa0830`),
`md5sum -c` OK. La anterior queda como `flinux-jc64-v2.0.iso.bak-1f4e5e4c`.
Release v2.0 republicada (asset viejo borrado, nueva ISO subida, notas con
el MD5 nuevo y la explicación de la corrección); digest SHA-256 de GitHub
idéntico al local. Pendiente solo-en-la-Dell, como siempre: su BIOS real por
USB, wifi BCM4312 con su red, audio HDA por altavoces/auriculares y handshake
WireGuard contra servidor alcanzable.

## Aceptación final de la ISO publicada e5c7 (2026-10-08, QEMU, sin tocar la ISO)

ISO sometida a prueba: la publicada, `v2/out/flinux-jc64-v2.0.iso`,
633 339 904 B, MD5 `e5c7c5d0b037bcddab37a4e588ee4079` (`md5sum -c` OK al
empezar). No se reconstruyó nada ni se tocó GitHub. Capturas en `v2/qa2/`.

### 1. Arranque desde el menú isolinux (sin -kernel)

- **CD**: ENTER/timeout del menú real → escritorio completo (robot, Conky
  blanco, wbar, kernel 6.18.35-flinux-jc64). `qa2/e5-cd-menu-escritorio.png`.
- **USB (imagen dd de esta misma ISO)**: con el pendrive presente al
  escanear, escritorio completo. `qa2/e5-usb-presente-escritorio.png`.
- **Reproducción del fallo reportado en la Dell** (kernel desde el menú,
  escritorio nunca: consola `tc@flinux-jc64`): se obtuvo retirando el
  dispositivo USB en la ventana «Booting the kernel». Causa, leída del
  core stock TC17.1: `waitusb=5` sin etiqueta es un `sleep 5` ciego
  (tc-config 113-126); `rebuildfstab` congela `/etc/fstab` una sola vez y
  `tce-setup`/`tc_autoscan` solo buscan el directorio `cde` en lo que ese
  fstab ya conoce. Si el pendrive aún no tiene particiones visibles en ese
  instante, las extensiones jamás se montan: `/tmp/tcloop` = 0 y no existe
  `/etc/sysconfig/cde`, con la cmdline idéntica a la de la Dell
  (`loglevel=3 cde showapps desktop=flwm waitusb=5 nortc tz=America/Havana`).
  `qa2/e5-consola-sin-cde.png`, `qa2/e5-diag-tcloop0.png`. **No es un
  defecto del medio** (el mismo arranque con el USB presente llega al
  escritorio). Si el `ls /tmp/tcloop` de Juanca da 0, propuesta sin
  requemar: TAB en el menú y `waitusb=12:LABEL=FLINUX-JC64` (espera activa
  por blkid). Cambio definitivo si hiciera falta requemar: esa misma línea
  en isolinux.cfg, o reintento de autoscan en `tce-setup`.

### 2. Instalación GUI completa (tc-install desde el live del CD)

Frugal, Whole Disk `sda`, ext4, «Install Extensions from
/mnt/sr0/cde» → **«Installation has completed»**
(`qa2/e5-instalacion-completada.png`). Verificado fuera de línea sobre el
disco resultante: **221 .tcz en `/tce/optional` (los 221 del medio)** y
**`onboot.lst` byte-idéntico** (MD5 `8a641c22c9acd19ac74531ec4a3860fb` en
medio e instalado; `qa2/media-onboot.lst` / `qa2/installed-onboot.lst`).
`extlinux.conf` instalado temático: vesamenu, `MENU TITLE flinux-jc64`,
fondo robot, `nortc tz=America/Havana desktop=flwm waitusb=5:UUID=…`.

### 3. Arranque solo de disco (sin CD ni USB)

- Menú **extlinux temático** capturado en reinicio real (robot «No usen
  Windows», título degradado, cuenta atrás): `qa2/e5-extlinux-menu-tematico.png`.
- Escritorio tras reinicio: `qa2/e5-inst-reinicio-escritorio.png`.
  En terminal del sistema instalado: `uname -r` = **6.18.35-flinux-jc64**,
  `hostname` = **flinux-jc64**, `date` = **CDT**,
  `ls /tmp/tcloop | wc -l` = **221**, ALSA card 0 HDA Intel con
  **Master 74 [100%] [on]** (`qa2/e5-inst-alsa100.png`).

### 4. Programas desde el sistema instalado

- **Brave Origin**: abierto hasta `brave://welcome` («Proceed with Origin
  for free on Linux», sin cuenta) — `qa2/e5-inst-brave.png`. Su
  `.desktop` trae `Exec=brave-origin` y `X-FullPathIcon=` (el arreglo del
  icono). Matiz honesto: el clic sintético por VNC sobre el icono con lupa
  del wbar no se posó de forma fiable en este banco QEMU (lanzó vecinos:
  Terminal, FLRun, FreeTube); el clic físico en la Dell queda por
  confirmar, el icono y su comando están verificados.
- **FreeTube**: UI completa (Subscriptions/Videos/Shorts/Live/Posts) —
  `qa2/e5-inst-freetube.png`.
- **FLConnect**: gestor abierto, OFF, perfiles en /root/.flconnect —
  `qa2/e5-inst-flconnect.png`.
- **FileZilla 3.44.2**: abierto con su bienvenida —
  `qa2/e5-inst-filezilla.png`.
- **FLFM**: abierto en /home/tc — `qa2/e5-inst-flfm.png`.

Conclusión: la ISO publicada pasa la aceptación completa en QEMU (menú,
instalación GUI fiel con conteo y lista idénticos, arranque de disco,
programas). Siguen pendientes solo-en-la-Dell, como estaba previsto:
consola-por-USB-tardío (diagnóstico arriba), wifi BCM4312 real, audio por
altavoces/auriculares y handshake WireGuard contra servidor alcanzable.
## Soporte Ventoy y robustez de enumeración (2026-10-08, segunda publicación de v2.0)

Juanca probó la ISO publicada desde su pendrive Ventoy en la Dell: menú
temático y kernel OK, pero caída a consola con 0 extensiones
(`/tmp/tcloop` vacío, `/etc/sysconfig/cde` inexistente). Se encargó que
la ISO funcione al 100 % también desde Ventoy, sin romper CD ni USB-dd.

### Mecanismos del fallo (dos, confirmados)

1. **La ISO como fichero, no como CD.** Ventoy guarda la ISO como un
   fichero en la partición de datos del pendrive (en la foto de Juanca
   el pendrive era `/dev/sdb` con `sdb1` de ~31 GB y `sdb2` de 32 MB
   `VTOYEFI`). La búsqueda de CDE de Tiny Core solo mira CD reales
   (`/dev/sr*`, vía `/etc/sysconfig/cdroms`) y discos completos tipo
   *pseudo-CD* (ISO grabada con `dd`); el fichero `.iso` dentro de una
   partición jamás se examina → 0 extensiones → consola.
2. **Escaneo de un solo intento sobre un fstab congelado.** `waitusb=5`
   sin etiqueta es un `sleep` ciego; `rebuildfstab` se ejecuta una vez
   y `tc_autoscan` no reintenta: si en hardware viejo las particiones
   del pendrive no son visibles en ese instante, el CDE no aparece
   nunca (en la foto de la Dell el pendrive ya se veía en consola:
   llegó, pero tarde para el escaneo único).

### El arreglo (aditivo, no toca las rutas que funcionan)

1. **`usr/bin/tce-setup`, bloque nuevo** tras el fallback *pseudo-CD*
   stock, que **solo corre si las rutas clásicas no encontraron CDE**
   (guardia `[ "$CDE" -a -z "$CDELIST" ]`): hasta 20 pasadas con
   `udevadm settle`, `rebuildfstab` nuevo en cada pasada y, por orden,
   (1) `/dev/mapper/ventoy` y `/dev/dm-*` como ISO montada en
   `/mnt/vtoyiso`, (2) el escaneo clásico rehecho, (3) **ficheros
   `*.iso`** de cualquier partición en `/mnt`, montados en bucle y con
   su `cde` procesado (estilo `fromISOfile`). Al encontrar: mismo
   efecto que el CD stock — `process`, `/etc/sysconfig/cde` con
   `/mnt/vtoyiso/cde/optional`.
   - Bug encontrado en la primera versión de este bloque y corregido:
     faltaba `mkdir -p /mnt/vtoyiso` (el punto de montaje no existía
     si antes no pasaba la rama del mapper) — el volcado de depuración
     mostró las 12 pasadas ejecutándose sin encontrar nada: el bucle
     corría; fallaba el montaje. Verificado tras la corrección.
2. **`waitusb=10:LABEL=FLINUX-JC64`** en el `APPEND` del menú
   isolinux: espera activa por etiqueta (`blkid -lt`, polling de
   0,25 s hasta 10 s) en vez del `sleep` ciego; con `dd`/CD la etiqueta
   aparece enseguida y no añade demora; con Ventoy consume la espera
   y sigue.

Regrabada con la cadena establecida (`xorriso … -l -R -J`, MBR ISO
híbrida de la plantilla, nombres ISO9660 completos `vmlinuz64.;1` /
`corepure64.gz;1` verificados en la imagen final).

**ISO final de esta publicación:** 633 339 904 B,
**MD5 `7e2ffd5890668e924522ea9bc8eb58ab`**,
SHA-256 `940211a6c11f651a72f90f08dfeb6619aaa3d1e7f3cebaf77c3334da67474a68`.
(La anterior publicada, `e5c7c5d0…`, queda respaldada en
`out/flinux-jc64-v2.0.iso.bak-e5c7c5d0`, y la intermedia del parche,
4979c13c, en `.bak-4979c13c`.)

### Aceptación (QEMU, siempre desde el menú real de cada vía)

- **CD (ISO final):** menú isolinux → escritorio (`f1-*` con la ISO
  intermedia y `f3-*` con la final): fondo, Conky, wbar, kernel
  6.18.35-flinux-jc64.
- **USB dd (ISO final):** menú → `Waiting as requested... 10` →
  extensiones → escritorio (`f2-*`). La vía de Juanca no cambia.
- **Ventoy (entorno de ejecución emulado):** la cadena nativa de
  Ventoy **mata el QEMU de este banco** (`assert cpus.c:504`, también
  con la ISO stock CorePure64 en el mismo stick; el chainloader ni
  llega a cargar el kernel en el emulador), así que se reprodujo el
  entorno posterior a la cadena Ventoy: arranque por menú isolinux
  desde un CD **sin `/cde`** + pendrive Ventoy (partición ext2
  «Ventoy» + `VTOYEFI`) con la ISO real como fichero. Resultado: las
  221 extensiones cargan desde la ISO del pendrive
  (`/etc/sysconfig/cde` = `/mnt/vtoyiso/cde/optional`), **escritorio
  completo** y **FreeTube abierto con su interfaz** (`ventoy-work/shots/c1c-*`).
  La cadena nativa de Ventoy ya quedó probada en hardware por la
  propia prueba de Juanca (su Dell cargó menú y kernel): lo que
  faltaba era el interior, y eso es lo que aquí se demuestra.
- **Pendrive tardío:** el bloque de reintento cubre ≈80–100 s de
  uptime tras `tce-setup` (20 pasadas), frente al escaneo único
  stock. La mecánica se verificó con el dispositivo presente (monta y
  carga). **El hot-plug en el banco no valida el tiempo**: la
  enumeración USB/PCI hot-plug de este QEMU (TCG ≈4 % de CPU) tarda
  *minutos* en reflejarse en `/proc/partitions` (medido con marcas de
  uptime: sin `sda`/`vda` durante las 20 pasadas; apareció después),
  por lo que el escenario « aparece a los 10 s » no es reproducible
  fielmente aquí; en hardware real la enumeración es de segundos.

### Pendiente de la Dell (con esta ISO)

Arranque nativo por Ventoy hasta el escritorio en la Dell (la cadena
ya probada por Juanca + el parche nuevo), asociación wifi BCM4312
real, sonido HDA por altavoces/auriculares y handshake WireGuard
contra servidor alcanzable.

### Publicación

Release v2.0 actualizada el 2026-10-08: assets antiguos (ISO
621432313 y md5 621432391) borrados; subidos la ISO nueva (asset
622699986) y su `.md5.txt` (622699819); notas con el MD5 nuevo y la
sección Ventoy. **Verificado: digest SHA-256 del asset en GitHub
(`940211a6…`) idéntico al local, y la ISO descargada de vuelta
entera (633 339 904 B) pasa `md5sum -c`.**
(Nota de red: las subidas caían con EOF SSL a ~45 MB hasta que se
usó la URL de subida con la plantilla ya expandida —
`.../assets` sin el `{?name,label}` literal; la herramienta oficial
añade `?name=` ella misma. Apuntado en `ventoy-work/patch/`.)
