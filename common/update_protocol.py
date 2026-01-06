# common/update_protocol.py
import re
import os
import sys

# Configuration
INPUT_HEADER = "protocol_defs.h"
# Target output relative to this script location if running from 'common' 
# OR relative to CWD if running from root. 
# We'll assume the script is run from the 'common' directory or we find paths relative to it.

# Output path: E-leadscrew/src/generated/protocol.vh
# We will resolve this path relative to the script's location.
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT_VERILOG = os.path.join(SCRIPT_DIR, "..", "E-leadscrew", "src", "generated", "protocol.vh")
INPUT_PATH = os.path.join(SCRIPT_DIR, INPUT_HEADER)

def generate_verilog():
    if not os.path.exists(INPUT_PATH):
        print(f"Error: Input file {INPUT_PATH} not found.")
        sys.exit(1)

    print(f"Reading from: {INPUT_PATH}")
    with open(INPUT_PATH, 'r') as f:
        c_content = f.read()

    verilog_lines = [
        "// AUTOMATICALLY GENERATED - DO NOT EDIT",
        "// Source: common/protocol_defs.h",
        ""
    ]

    # Regex to find #define NAME VALUE
    # Handles: 
    # #define NAME 0x55
    # #define NAME 123
    defines = re.findall(r'#define\s+(\w+)\s+(0x[0-9A-Fa-f]+|\d+)', c_content)

    for name, value in defines:
        if value.startswith("0x"):
            # Clean hex string
            hex_val = value.replace("0x", "")
            # Assume 8-bit for hex values found in this specific protocol context if small
            # But safer to just use 'h syntax without width for general cases or 
            # if we know they are bytes (like Opcodes) we can be explicit.
            # Verilog `define is text substitution.
            # `define CMD_SET_RATIO 8'h55 is safer for comparisons.
            
            # Simple heuristic: if it looks like a byte (<= FF), make it 8'h
            val_int = int(hex_val, 16)
            if val_int <= 255:
                verilog_line = f"`define {name} 8'h{hex_val}"
            else:
                verilog_line = f"`define {name} 'h{hex_val}"
        else:
            verilog_line = f"`define {name} {value}"
            
        verilog_lines.append(verilog_line)
        print(f"  Defined: {name} -> {verilog_line.split(' ')[-1]}")

    # Ensure output directory exists
    output_dir = os.path.dirname(OUTPUT_VERILOG)
    os.makedirs(output_dir, exist_ok=True)

    with open(OUTPUT_VERILOG, 'w') as f:
        f.write("\n".join(verilog_lines))
    
    print(f"Success: Generated {OUTPUT_VERILOG}")

if __name__ == "__main__":
    generate_verilog()
