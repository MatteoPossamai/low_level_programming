# Wireshark

## Possible TCP packets type

• `SYN`: Synchronize
• `ACK`: Acknowledge
• `FIN`: Finish
• `RST`: Reset
• `PSH`: Push
• `URG`: Urgent
• `ECE`: Echo
• `CWR`: Reduced
• `NS`: Nonce

## Panes

- Top: captured packets, one per row
- Middle: all network info of the packet selected
- Bottom: hex of the packet selected

## Display filters

Type in the bar above the packet list, then press Enter. No quotes.
Clear the bar and press Enter to show everything again. Only hides packets;
does not remove them from the capture.

- `tcp.port == 8080`: source or destination port 8080
- `tcp.len > 0`: TCP packets carrying data
- `tcp.flags.syn == 1`: connection setup
- `tcp.flags.fin == 1`: connection shutdown
- `tcp.flags.reset == 1`: connection resets
- `tcp.stream == 0`: first TCP connection in the capture
- `tcp.port == 8080 && tcp.len > 0`: combine conditions with AND

## Notes about protocols

- If you send packets between processes locally, then the
  data link layer is going to report `Linux cooked data`
  given that there is no Ethernet
- `IPv4` and `TCP` do not have a footer, all in header
- TCP sequence and acknowledgement numbers:
  - `seq`: first byte in the packet
  - `ack`: next expected byte from other party
- So `TCP` takes care of bytes, not of messages
  (that are called datagram, then UDP)
- `SYN` and `FIN` each consume one sequence number, even with empty payload
- Boundaries that are not aligned (possible chunking). `TCP` maintains order, not the structure. The stream in the end is not distinguishable, that is the promise:
  - `write`
  - `TCP` packets
  - `recv`
- Normal three way handshake for setup and close:
  - `SYN`, `SYN, ACK`, `ACK` setup
  - `FIN`, `FIN, ACK`, `ACK` setup
- OS stores time stamps with `us` precision
- Different times:
  - Frame -> Arrival time: when OS got packet
  - Time since first frame
  - Time delta from previous
  - Optionals...

## Checksum offloading

The OS can delegate checksum calculation to the network card. A capture may
record the packet before the card finishes it, so Wireshark can report an
incorrect or incomplete checksum even when the transmitted packet is correct.
A checksum warning alone does not prove corrupted traffic.

## `tcpdump`

Might require `sudo`.

Example:

```shell
sudo tcpdump -i lo -nn -s 0 -w capture.pcap 'tcp port 8080'
```

### Flags

[Complete manual](https://www.tcpdump.org/manpages/tcpdump.1.html)

- `-i`: interface. `any` for all, `lo` for loopback (locahost)
- `-nn`: numeric IP addresses, no resolve
- `-s`: capture up to. With `0` use default maximum packet size
- `-w`: write to a file. Just add the name of file
- `'tcp port 8080'`: fixes the port to look, `src` or `dst`
- `'tcp src/dst port 8080'`: fixes the port to look, only for `src` or `dst` based on setting
- `-D`: list available interfaces
- `-c`: after how many packets to stop
- `-r`: read existing file
- `-A`: packet bytes as text without link header
- `-X`: packet bytes as hex without link header
- `-vv`: verbose
- `tttt`: print date and time for each packet
