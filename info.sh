#!/bin/bash
TARGET_IP=$(grep "Request" /tmp/arp_capture.txt | awk '{print $10}' | sort | uniq -c | sort -rn | head -1 | awk '{print $2}')
VICTIM_IP=$(grep "Request" /tmp/arp_capture.txt | grep "$TARGET_IP" | awk '{print $13}' | head -1)
VICTIM_MAC=$(grep "Request" /tmp/arp_capture.txt | grep "$TARGET_IP" | awk '{print $2}' | head -1)

echo "sudo ./ft_malcolm $TARGET_IP aa:bb:cc:dd:ee:ff $VICTIM_IP $VICTIM_MAC"