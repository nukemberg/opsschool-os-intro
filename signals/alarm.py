#!/usr/bin/env python

import signal
import time
import socket


def handler(signum, frame):
    print('Signal {} caught'.format(signum))


def main():
    signal.signal(signal.SIGALRM, handler)

    signal.alarm(1)
    while True:
        print('Running, sleeping for 10 seconds')
        time.sleep(10)

        # uncomment this to listen on socket
        #print('Listening on socket')
        #s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        #s.recvfrom(1024)

if __name__ == "__main__":
    main()