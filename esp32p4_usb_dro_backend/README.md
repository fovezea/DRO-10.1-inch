# ESP32-P4 Electronic Lead Screw (ELS) Architecture

This repository contains the boilerplate for a bidirectional USB communication test, but more importantly, it models the **DDS (Direct Digital Synthesis) Phase Accumulator Architecture** for an Electronic Lead Screw.

## Why not Bresenham?
Bresenham's line algorithm is famous for CNC/3D printing but has two major ELS drawbacks:
1. **It's built for division, not multiplication.** It struggles when your thread pitch requires spinning the Z-axis faster than the Spindle (a ratio > 1).
2. **Interrupt Overhead.** If you have a 10,000 PPR (Pulses Per Revolution) encoder and you spin at 1000 RPM, the encoder emits over **166,000 pulses every second**. If you try to run an interrupt and Bresenham math on each pulse, the ESP32 CPU will lock up.

## The Phase Accumulator (DDS) Approach
Instead of acting on every pulse individually, we act on **time intervals**, decoupling the high-speed input from the math.

### 1. High-Speed Hardware Input (PCNT)
The ESP32 has a dedicated **Pulse Counter (PCNT)** peripheral. You route the A/B quadrature signals from the 10k PPR encoder directly into the PCNT hardware. 
- The PCNT tracks rotation completely in silicon. It does not use the CPU.
- Our control loop runs at a fixed interval (e.g. 5ms or 200Hz).
- Every 5ms, the CPU wakes up, reads the PCNT to see how many pulses passed (e.g., `+500 pulses`), and then resets the counter. 

### 2. The 32-bit Phase Accumulator
For each axis tracking the spindle, multiplying the spindle speed is just floating-point math converted to fixed-point for speed:

```c
// Ratio 1.5x converted to Q16.16 fixed-point (1.5 * 65536)
int32_t fixed_ratio = 98304; 
phase_accumulator[X] += (encoder_delta * fixed_ratio);
```

Every time the `phase_accumulator` rolls over 1.0 (`65536` in fixed point), we know we need to step the motor.
```c
int32_t steps_to_fire = phase_accumulator[X] / 65536;
phase_accumulator[X] -= (steps_to_fire * 65536); // Keep the remainder!
```

### 3. Smooth Hardware Output (MCPWM / RMT)
Now we know we need to fire `steps_to_fire` (e.g., 75 steps) before the next 5ms loop.
Instead of doing a `for` loop with `delayMicroseconds()`, you hand the number `75` to the ESP32's **RMT** or **MCPWM** hardware.
- The hardware will perfectly distribute the 75 steps across the 5ms window.
- The stepper motor sees a perfectly smooth, jitter-free frequency.
- The CPU goes back to sleep or handles USB communications!

This architecture guarantees true multiplication ratios, handles extreme PPRs without sweating, and gives your hardware perfectly smooth motion.
