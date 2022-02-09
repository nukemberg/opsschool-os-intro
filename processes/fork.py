#!/usr/bin/python3
import os, sys
import time

parent_pid = os.getpid()
print("PID: {}".format(parent_pid))
pid = os.fork()
if pid < 0:
    print("Failed to fork, returned {}".format(pid), file=sys.stderr)
elif pid == 0:
    print("Child process")
    time.sleep(10)
else:
    print("Forked child {}".format(pid))
    time.sleep(10)
    
