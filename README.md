# fpp-plugin-tplink
FPP TP-Link(Kasa) Plugin

Add Plugin URL to FPP
https://raw.githubusercontent.com/computergeek1507/fpp-plugin-tplink/main/pluginInfo.json

## Switching plugs from a sequence

A start channel means the sequence owns the plug. When a playlist starts, the plug is sent the channel's state on the very first frame (so a channel at 0 turns it off right away, not 30 seconds in). After that the plugin sends a command only when the on/off state changes.

In more detail, for TPLink Switch/Plug, Tasmota Switch and Tapo Switch entries:

- A channel value of 127 or more means on. Below 127 means off.
- The plugin sends a command only when the on/off state changes. Holding the channel steady sends nothing more.
- When a playlist starts, the first frame always sends, so a channel at 0 turns the plug off immediately. After that first send, only changes are acted on.
- Each plug has one sender, so the sequence's commands to a plug never overlap and always arrive in order. If a plug does not answer, the plugin retries the latest state for up to 2 minutes, waiting longer each time (up to 8 seconds between tries), then gives up and says so in the log. A newer state from the sequence replaces the one being retried, is sent straight away, and gets 2 minutes of its own.
- Leave the start channel at 0 for plugs you control with commands (playlist lead-ins, Home Assistant).

The commands (TPLink Set Switch, TPLink Toggle Switch, TPLink All Switches On, Off and Toggle) work as before.

## Tapo devices

Tapo plugs, power strips and bulbs are controlled directly over the local network (TP-Link's KLAP protocol on port 80), with no cloud and no Python. Commands take a few tens of milliseconds once a session is open; the session is kept and reused for hours.

- Enter your Tapo account (the email and password you use in the Tapo app) once, under **Tapo Account** on the plugin's settings page. Every Tapo device uses it.
- The password itself is not stored. The plugin keeps only the hash of the email and password that the devices check during login (`TapoAuthV1`/`TapoAuthV2`), so the Tapo account password never sits on the FPP box or in its backups. The hash still lets FPP control your devices on the local network, so treat backups as you would any device credential.
- Newer Tapo firmware only allows local control after you turn on **Third-Party Compatibility** in the Tapo app (Me → Third-Party Services).
- For a power strip, set the Plug ID to the outlet number (1, 2, 3...). Plug ID 0 switches the whole strip.
- Devices on old firmware that predates KLAP are not supported; update their firmware in the Tapo app.

## Tests

The flip rule, the per-plug sender and the Kasa protocol code live in `src/core` and build without FPP. `make -C tests` builds and runs their unit tests on macOS or Linux, against a fake plug on 127.0.0.1.

`tests/run_tapo_tests.sh` builds the native Tapo client tests and runs them against `tests/fake_tapo.py`, a fake Tapo device whose KLAP side is python-kasa's own code (`pip install python-kasa` first).
