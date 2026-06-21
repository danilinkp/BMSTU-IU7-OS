savedcmd_md3.ko := ld -r -m elf_x86_64 -z noexecstack --no-warn-rwx-segments --build-id=sha1  -T /usr/lib/modules/7.0.3-arch1-2/build/scripts/module.lds -o md3.ko md3.o md3.mod.o .module-common.o
