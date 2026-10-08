# FLinux-JC64

Remaster de CorePure64 (Tiny Core 17.1) de 64 bits para la **Dell Inspiron 1545**: kernel 6.18.35-flinux-jc64 con wifi Broadcom BCM4312 (b43, firmware LP-PHY), Intel GMA 4500MHD, audio HDA IDT y ethernet tg3.

Lleva preinstalados Brave Origin, FreeTube, FLConnect64 (WireGuard/OpenVPN), FLTV64, FLRadio, FileZilla reparado, FLFM, FLWriter, flskin y nano, con Xorg y el FLWM de degradado neón rojo/naranja/amarillo, instalador parcheado y ALSA al 100 % automático.

La ISO está en la Release **v2.0** (pestaña Releases), junto con su `.md5.txt`.

## Código fuente y licencia

El código propio de FLinux-JC64 y el de construcción de la ISO se publican bajo la **licencia GPL-3.0** (archivo `LICENSE` en la raíz de este repositorio):

- `build/` — scripts de construcción de la ISO, el parche de búsqueda del CDE para Ventoy y la configuración de isolinux.
- `kernel/` — configuraciones del kernel 6.18.35 (la base Tiny Core y la renombrada `flinux-jc64`).
- `apps/` — código fuente de las aplicaciones preinstaladas (FLFM, FLWriter, FLRadio, FLTV, FLWM, FLConnect64 y flskin).
- `INFORME-V2.md` — informe técnico completo de la versión 2.

Los componentes de terceros conservan sus propias licencias y su código se obtiene de sus proyectos oficiales: **Tiny Core Linux**, el **kernel Linux**, **Brave Origin** (Brave Software), **FreeTube**, **FileZilla**, **Conky** y **X.Org** (ver la sección «Créditos»).

Proyectos relacionados:

- FLConnect: https://github.com/jcarlosrf26/flconnect
- flskin: https://github.com/jcarlosrf26/flskin

## Créditos

- **Tiny Core Linux** — creado por Robert Shingledecker y el equipo Tiny Core (https://tinycorelinux.net/): la base de esta remasterización.
- **Kernel Linux** — Linus Torvalds y la comunidad del kernel (https://www.kernel.org/).
- **FLinux y sus apps FLFM, FLWriter, FLRadio, FLTV** — Facundo Adorno (https://flinux-distro.sourceforge.io/); esta distribución adapta su trabajo.
- **FLWM** — Bill Spitzak (https://flwm.sourceforge.net/).
- **WireGuard** — Jason A. Donenfeld (https://www.wireguard.com/).
- **Brave Origin** — Brave Software, Inc. (https://brave.com/origin/linux/).
- **FreeTube** — el equipo de FreeTube (https://freetubeapp.io/).
- **FileZilla** — Tim Kosse y el proyecto FileZilla (https://filezilla-project.org/).
- **Conky** — Brenden Matthews y el equipo Conky (https://github.com/brndnmtthws/conky).
- **X.Org** — X.Org Foundation (https://www.x.org/).
