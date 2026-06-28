# Knocker 🚪‼️ 

A simple single threaded blocking TCP port scanner made in C++ which knocks at each port's door in the specified range of ports defined by the user.
I can hear it say "I am the one who knocks".

---

## Usage
1. run `make`
2. run `./knocker <ip-adress> <port-start> <port-end>`

for eg. `./knocker 127.0.0.1 0 1000` will scan for ports from 0 to 1000 in the local host.
