# vpn-try

Just a try to make my own vpn, and play around with sockets in c++.

```
# redirect all traffic to the application
sudo iptables -t nat -A OUTPUT -j REDIRECT -p tcp --to-port 4333 -m owner ! --uid-owner root

# redirect google traffic to the application
sudo iptables -t nat -A OUTPUT -p tcp -d google.com --dport 80 -j REDIRECT --to-port 4333 -m owner ! --uid-owner root 
sudo iptables -t nat -A OUTPUT -p tcp -d google.com --dport 443 -j REDIRECT --to-port 4333 -m owner ! --uid-owner root 

# list iptables rules created
sudo iptables -t nat -L --line-number

# remove a particular iptable rule
sudo iptables -t nat -D OUTPUT <line-num>

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

## ToDo

- [ ] Create scripts to handle iptables
  1. Create iptables rules to redirect traffic to the application
  2. Remove previously created iptables rules
