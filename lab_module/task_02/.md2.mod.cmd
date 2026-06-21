savedcmd_md2.mod := printf '%s\n'   md2.o | awk '!x[$$0]++ { print("./"$$0) }' > md2.mod
