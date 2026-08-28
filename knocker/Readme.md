# Knocker 🚪‼️ 

A simple single threaded blocking TCP port scanner made in C++ which knocks at each port's door in the specified range of ports defined by the user.
I can hear it say "I am the one who knocks".

---

## Usage of knocker
1. run `make kncoker`
2. run `./knocker <ip-address> <port-start> <port-end>`

## Usage of knocer-epoll
1. run `make knocker-epoll`
2. run `./knocker-epoll <ip-address> <port-start> <port-end>`

for eg. `./knocker 127.0.0.1 1 1000` will scan for ports from 0 to 1000 in the local host.

### Note for the epoll one
If you try to compare the simple and epoll ones on local-host then you will definitely see that simple one out-performs epoll one.
To see the real power of epoll one, use and compare them for an actual network latency.
The easiest way to introduce that is by connecting your PC and phone to the same Wi-Fi network and then putting the ip-address of that network.
To see the ip address of the network in your android phone, refer [this](https://www.pttrns.com/what-is-my-wifi-ip-address-on-android/).

This is how it turned out for me...
![epoll's performance](./screenshots/epoll-perf.png)
