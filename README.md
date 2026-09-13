# fpp-plugin-tplink
FPP TP-Link(Kasa) Plugin

Add Plugin URL to FPP
https://raw.githubusercontent.com/computergeek1507/fpp-plugin-tplink/main/pluginInfo.json

## Switching plugs from a sequence

A start channel means the sequence owns the plug: when a playlist starts, the plug is sent the channel's state on the very first frame (so a channel at 0 turns it off right away, not 30 seconds in). After that the plugin sends a command only when the on/off state changes (127 or more is on, below 127 is off). There is no periodic re-send. Leave the start channel at 0 for plugs you control with commands (playlist lead-ins, Home Assistant).
