# ESP32-P4 DRO (Digital Readout) for Milling Machine

[中文版本](./README_CN.md)

This project provides a complete Digital Readout (DRO) system for milling machines using the JC8012P4A1C 10.1-inch ESP32-P4 board with LVGL 9.2 and EEZ Studio integration. It features a modern touch interface for displaying and controlling machine axis positions as a front end communicationg by espnow with ESP32 backend that read the liniar of rotary scales.

**Key Features:**
 - ✅ **Multi-Axis DRO**: Support for virtual axes and combination between axes. Name and interaction of the axes are configurable 
- ✅ **LVGL 9.2**: Fully configured and optimized for 1280x800 display
- ✅ **EEZ Studio Integration**: Ready for visual UI design and development
- ✅ **Modern UI**: Professional DRO interface with touch controls
- ✅ **Hardware Support**: All drivers included (display, touch, audio, sensors)
- ✅ **Memory Optimized**: LVGL memory issues resolved and stable
- ✅ **Self-Contained**: All components included locally, no external dependencies
- ✅ **Thread-Safe**: Proper LVGL locking and task synchronization

## 🎯 **Current Status: Work in progress DRO System do not use it**

### ✅ **Working Features**
- **Physical Display**: 1280x800 MIPI-DSI display with proper backlight control
- **Display Orientation**: Horizontal/landscape mode (270-degree rotation) with proper touch alignment
- **Multi-Axis Display**: Shows X, Y, Z, W, and C axis positions
- **Touch Interface**: Capacitive touch support (GSL3680 controller) with proper coordinate mapping
- **LVGL Display**: Stable display initialization and rendering with thread safety
- **EEZ Studio Ready**: Complete integration framework with working UI screens
- **Memory Stability**: Stable memory usage, no crashes or watchdog timeouts
- **Tab Navigation**: Working tab interface for different DRO functions
- **Axis Controls**: Set zero, set value, and global zero functions

### ✅ **Not Working Features**
- **ESPNOW**: WiFi configuration working but espnow not
- **Display variable and buttons**: Only layout is done. No callbacks will be done untill the UI is finalised since the EEZ will overide them all 

### 🔧 **Hardware Configuration**
- **Main Board**: ESP32-P4 (JC8012P4A1C 10.1-inch board)
- **Display**: 1280x800 MIPI-DSI LCD (JD9365 controller)
- **Touch**: GSL3680 capacitive touch controller
- **Orientation**: 270-degree rotation for proper landscape mode
- **Touch Alignment**: Proper coordinate mapping for rotated display

## 🚀 **Quick Start**

### Prerequisites

* JC8012P4A1C 10.1-inch ESP32-P4 board
* 10.1-inch 1280x800 LCD display with MIPI-DSI interface
* Touch controller (GSL3680 or compatible)
* USB-C cable for power supply and programming
* ESP-IDF v5.4.2 or later (tested with v5.4.2)

### Hardware Setup

1. **Connect Display** via MIPI-DSI interface
2. **Connect Touch Controller** via I2C
3. **Connect USB-C cable** to `USB-UART` port
4. **Power on** the board

### Building and Flashing

```bash
# Build the project
idf.py build

# Flash to ESP32-P4 (adjust port as needed)
idf.py -p /dev/ttyUSB0 flash monitor
```

## 📁 **Project Structure (Self-Contained)**

