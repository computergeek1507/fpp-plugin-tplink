# fpp-plugin-tplink
FPP TP-Link(Kasa) Plugin

Add Plugin URL to FPP
https://raw.githubusercontent.com/computergeek1507/fpp-plugin-tplink/main/pluginInfo.json

## Switching plugs from a sequence

Give a plug a start channel and the sequence switches it on and off by crossing half (127 or more is on, below 127 is off). The plugin sends a command only when the on/off state changes, and it never turns off a plug the sequence has not turned on, so a plug switched on by a command or Home Assistant stays on while a sequence leaves its channel at 0. A start channel of 0 means sequences never touch the plug.
