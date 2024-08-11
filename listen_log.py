# A simple Python script for listening to debugger messages.
# You can run it on any device, whether it's a phone, tablet, or computer.
# On a phone or tablet, you can run it using Termux.
# The debugger is convenient to use by sending network packets, as the game runs in full-screen mode,
# which doesn't allow you to track debugger messages in real time.
# With this script, we will receive debugger messages on a separate screen on another device.

import socket
import time

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', 7878))
    print("Listening on 0.0.0.0:7878")
    
    while True:
        data, addr = sock.recvfrom(1024)
        message = data.decode('utf-8')
        current_time = time.strftime('[%H:%M:%S]')
        print(f"{current_time} <{message}>")

if __name__ == "__main__":
    main()