```
DRO-10.1-inch/
├── components/                        # All local components (self-contained)
│   ├── eez/                          # EEZ Studio integration component
│   │   ├── CMakeLists.txt            # Build configuration
│   │   ├── eez_ui.h                  # Main API header
│   │   ├── eez_ui.c                  # Main implementation
│   │   ├── eez_ui_events.c           # Event handlers
│   │   ├── eez_ui_vars.c             # Variable management for DRO
│   │   ├── README.md                 # Component documentation
│   │   └── generated/                # EEZ Studio exported files
│   │       ├── screens.c             # Generated UI screens
│   │       ├── screens.h             # Screen definitions
│   │       ├── vars.h                # Variable declarations
│   │       ├── actions.h             # Action definitions
│   │       ├── styles.h              # UI styles
│   │       └── ui.h                  # Main UI header
│   ├── esp32_p4_function_ev_board/   # Main BSP for display/touch/hardware
│   ├── esp_lcd_jd9365/               # LCD controller driver (JD9365)
│   ├── esp_lcd_touch_gsl3680/        # Touch controller driver (GSL3680)
│   ├── bsp_extra/                    # Additional BSP utilities
│   ├── esp_cam_sensor/               # Camera sensor support
│   ├── esp_video/                    # Video processing
│   └── espressif__esp_lcd_jd9165/    # Additional LCD support
├── main/
│   └── main.c                        # Main application with DRO functionality
├── managed_components/               # LVGL and driver components
├── sdkconfig.defaults               # Project configuration
├── partitions.csv                   # Flash partition layout
├── dependencies.lock                # Local component dependencies
├── EEZ_STUDIO_INTEGRATION.md        # Complete EEZ Studio guide
└── README.md                        # This file
```

**Note**: This project is fully self-contained with all hardware drivers and BSP components included locally.

## 🔧 **Technical Implementation Details**

### Display System
- **Controller**: JD9365 MIPI-DSI LCD controller
- **Resolution**: 1280x800 pixels in horizontal orientation
- **Backlight**: Automatic backlight control with `bsp_display_backlight_on()`
- **Rotation**: 270-degree rotation for landscape mode with proper touch alignment
- **Thread Safety**: Proper LVGL locking (`lvgl_port_lock/unlock`) for UI operations

### Touch System
- **Controller**: GSL3680 capacitive touch controller
- **Alignment**: Proper coordinate mapping for 270-degree rotated display
- **Mirror Settings**: `mirror_x = 1, mirror_y = 0` for correct alignment
- **Response**: All buttons and tabs respond correctly to touch input

### LVGL Integration
- **Version**: LVGL 9.2 with optimized memory configuration
- **Memory**: Stable memory usage with proper allocation
- **Threading**: Separate UI creation task with proper synchronization
- **Timing**: Proper initialization sequence with delays
- **Stability**: No crashes or watchdog timeouts

### DRO System
- **Axes**: X, Y, Z, W, and C axis support
- **Display**: Real-time position display with 4-decimal precision
- **Controls**: Set zero, set value, and global zero functions
- **Units**: Millimeter display with inch/mm toggle capability
- **Tools**: Tool number and work offset management

## 🎨 **EEZ Studio Integration**

### Ready for Visual Development

The project includes a complete EEZ Studio integration framework:

```c
// Basic EEZ Studio usage
#include "ui.h"

void app_main(void)
{
    // Initialize display and LVGL
    app_display_init();
    app_lvgl_init();
    
    // Create UI screens (in separate task for thread safety)
    xTaskCreate(ui_creation_task, "ui_creation", 4096, NULL, 5, NULL);
    
    // Create UI tick task for updates
    xTaskCreate(ui_tick_task, "ui_tick", 4096, NULL, 3, NULL);
}
```

### EEZ Studio Workflow

1. **Design UI** in EEZ Studio
2. **Export as C code** for LVGL
3. **Copy files** to `components/eez/generated/`
4. **Update CMakeLists.txt** with generated files
5. **Build and test**

See [EEZ_STUDIO_INTEGRATION.md](./EEZ_STUDIO_INTEGRATION.md) for complete guide.

## 📋 **DRO Features**

### Axis Management
- **X Axis**: Primary horizontal axis
- **Y Axis**: Secondary horizontal axis  
- **Z Axis**: Vertical axis
- **W Axis**: Additional horizontal axis
- **C Axis**: Rotary axis

### Functions
- **Position Display**: Real-time position with 4-decimal precision
- **Set Zero**: Set current position as zero for any axis
- **Set Value**: Set specific position value for any axis
- **Global Zero**: Set all axes to zero simultaneously
- **Tool Management**: Tool number and work offset handling
- **Unit Toggle**: Switch between metric (mm) and imperial (inch)

### UI Components
- **Tab Interface**: Multiple tabs for different DRO functions
- **Numeric Keypad**: Touch keypad for value entry
- **Status Display**: Information and status messages
- **Settings**: Configuration and calibration options

