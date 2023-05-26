# vpn-try

Just a try to make my own vpn, and play around with sockets in c++.

```
# redirect all traffic to the application
sudo iptables -t nat -A OUTPUT -j REDIRECT -p tcp --to-port 4333

# redirect particular trafic to the application
sudo iptables -t nat -A OUTPUT -j REDIRECT -p tcp --to-port 4333

# clear iptables
sudo iptables -t nat -F
```

## Communication protocol

**Message with network information**

| s_addr        | sin_port      | message                 |
|---------------|---------------|-------------------------|
| 32 bit number | 16 bit number | base68::encode(message) |
| 6 bytes       | 3 bytes       | tbd                     |

**Message without network information**

| message                 |
|-------------------------|
| base68::encode(message) |
