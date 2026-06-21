savedcmd_md2.ko := ld -r -m elf_x86_64 -z noexecstack --no-warn-rwx-segments --build-id=sha1  -T /usr/lib/modules/7.0.3-arch1-2/build/scripts/module.lds -o md2.ko md2.o md2.mod.o .module-common.o
