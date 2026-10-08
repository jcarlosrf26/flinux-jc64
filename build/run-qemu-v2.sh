#!/bin/bash
# Arranque de prueba flinux-jc64 con el QEMU empaquetado (tools-root).
W=$HOME/workspace/flinux-jc-64
ISO=${ISO:-$W/v2/out/flinux-jc64-v2.0.iso}
MODE=${1:-bootcd}
ORDER=d
[ "$MODE" = "bootdisk" ] && ORDER=c
QB=$HOME/workspace/tools-root/usr/bin
export LD_LIBRARY_PATH=$HOME/workspace/tools-root/usr/lib/x86_64-linux-gnu:$HOME/workspace/tools-root/lib/x86_64-linux-gnu
rm -f /tmp/qm64.sock /tmp/ser64.sock
if [ "$MODE" = "bootcdonly" ]; then
  exec $QB/qemu-system-x86_64 \
    -L $HOME/workspace/tools-root/usr/share/qemu \
    -L $HOME/workspace/tools-root/usr/share/seabios \
    -m 2048 -machine accel=tcg -cpu max -vga std -usb -device usb-tablet \
    -drive file=$W/v2/work/test.raw,format=raw,if=ide,index=0,media=disk \
    -drive file=$ISO,format=raw,if=ide,index=1,media=cdrom,readonly=on \
    -boot order=d \
    -nic user,model=e1000 \
    -device intel-hda -device hda-duplex \
    -vnc 127.0.0.1:3 \
    -monitor unix:/tmp/qm64.sock,server,nowait \
    -serial unix:/tmp/ser64.sock,server,nowait
elif [ "$MODE" = "diskonly" ]; then
  exec $QB/qemu-system-x86_64 \
    -L $HOME/workspace/tools-root/usr/share/qemu \
    -L $HOME/workspace/tools-root/usr/share/seabios \
    -m 2048 -machine accel=tcg -cpu max -vga std -usb -device usb-tablet \
    -drive file=$W/v2/work/test.raw,format=raw,if=ide,index=0,media=disk \
    -boot order=c \
    -nic user,model=e1000 \
    -device intel-hda -device hda-duplex \
    -vnc 127.0.0.1:3 \
    -monitor unix:/tmp/qm64.sock,server,nowait \
    -serial unix:/tmp/ser64.sock,server,nowait
else
  exec $QB/qemu-system-x86_64 \
    -L $HOME/workspace/tools-root/usr/share/qemu \
    -L $HOME/workspace/tools-root/usr/share/seabios \
    -m 2048 -machine accel=tcg -cpu max -vga std -usb -device usb-tablet \
    -drive file=$W/v2/work/test.raw,format=raw,if=ide,index=0,media=disk \
    -drive file=$ISO,format=raw,if=ide,index=1,media=cdrom,readonly=on \
    -boot order=$ORDER \
    -nic user,model=e1000 \
    -device intel-hda -device hda-duplex \
    -vnc 127.0.0.1:3 \
    -monitor unix:/tmp/qm64.sock,server,nowait \
    -serial unix:/tmp/ser64.sock,server,nowait
fi