## 🛠️ **Development**

### Custom Development

To extend the DRO application:

1. **Modify `main/main.c`** for application logic
2. **Use EEZ Studio** for UI design
3. **Extend `components/eez/`** for additional features

## 📦 **Project Moving Guide**

When moving this project to a different location, you may encounter configuration errors related to manifest files. See [PROJECT_MOVING_GUIDE.md](./PROJECT_MOVING_GUIDE.md) for complete instructions.

**Quick fix:**
```bash
# Run the automated fix script
python fix_manifest_paths.py --backup

# Test the configuration
idf.py reconfigure
```
4. **Add axis input** from encoders or sensors
5. **Implement communication** with machine control systems

### Configuration

```bash
# Open configuration menu
idf.py menuconfig
```

Key configuration sections:
- `Component config > LVGL` - LVGL settings
- `Component config > Board Support Package(ESP32-P4)` - Hardware settings
- `Component config > Display` - Display and touch settings

### Example: Adding Axis Input

```c
// Example: Read encoder input for X axis
void read_x_axis_encoder(void)
{
    // Read encoder value
    int32_t encoder_value = read_encoder(ENCODER_X);
    
    // Convert to position (mm)
    float position_mm = encoder_value * ENCODER_RESOLUTION_MM;
    
    // Update DRO variable
    set_var_virtual_axis_1(position_mm);
}
```

## 🔍 **Troubleshooting**

### Common Issues (All Resolved in Current Version)

1. **✅ Display Working**:
   - MIPI-DSI connections verified
   - Display backlight automatically enabled
   - LVGL configuration optimized
   - 270-degree rotation properly configured

2. **✅ Touch Working**:
   - GSL3680 controller properly initialized
   - Touch coordinates aligned with display rotation
   - All buttons and tabs respond correctly
   - Proper mirror settings applied

3. **✅ Build Success**:
   - ESP-IDF v5.4.2 compatibility verified
   - All component dependencies included locally
   - EEZ Studio integration working

4. **✅ Memory Stable**:
   - Stable memory usage maintained
   - LVGL memory configuration optimized
   - No memory leaks or crashes

### System Monitoring

The project includes built-in system monitoring:

```c
// System monitoring output (every 10 seconds)
I (12345) SYSTEM_MONITOR: Free heap: 30123456, Min free: 29876543, Largest block: 8388608
```

### Debug Tips

```c
// Memory monitoring
ESP_LOGI(TAG, "Free heap: %lu", esp_get_free_heap_size());

// Display status
ESP_LOGI(TAG, "Display initialized, backlight on");

// Touch status
ESP_LOGI(TAG, "Touch input device initialized successfully");
```

## 📚 **Resources**

- **EEZ Studio**: [https://eezstudio.org/](https://eezstudio.org/)
- **LVGL Documentation**: [https://docs.lvgl.io/](https://docs.lvgl.io/)
- **ESP-IDF**: [https://docs.espressif.com/projects/esp-idf/](https://docs.espressif.com/projects/esp-idf/)
- **ESP-BSP**: [https://github.com/espressif/esp-bsp](https://github.com/espressif/esp-bsp)

## 🎉 **Success Criteria (All Met)**

Your DRO setup is working correctly when:

✅ **Display shows** DRO interface in correct landscape orientation
✅ **Touch interface** responds to all buttons and tabs
✅ **Axis displays** show position values correctly
✅ **Tab navigation** works with both touch and swipe
✅ **EEZ Studio integration** works for UI development
✅ **Memory usage** is stable
✅ **System stability** with no crashes or timeouts
✅ **Clean console** output for better debugging

## 🚀 **Next Steps**

1. **Connect encoders** for real axis position input
2. **Add machine communication** (Modbus, Ethernet, etc.)
3. **Implement advanced features** (tool compensation, work offsets)
4. **Add data logging** and history functions
5. **Extend with additional** sensors and peripherals
6. **Add network connectivity** for remote monitoring

## 📄 **License**

This project follows the same license terms as the original ESP-IDF examples.

---
