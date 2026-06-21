savedcmd_my_workqueue.mod := printf '%s\n'   my_workqueue.o | awk '!x[$$0]++ { print("./"$$0) }' > my_workqueue.mod
