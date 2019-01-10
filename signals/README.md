# Fun with signals
This exercise teaches the basics of how signals work. During this exercise we will run and modify `run-me.py`.

Make sure you are running on a POSIX compatible O/S (Linux, OSX, etc).

When you run `python run-me.py` the process will print its _pid_ so you can easily send signals to it. 
Alternatively, use `python run-me.py &` to run the process in the background if you want to send signals using the same terminal.

## Signals
1. Run the process and kill it with _ctrl+c_. Run it again and send the `INT` signal to it.
1. Edit the process and install a signal handler on `INT` signal. Now run and use _ctrl+c_ again.
1. How do you stop a process without _ctrl+c_? Try sending the `QUIT` signal to the process.
1. Install a signal handler for the `QUIT` signal, run it again and send the `QUIT` signal to it.
1. Run and send the `HUP` signal to the process. Install a signal handler for the `HUP` signal, run and close the terminal (or SSH). Is the process still running?
1. Run and send the `KILL` signal to the process. Install a signal handler for `KILL` then try again.

## Signal queue
1. Run the process with signal handler install for `USR1` only
1. Send `USR1` to the process and then immediately send it again. Observe the output of the program.
1. Install a signal handler for `USR2` as well, run the program and send `USR1` immediately followed by `USR2` to the process. 

## Alarms
1. Run `alarm.py` process

## Discussion
- Which signals cause the process to exit normally? which cause it to exit abnormally? can this be controlled?
- What is the purpose of the `QUIT` signal?
- What is the default signal sent by `kill`? why?
- Which signals cannot be caught or ignored? why?
- What is the purpose of the `INT` and `HUP` signals?  
- What happens when a signal is caught while another signal is running? how can this be controlled?
- What is the purpose of the `ABRT` signal?
- What happens when a signal is caught during `sleep`? what happens when the signal handler finishes?

## References
- https://en.wikipedia.org/wiki/Signal_(IPC)#POSIX_signals
- [man 7 signal](http://man7.org/linux/man-pages/man7/signal.7.html)