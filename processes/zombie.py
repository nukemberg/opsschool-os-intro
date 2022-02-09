#!/usr/bin/python3

import os
import time
import signal

parent = os.getpid()
child = os.fork()

if child < 0:
    print("Fork failed: {}".format(child))
elif child == 0:
    print("Child forked, child exiting")
else:
    print("[parent({})] Child pid: {}".format(parent, child))
    print("[parent({})] Not waiting for child".format(parent))
    def wait_child(sig, _):
        pid, res = os.wait()
        print("Child {} done, exit code: {}".format(pid, res))
        
    signal.signal(signal.SIGUSR1, wait_child)
    time.sleep(60)


