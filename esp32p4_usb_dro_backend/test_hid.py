import hid
import struct
import time

# Espressif Default VID and PID
VID = 0x303a
PID = 0x4004

# Endpoint Report IDs (Must match usb_reports.h)
REPORT_ID_INPUT = 1
REPORT_ID_OUTPUT = 2

# Struct formats based on __attribute__((packed)) from C
# Input Report: uint8_t report_id, uint32_t status_flags, int32_t axis[5]
# '<' = Little Endian, 'B' = unsigned char, 'I' = unsigned int, '5i' = 5 integers
INPUT_FORMAT = '<B I 5i'

# Output Report: uint8_t report_id, float mult_axis[5], uint8_t track_spindle[5], uint8_t command_code
# '<' = Little Endian, 'B' = unsigned char, '5f' = 5 floats, '5B' = 5 unsigned chars, 'B' = unsigned char
OUTPUT_FORMAT = '<B 5f 5B B'

def test_hid():
    print(f"Searching for USB Custom HID Device (VID {hex(VID)}, PID {hex(PID)})...")
    
    # Try looking for our exact combination
    device = hid.device()
    try:
        device.open(VID, PID)
        print("Successfully opened device!")
    except:
        print("Could not open device. It might not be plugged in, flushed, or VID/PID might differ.")
        print("Available HID devices on your system:")
        for d in hid.enumerate():
            print(f" - Vendor: {hex(d['vendor_id'])}, Product: {hex(d['product_id'])}, {d['product_string']}")
        return

    # Set non-blocking mode so we can send/receive in a loop without hanging
    device.set_nonblocking(True)

    try:
        # Let's send a setup output report to the board:
        # Mapping Axis 0 to Spindle with 1.5 multiplier, Axis 1 with 0.5 multiplier
        # 1 byte ID, 5 floats (mult), 5 bools (track), 1 byte command
        out_buf = struct.pack(OUTPUT_FORMAT, 
            REPORT_ID_OUTPUT, 
            1.5, 0.5, 1.0, 1.0, 1.0,  # mult_axis
            1, 1, 0, 0, 0,            # track_spindle
            1                         # command_code (1)
        )
        print(f"Sending Setup Report (Len {len(out_buf)})...")
        # For writing, the first byte of the array must be the Report ID if using write()
        device.write(out_buf)

        print("\n--- Listening to Encoders from ESP32-P4 Backend ---")
        start_time = time.time()
        
        while True:
            # Read back our INPUT_FORMAT (25 bytes)
            # We request 64 bytes (the standard HID endpoint size)
            data = device.read(64)
            if data:
                if data[0] == REPORT_ID_INPUT:
                    # Pad or slice the data to perfectly match our struct if needed, 
                    # but unpack needs exact bytes. (25 bytes)
                    # hid.read sometimes returns the array without report ID on some OSs,
                    # but typically standard is [Report ID, Byte 1, ... Byte N]
                    
                    try:
                        # Unpack exactly the length of our format (25 bytes)
                        unpacked = struct.unpack(INPUT_FORMAT, bytearray(data[:25]))
                        report_id = unpacked[0]
                        status = unpacked[1]
                        axes = unpacked[2:7]

                        print(f"Axes: {axes} | Status: {status}")
                    except Exception as e:
                        print(f"Data unpack error: {e}. Received bytes: {data}")
            
            time.sleep(0.01) # Sleep 10ms to match our 100Hz max loop

    except KeyboardInterrupt:
        print("\nTest terminated by user.")
    finally:
        device.close()

if __name__ == "__main__":
    test_hid()
