savedcmd_my_vfs.mod := printf '%s\n'   my_vfs.o | awk '!x[$$0]++ { print("./"$$0) }' > my_vfs.mod
