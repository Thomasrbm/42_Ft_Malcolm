# ft_malcolm

> A minimal, educational ARP cache poisoning tool written in C, built as part of the 42 cybersecurity curriculum.

`ft_malcolm` listens for ARP requests on the local network and, upon detecting a matching solicitation, forges an ARP reply that links a chosen IPv4 address to an attacker-controlled MAC address. The victim updates its ARP table with the forged mapping, redirecting subsequent traffic for the spoofed host toward the attacker.

The implementation relies on raw `AF_PACKET` sockets, manually crafted Ethernet and ARP headers, and a custom `libft`-style subset for string and memory primitives — no external dependencies beyond `libc` and the Linux networking stack.

---

## Table of contents

- [Background](#background)
- [Features](#features)
- [Build](#build)
- [Usage](#usage)
- [Options](#options)
- [Examples](#examples)
- [How it works](#how-it-works)
- [Project layout](#project-layout)
- [Detection and mitigation](#detection-and-mitigation)
- [Legal notice](#legal-notice)
- [References](#references)

---

## Background

The Address Resolution Protocol (RFC 826) maps IPv4 addresses to MAC addresses on a local broadcast domain. When host A wants to talk to host B, it broadcasts:

```
ARP Request
  src MAC : aa:aa:aa:aa:aa:aa
  src IP  : 192.168.1.1
  dst MAC : ff:ff:ff:ff:ff:ff   (broadcast)
  dst IP  : 192.168.1.2         ("who has this IP?")
```

Host B replies unicast with its own MAC, and A caches the mapping. ARP has **no authentication**: any host on the segment can answer, and most operating systems will overwrite an existing cache entry with the most recent reply. This is the foundation of ARP cache poisoning — and of `ft_malcolm`.

After a successful spoof:

```
[Victim] ARP table: 192.168.1.1 → cc:cc:cc:cc:cc:cc   (attacker's MAC)
```

The attacker can now intercept, drop, or relay traffic destined for the spoofed IP, enabling man-in-the-middle scenarios.

---

## Features

- Targeted ARP reply spoofing (listens for a specific request before answering)
- Gratuitous ARP mode (`-g`) for unsolicited cache injection
- Verbose mode (`-v`) showing parsed request/reply fields
- Hex dump mode (`-hex`) of the outgoing frame
- Hostname resolution via `getaddrinfo` (accepts IPs *or* hostnames)
- Root privilege check, clean `SIGINT` handling, single global socket
- Strict 42 compilation flags (`-Wall -Wextra -Werror`)
- No external libraries beyond `libc` / Linux headers

---

## Build

```sh
make            # build ./ft_malcolm
make clean      # remove object files
make fclean     # remove objects and binary
make re         # rebuild from scratch
```

Requirements:

- Linux (uses `linux/if_packet.h`, `AF_PACKET`)
- `gcc` and `make`
- Root privileges at runtime (raw sockets)

---

## Usage

```
sudo ./ft_malcolm [-v] [-g] [-hex] <source_ip> <source_mac> <target_ip> <target_mac>
```

| Argument       | Description                                                       |
|----------------|-------------------------------------------------------------------|
| `source_ip`    | IPv4 (or hostname) that you want to **impersonate**               |
| `source_mac`   | MAC address that the victim will associate with `source_ip`       |
| `target_ip`    | IPv4 (or hostname) of the **victim** to poison                    |
| `target_mac`   | True MAC address of the victim                                    |

The program must be run as root (raw `AF_PACKET` sockets require `CAP_NET_RAW`).

---

## Options

| Flag    | Effect                                                                                  |
|---------|-----------------------------------------------------------------------------------------|
| `-v`    | Verbose — print parsed addresses for the incoming request and the outgoing reply        |
| `-g`    | Gratuitous ARP — send an unsolicited reply immediately, without waiting for a request   |
| `-hex`  | Print a hexdump of the forged ARP frame before sending                                  |

Flags must appear **before** the positional arguments.

---

## Examples

Wait for the victim (`192.168.1.50`) to ask for `192.168.1.1`, then answer with our MAC:

```sh
sudo ./ft_malcolm 192.168.1.1 cc:cc:cc:cc:cc:cc 192.168.1.50 bb:bb:bb:bb:bb:bb
```

Same scenario, with verbose output and a hex dump of the reply:

```sh
sudo ./ft_malcolm -v -hex 192.168.1.1 cc:cc:cc:cc:cc:cc 192.168.1.50 bb:bb:bb:bb:bb:bb
```

Gratuitous ARP — push the mapping without waiting for a solicitation:

```sh
sudo ./ft_malcolm -g 192.168.1.1 cc:cc:cc:cc:cc:cc 192.168.1.50 bb:bb:bb:bb:bb:bb
```

Using a hostname instead of an IP:

```sh
sudo ./ft_malcolm gateway.local 08:00:27:90:ea:72 10.11.200.189 08:00:27:8a:35:d2
```

Inspect the victim's ARP cache before and after:

```sh
ip neigh                     # or: arp -n
```

---

## How it works

1. **Argument parsing** (`args.c`, `parse.c`)
   - Flags are consumed until the first non-`-` token.
   - IPs are resolved through `getaddrinfo(AF_INET, …)`, MACs are parsed byte by byte using a custom `ft_strtoul`.

2. **Socket setup** (`network.c`)
   - Opens `socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP))` — a layer-2 socket that sees only ARP frames.
   - Walks `getifaddrs()` to pick the first non-loopback, running interface.
   - Binds to it via `SO_BINDTODEVICE` and resolves its kernel index.

3. **Listening** (`arp.c::receive_arp`)
   - Blocks on `recvfrom` until an ARP frame arrives.
   - Filters on opcode `1` (request), matching `dst_ip == source_ip`, `src_ip == target_ip`, `src_mac == target_mac`.

4. **Forging the reply** (`arp.c::build_reply`)
   - Crafts a 42-byte frame: 14-byte Ethernet header + 28-byte ARP payload.
   - Opcode `2` (reply), hardware type `1` (Ethernet), protocol type `0x0800` (IPv4).
   - `src` fields carry the spoofed identity; `dst` fields target the victim.

5. **Sending** (`arp.c::send_reply`)
   - Fills a `sockaddr_ll` with the interface index and victim MAC, then `sendto()` on the raw socket.

6. **Signal handling** (`main.c`)
   - A `SIGINT` handler closes the global socket cleanly so the program can exit without leaking descriptors.

---

## Project layout

```
ft_Malcolm_42/
├── Makefile
├── ft_malcolm.h        # public header: structs, prototypes, constants
├── main.c              # entry point, signal handling, top-level flow
├── args.c              # flag and argument parsing
├── parse.c             # IP / MAC parsing, address loading
├── network.c           # raw socket setup and interface binding
├── arp.c               # ARP receive, reply construction, sending
├── log.c               # verbose output and hexdump
├── utils.c             # libft-style helpers (memset/memcpy/strcmp/strtoul…)
└── DOC/                # protocol notes and references
```

Key data structures (`ft_malcolm.h`):

```c
struct __attribute__((packed)) ethernet_header {
    uint8_t  dst_mac[6];
    uint8_t  src_mac[6];
    uint16_t ethertype;
};

struct __attribute__((packed)) arp_packet {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_size;
    uint8_t  proto_size;
    uint16_t opcode;
    uint8_t  src_mac[6];
    uint8_t  src_ip[4];
    uint8_t  dst_mac[6];
    uint8_t  dst_ip[4];
};
```

---

## Detection and mitigation

ARP spoofing is loud and well understood. On a defender's side:

- **Static ARP entries** for critical hosts (`arp -s <ip> <mac>`).
- **Dynamic ARP Inspection (DAI)** on managed switches, paired with DHCP snooping.
- **Monitoring**: tools like `arpwatch`, `arping`, or Suricata rules that flag MAC changes for a known IP.
- **Kernel knobs**: `net.ipv4.conf.*.arp_accept`, `arp_ignore`, `arp_announce` tune how aggressively replies are accepted.
- **Segmentation**: 802.1X, VLAN isolation, or simply moving sensitive traffic onto an L3 boundary.

A useful sanity check while testing:

```sh
sudo ip neigh flush all
ping -c 1 <victim>
ip neigh | grep <spoofed_ip>
```

---

## Legal notice

This project is published **strictly for educational purposes** and for use on networks you own or are explicitly authorized to test. Running ARP cache poisoning against systems or networks without the owner's written consent is illegal in most jurisdictions and may constitute a criminal offense.

The authors accept no liability for misuse.

---

## References

- RFC 826 — *An Ethernet Address Resolution Protocol*
- RFC 5227 — *IPv4 Address Conflict Detection* (gratuitous ARP semantics)
- `man 7 packet`, `man 7 netdevice`, `man 3 getifaddrs`
- Linux kernel: `Documentation/networking/arp.rst`
