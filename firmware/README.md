# Co-processor firmware

The ESP32-P4 has no radio. On the osptek board the Wi-Fi/BT co-processor is an
**ESP32-C5**, and it ships with a blank flash, so one of these has to be written before the
hosted link exists at all.

| Image | Target | ESP-Hosted | Board |
|---|---|---|---|
| `ESP32-C5_ESP-Hosted_v3.1.0_merged_0x0.bin` | ESP32-C5 | 3.1.0 | osptek (DRO_BOARD=osptek) |

Merged image: bootloader + partition table + otadata + app, written in one go at **offset
0x0**. It is built in the HMI project (`HMI-10.1-esp32-p4c6/esp_hosted_cp`) and copied here
because the repos stay separate; rebuild it there, not here.

The Guition board needs no image from this folder: its ESP32-C6 co-processor is built from
`esp32c6_hosted_cp/` in this repo.

## Flashing

1. Slide the UART switch down to the **C5**; connect the CH343P debug port.
2. Enter download mode **by hand**: hold **BOOT (K2)**, tap **RST (K1)**, release BOOT.
   The switch carries UART TX/RX only, so DTR/RTS never reach the C5 and esptool cannot
   reset it into download mode itself.
3. Flash with the reset suppressed:

   ```powershell
   python -m esptool --chip esp32c5 -p COM4 -b 460800 --before no-reset --after no-reset `
     write_flash 0x0 firmware/ESP32-C5_ESP-Hosted_v3.1.0_merged_0x0.bin
   ```

4. Tap **RST** to boot it, then slide the switch back to the P4.

A blank C5 boots to `invalid header: 0xffffffff` x3, an `ets_flash_boot` assertion and then a
watchdog reset loop -- that is "no firmware", not a faulty chip.

Success looks like this on the host:

```
eh_init_evt: slave chip id: 0x17 (esp32c5)
eh_init_evt: esp-hosted fw versions: host=3.1.0 coprocessor=3.1.0 (match)
```
