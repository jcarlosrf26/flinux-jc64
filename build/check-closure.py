#!/usr/bin/env python3
"""Resuelve el cierre .dep de la onboot v2 y comprueba que cada ELF del
sistema ensamblado resuelve sus NEEDED contra las libs presentes
(stageo completo en /tmp/v2-stage). Solo lectura/análisis."""
import os, subprocess, sys

W = os.path.expanduser("~/workspace/flinux-jc-64")
DIRS = [f"{W}/v2/build/pkg", f"{W}/dl/pool64"]
SPECIAL = {"flconnect.tcz": f"{W}/src/flconnect64/flconnect.tcz"}

def locate(pkg):
    for d in DIRS:
        if os.path.exists(f"{d}/{pkg}"):
            return d
    return None

def deps(pkg):
    d = SPECIAL.get(pkg) and os.path.dirname(SPECIAL[pkg]) or locate(pkg)
    f = f"{d}/{pkg}.dep"
    if not os.path.exists(f):
        return []
    return [l.strip() for l in open(f) if l.strip().endswith(".tcz")]

order, seen = [], set()
def visit(pkg):
    if pkg in seen:
        return
    seen.add(pkg)
    for d in deps(pkg):
        visit(d)
    order.append(pkg)

onboot = [l.strip() for l in open(f"{W}/v2/build/onboot-v2.lst") if l.strip()]
for p in onboot:
    visit(p)
print(f"cierre: {len(order)} paquetes")
missing = [p for p in order if not (locate(p) or p in SPECIAL)]
print("sin fichero local:", missing or "ninguno")
open(f"{W}/v2/build/closure-v2.txt", "w").write("\n".join(order) + "\n")

# --- stageo ---
STAGE = os.path.expanduser("~/workspace/.tmp/v2-stage")
if "--stage" in sys.argv:
    subprocess.run(["rm", "-rf", STAGE])
    os.makedirs(STAGE, exist_ok=True)
    for p in order:
        src = SPECIAL.get(p) or f"{locate(p)}/{p}"
        r = subprocess.run(["unsquashfs", "-f", "-d", STAGE, src],
                           capture_output=True)
    print("stageo completo en", STAGE)

    def elf_needed(path):
        try:
            out = subprocess.run(["readelf", "-d", path], capture_output=True,
                                 text=True).stdout
        except Exception:
            return None
        if "NEEDED" not in out and "no dynamic section" in out.lower():
            return None
        return [l.split("[")[1].rstrip("]") for l in out.splitlines()
                if "(NEEDED)" in l]

    def soname(path):
        out = subprocess.run(["readelf", "-d", path], capture_output=True,
                             text=True).stdout
        for l in out.splitlines():
            if "(SONAME)" in l:
                return l.split("[")[1].rstrip("]")
        return None

    LIBDIRS = ["usr/local/lib", "usr/lib", "lib"]
    libs = {}   # dir -> {soname/archivo}
    bad = []
    for root, _, files in os.walk(STAGE):
        rel = os.path.relpath(root, STAGE)
        for f in files:
            p = os.path.join(root, f)
            r = subprocess.run(["file", "-b", p], capture_output=True, text=True).stdout
            if "ELF" not in r:
                continue
            sn = soname(p) or f
            libs.setdefault(rel if rel != "." else "", set()).add(sn)

    def resolvable(name, bdir_rel):
        if name in libs.get(bdir_rel, set()):
            return True
        return any(name in s for d, s in libs.items()
                   if d.rstrip("/").startswith(tuple(LIBDIRS)))

    unresolved = {}
    nchecked = 0
    for root, _, files in os.walk(STAGE):
        rel = os.path.relpath(root, STAGE)
        for f in files:
            p = os.path.join(root, f)
            r = subprocess.run(["file", "-b", p], capture_output=True, text=True).stdout
            if "ELF 64-bit" not in r:
                continue
            need = elf_needed(p)
            if not need:
                continue
            nchecked += 1
            miss = [n for n in need if not resolvable(n, rel)]
            if miss:
                unresolved[os.path.join(rel, f)] = miss
    print(f"ELF x86-64 comprobados: {nchecked}")
    if unresolved:
        print("SIN RESOLVER:")
        for k, v in sorted(unresolved.items()):
            print(" ", k, "->", ", ".join(sorted(set(v))))
    else:
        print("cierre ELF completo: 0 sin resolver")
