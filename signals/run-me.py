#!/usr/bin/env python

import os, sys
import signal
import time
import atexit
import uuid


def before_exit():
    print('Program exiting normally')


def signal_handler(signum, frame):
    handler_id = str(uuid.uuid4()) # a unique id for this handler instance
    print('[{}] Caught signal : {}, at line: {}'.format(handler_id, signum, frame.f_lineno))
    time.sleep(1) # this is so you have time to send another signal before the handler finishes
    print('[{}] Signal handler ({}) done'.format(handler_id, signum))


def main():
    atexit.register(before_exit)

    # In python's signal package, signal numbers are given as named constants:
    # E.g. TERM signal is signal.SIGTERM

    # To install a signal handler, uncomment and modify the following line
    #signal.signal(signal.SIGINT, signal_handler)

    # To ignore a signal, uncomment and modify the following line
    #signal.signal(signal.SIGIsNT, signal.SIG_IGN)
    
    print('PID: {}'.format(os.getpid()))
    while True:
        print('[{}] Running, sleeping for 10 seconds'.format(time.time()))
        time.sleep(10)


if __name__ == "__main__":
    main()
