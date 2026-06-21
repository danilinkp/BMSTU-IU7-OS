savedcmd_md1.mod := printf '%s\n'   md1.o | awk '!x[$$0]++ { print("./"$$0) }' > md1.mod
