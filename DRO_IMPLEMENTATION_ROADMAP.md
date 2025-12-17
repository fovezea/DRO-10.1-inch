# DRO Implementation Roadmap

This document outlines the plan to implement the core Digital Readout (DRO) functionality and the Tool Table system for the ESP32-P4 project.

## 🎯 Objectives
1.  **Full DRO Functionality**: Implement logic for all buttons on the main screen (Zero, Set, Abs/Inc, Units, etc.).
2.  **Tool Table**: Implement a 200-tool library with NVS (Non-Volatile Storage) persistence.
3.  **UI Integration**: Connect the backend logic to the existing EEZ Studio generated UI.

---

## 🏗️ Architecture Overview

We will create a new component `components/dro_system` to encapsulate the business logic. This ensures a clean separation between the UI (LVGL/EEZ) and the core DRO calculations.

### Data Structures
*   **`DROState`**: Holds the runtime state (Current Mode, Active Units, Selected Axis).
*   **`AxisState`**: Holds data for each of the 5 axes (Raw Position, Relative Offset, Blink Status).
*   **`ToolData`**: Holds data for a single tool (Offsets for X/Y/Z, Name/Description).

---

## 📅 Implementation Phases

### Phase 1: Core DRO Logic (The Backend)
*Goal: Create the "Brain" of the DRO that knows how to calculate positions.*

1.  **Create `components/dro_system`**:
    *   `dro_core.c/.h`: Main logic definitions.
    *   `dro_nvs.c/.h`: Storage management.
2.  **Implement State Management**:
    *   **Units**: Conversion toggling between MM (1.0) and Inch (1/25.4).
    *   **Modes**: Toggling between Absolute (Machine Zero) and Incremental (Work Zero).
    *   **Axis Calculations**: `DisplayPos = (RawPos - WorkOffset - ToolOffset) * UnitScale`.
3.  **Implement Axis Actions**:
    *   `dro_axis_zero(axis_index)`
    *   `dro_axis_set_value(axis_index, value)`
    *   `dro_axis_half(axis_index)`

### Phase 2: Tool Table Verification & NVS
*Goal: Persistent storage for 200 tools.*

1.  **Define `ToolData` Struct**:
    ```c
    typedef struct {
        float offsets[5]; // Offsets for X, Y, Z, W, C
        char name[16];    // Short description
        uint8_t type;     // Drill, Lathe, Endmill, etc.
    } ToolData;
    ```
2.  **NVS Implementation**:
    *   **Key Namespaces**: Use efficient referencing (e.g., `tool_0`, `tool_1`... or a binary blob).
    *   **Caching**: Load the table into RAM on boot for fast access; save to NVS only on modification.
3.  **API**:
    *   `dro_tool_get(index)`
    *   `dro_tool_set(index, data)`
    *   `dro_tool_apply(index)`: Sets the current active tool offsets.

### Phase 3: UI Integration (The Frontend)
*Goal: Make the buttons on the screen actually do things.*

1.  **Event Glue Code (`dro_driver.c`)**:
    *   Map LVGL events from `objects.set_zero_axisX_button` to `dro_axis_zero(X)`.
    *   Map `toggle_inch_mm_button` to `dro_toggle_units()`.
    *   Map `toggle_abs_incr_button` to `dro_toggle_mode()`.
2.  **Numpad Integration**:
    *   When `Set Value` is clicked: Open Numpad -> Wait for Result -> Call `dro_axis_set_value`.
3.  **Display Loop**:
    *   Update `virtual_axis_X` variables in `eez_ui_vars.c` every tick based on the calculated `dro_core` state.
    *   Update Status Labels (e.g., "MM" vs "INCH", "ABS" vs "INC").

### Phase 4: Additional Button Logic
1.  **Half Function**:
    *   Logic: `WorkOffset = CurrentRawPos - (CurrentRelPos / 2)`.
2.  **Tool Selection**:
    *   Click `Tool Number` button -> Enter ID -> Call `dro_tool_apply(ID)`.

---

## 🚀 Next Steps (Execution Plan)

1.  **[ ] Create Component**: setup `components/dro_system`.
2.  **[ ] Define Headers**: Write `dro_core.h` with the API.
3.  **[ ] Implement Tool Table**: Write the NVS logic.
4.  **[ ] Wire UI**: Connect the buttons in `main.c` or a dedicated `ui_driver.c`.

---
