savedcmd_md3.mod := printf '%s\n'   md3.o | awk '!x[$$0]++ { print("./"$$0) }' > md3.mod
