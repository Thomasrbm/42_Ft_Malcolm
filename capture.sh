#!/bin/bash
INTERFACE=$(ip route | grep default | awk '{print $5}' | head -1)
sudo tcpdump -i enp0s3 arp -n -e > /tmp/arp_capture.txt