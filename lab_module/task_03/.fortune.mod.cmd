savedcmd_fortune.mod := printf '%s\n'   fortune.o | awk '!x[$$0]++ { print("./"$$0) }' > fortune.mod
