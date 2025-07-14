# EEZ Studio Component

This component provides integration between EEZ Studio generated LVGL UI files and the ESP32-P4 WiFi Scanner application.

## Directory Structure

```
components/eez/
├── CMakeLists.txt          # Component build configuration
├── eez_ui.h               # Main header file with API
├── eez_ui.c               # Main implementation
├── eez_ui_events.c        # Event handlers for EEZ Studio
├── README.md              # This file
└── generated/             # Place EEZ Studio generated files here
    ├── ui.c               # Generated UI code
    ├── ui.h               # Generated UI headers
    ├── assets.c           # Generated assets (images, fonts)
    └── assets.h           # Generated assets headers
```

## Usage

### 1. Basic Integration

```c
#include "eez_ui.h"

void app_main(void)
{
    // Initialize EEZ UI
    eez_ui_config_t config = EEZ_UI_CONFIG_DEFAULT();
    eez_ui_init(&config);
    
    // Create the UI screen
    eez_ui_create_screen();
    
    // Register callbacks
    eez_ui_register_scan_callback(wifi_scan_callback);
    eez_ui_register_network_callback(network_selected_callback);
}
```

### 2. WiFi Integration

```c
// Update WiFi list when scan completes
void wifi_scan_complete(wifi_ap_record_t *networks, uint16_t count)
{
    eez_ui_update_wifi_list(networks, count);
}

// Set status messages
void update_status(const char *message)
{
    eez_ui_set_status(message);
}
```

### 3. Event Callbacks

```c
void wifi_scan_callback(void)
{
    ESP_LOGI(TAG, "Starting WiFi scan...");
    esp_wifi_scan_start(NULL, false);
}

void network_selected_callback(const char *ssid)
{
    ESP_LOGI(TAG, "Selected network: %s", ssid);
    // Handle network selection
}
```

## EEZ Studio Integration

### 1. Export from EEZ Studio

1. Design your UI in EEZ Studio
2. Export as C code for LVGL
3. Copy generated files to `components/eez/generated/`

### 2. Update CMakeLists.txt

Add your generated files to `components/eez/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS 
        "eez_ui.c"
        "eez_ui_events.c"
        "generated/ui.c"           # Add your generated files
        "generated/assets.c"
    INCLUDE_DIRS 
        "."
        "generated"                # Add generated headers
    REQUIRES 
        lvgl
        esp-bsp
)
```

### 3. Integration Points

The EEZ component provides these integration points:

- **Screen Management**: `eez_ui_create_screen()` creates the main screen
- **WiFi Updates**: `eez_ui_update_wifi_list()` updates network list
- **Status Updates**: `eez_ui_set_status()` shows status messages
- **Event Handling**: Register callbacks for user interactions
- **Asset Management**: Generated assets are automatically included

### 4. Customization

You can customize the UI by:

1. **Modifying `eez_ui.c`**: Change the default UI layout
2. **Adding EEZ Studio files**: Drop generated files in `generated/` folder
3. **Extending callbacks**: Add more event handlers as needed
4. **Styling**: Modify colors, fonts, and layouts

## API Reference

### Functions

- `eez_ui_init()` - Initialize the EEZ UI system
- `eez_ui_create_screen()` - Create the main UI screen
- `eez_ui_update_wifi_list()` - Update WiFi network list
- `eez_ui_set_status()` - Set status message
- `eez_ui_register_scan_callback()` - Register scan button callback
- `eez_ui_register_network_callback()` - Register network selection callback
- `eez_ui_deinit()` - Clean up resources

### Configuration

```c
typedef struct {
    lv_obj_t *screen;           // Main screen object
    lv_obj_t *container;        // Main container
    bool auto_scan;             // Enable automatic WiFi scanning
    uint32_t scan_interval_ms;  // Scan interval in milliseconds
} eez_ui_config_t;
```

## Example Workflow

1. **Design UI in EEZ Studio**
   - Create your WiFi scanner interface
   - Add buttons, labels, and network list
   - Export as C code

2. **Copy Generated Files**
   ```bash
   cp /path/to/eez/export/* components/eez/generated/
   ```

3. **Update CMakeLists.txt**
   - Add generated files to SRCS
   - Add generated directory to INCLUDE_DIRS

4. **Build and Test**
   ```bash
   idf.py build
   idf.py flash monitor
   ```

## Troubleshooting

### Common Issues

1. **Missing generated files**: Ensure all EEZ Studio files are in `generated/` folder
2. **Build errors**: Check that all generated files are listed in CMakeLists.txt
3. **UI not showing**: Verify `eez_ui_create_screen()` is called after display initialization
4. **Events not working**: Check that callbacks are properly registered

### Debug Tips

- Enable debug logging: `esp_log_level_set("EEZ_UI", ESP_LOG_DEBUG)`
- Check LVGL memory usage
- Verify screen objects are created successfully
- Monitor event callbacks in console output 