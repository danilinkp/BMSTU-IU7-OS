savedcmd_tasklet.mod := printf '%s\n'   tasklet.o | awk '!x[$$0]++ { print("./"$$0) }' > tasklet.mod
