# OS

Build:
 * `make`

 Create disk:
 * `qemu-img create -f raw disk.img 10M`

Run:
 * `qemu-system-x86_64 kernel.bin -drive file=disk.img`.
