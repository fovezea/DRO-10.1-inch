# HMI-Centric DRO Implementation Roadmap

This roadmap defines the development of the **Digital Readout (DRO) HMI**. The design philosophy is strictly **"Smart Client, Dumb Backend"**.

<<<<<<< HEAD
## 🎯 Objectives

1. **Full DRO Functionality**: Implement logic for all buttons on the main screen (Zero, Set, Abs/Inc, Units, etc.).
2. **Tool Table**: Implement a 200-tool library with NVS (Non-Volatile Storage) persistence.
3. **UI Integration**: Connect the backend logic to the existing EEZ Studio generated UI.
=======
## 🏗️ System Architecture

### 1. The HMI (Frontend) - *The "Brain"*
*   **Role**: Contains **ALL** application logic, mathematics, and user interaction flows.
*   **Responsibilities**:
    *   Coordinate System Math (ABS/INC, Work Offsets).
    *   Geometric Calculations (PCD, Arcs, Tapers).
    *   Unit Conversion & Display Formatting.
    *   Data Persistence (Tool Tables, Calibration data).

### 2. The Backend (ESP32) - *The "Bridge"*
*   **Role**: A passive data pipe.
*   **Responsibilities**:
    *   Poll hardware encoders.
    *   Stream raw counter values to HMI.
    *   No business logic allowed.

### 3. The FPGA (Electronic Leadscrew) - *The "Muscle"*
*   **Role**: Real-time high-speed stepper control (Future Phase).
*   **Responsibilities**:
    *   Receive configuration from HMI (Pitch, Feed Rate).
    *   Execute hard-real-time motor synchronization.
>>>>>>> 625e49b23c69bc162984814d752430cb36e84e83

---

## 📅 Implementation Phases (Screen-Based)

### Phase 1: Core Readout (The "Dashboard")
*Status: 🚧 In Progress*

<<<<<<< HEAD
### Data Structures

* **`DROState`**: Holds the runtime state (Current Mode, Active Units, Selected Axis).
* **`AxisState`**: Holds data for each of the 5 axes (Raw Position, Relative Offset, Blink Status).
* **`ToolData`**: Holds data for a single tool (Offsets for X/Y/Z, Name/Description).

---

## 📅 Implementation Phases

### Phase 1: Core DRO Logic (The Backend)

*Goal: Create the "Brain" of the DRO that knows how to calculate positions.*

1. **Create `components/dro_system`**:
    * `dro_core.c/.h`: Main logic definitions.
    * `dro_nvs.c/.h`: Storage management.
2. **Implement State Management**:
    * **Units**: Conversion toggling between MM (1.0) and Inch (1/25.4).
    * **Modes**: Toggling between Absolute (Machine Zero) and Incremental (Work Zero).
    * **Axis Calculations**: `DisplayPos = (RawPos - WorkOffset - ToolOffset) * UnitScale`.
3. **Implement Axis Actions**:
    * `dro_axis_zero(axis_index)`
    * `dro_axis_set_value(axis_index, value)`
    * `dro_axis_half(axis_index)`

### Phase 2: Tool Table Verification & NVS

*Goal: Persistent storage for 200 tools.*

1. **Define `ToolData` Struct**:

    ```c
    typedef struct {
        float offsets[5]; // Offsets for X, Y, Z, W, C
        char name[16];    // Short description
        uint8_t type;     // Drill, Lathe, Endmill, etc.
    } ToolData;
    ```

2. **NVS Implementation**:
    * **Key Namespaces**: Use efficient referencing (e.g., `tool_0`, `tool_1`... or a binary blob).
    * **Caching**: Load the table into RAM on boot for fast access; save to NVS only on modification.
3. **API**:
    * `dro_tool_get(index)`
    * `dro_tool_set(index, data)`
    * `dro_tool_apply(index)`: Sets the current active tool offsets.

### Phase 3: UI Integration (The Frontend)

*Goal: Make the buttons on the screen actually do things.*

1. **Event Glue Code (`dro_driver.c`)**:
    * Map LVGL events from `objects.set_zero_axisX_button` to `dro_axis_zero(X)`.
    * Map `toggle_inch_mm_button` to `dro_toggle_units()`.
    * Map `toggle_abs_incr_button` to `dro_toggle_mode()`.
2. **Numpad Integration**:
    * When `Set Value` is clicked: Open Numpad -> Wait for Result -> Call `dro_axis_set_value`.
3. **Display Loop**:
    * Update `virtual_axis_X` variables in `eez_ui_vars.c` every tick based on the calculated `dro_core` state.
    * Update Status Labels (e.g., "MM" vs "INCH", "ABS" vs "INC").

### Phase 4: Additional Button Logic

1. **Half Function**:
    * Logic: `WorkOffset = CurrentRawPos - (CurrentRelPos / 2)`.
2. **Tool Selection**:
    * Click `Tool Number` button -> Enter ID -> Call `dro_tool_apply(ID)`.

### Phase 5: Advanced Features (New)

*Goal: Extended functionality for machine measurement and error correction.*

1.- [ ] **Phase 3: State Synchronization**
    - [ ] Implement `CMD_REQ_STATUS` (0x20) to poll FPGA state
    - [ ] Implement `CMD_SET_WORK_MODE` (0x12) for Screw/Follow/Conical modes
    - [ ] State machine for parsing RX packets

1. **Backlash Compensation**:
    * Implement logic to add/subtract a fixed backlash value when axis direction changes.
    * Add UI in settings to configure backlash amount per axis.
2. **Tool Length Setup**:
    * UI Wizard or specialized screen to measure and capture tool length offsets automatically (possibly using a touch probe input).
    * Integration with measuring the reference tool vs. active tool.
3. **Origin Setup**:
    * Support for multiple Work Coordinate Systems (WCS) (e.g., G54, G55 equivalent).
    * UI to save/recall specific origin points.

---

## 🚀 Next Steps (Execution Plan)

1. **[ ] Create Component**: setup `components/dro_system`.
2. **[ ] Define Headers**: Write `dro_core.h` with the API.
3. **[ ] Implement Tool Table**: Write the NVS logic.
4. **[ ] Wire UI**: Connect the buttons in `main.c` or a dedicated `ui_driver.c`.

---
=======
**Goal**: A high-performance, ISA-101 compliant visualization of position.

*   **[ ] Main Readout Screen**
    *   **Visuals**: Large, high-contrast Sans-Serif fonts (Green/White on Black).
    *   **Widgets**:
        *   `AxisDisplayWidget`: Shows Position (0.000) + Unit (mm/in) + Mode (ABS/INC).
        *   `StatusStrip`: WiFi Status, FPS, Active Workspace Name.
    *   **Interactions**:
        *   Tap Axis -> Open `AxisControlDialog` (Zero / Set Value / 1/2).
        *   Tap Unit Btn -> Toggle Global Units.
        *   Tap ABS/INC Btn -> Toggle Global Mode.

### Phase 2: Milling Functions (Wizards & Dialogs)
*Status: ⏳ Pending*

**Goal**: Graphical wizards that guide the user through complex cuts.

*   **[ ] Bolt Hole Circle (PCD) Screen**
    *   **Input Dialog**: Center (X,Y), Diameter, Count, Start Angle.
    *   **Visual Preview**: Dynamic canvas drawing of the hole pattern.
    *   **Run Mode**: "Distance-to-Go" view showing relative distance to the next hole.
*   **[ ] Linear Grid Screen**
    *   **Input Dialog**: Row Spacing, Col Spacing, Angle, Counts.
    *   **Visual Preview**: Grid layout visualization.
*   **[ ] Arc Generator Screen**
    *   **Input**: Center, Radius, Tool Dia, Max Cut Step.
    *   **Output**: List of coordinates (Waypoints) to follow.

### Phase 3: Lathe Mode (Context Switching)
*Status: ⏳ Pending*

**Goal**: A dedicated UI personality for Turning operations.

*   **[ ] Lathe Dashboard**
    *   **Context**: Remove Y/Z axes, rename/remap to X (Dia) and Z (Longitudinal).
    *   **Widgets**:
        *   `RadiusDiameterToggle`: Visual switch changing X-axis display logic (1x / 2x).
        *   `TaperCalculator`: Input Fields (D1, D2, Length) -> Result Label (Angle).
*   **[ ] Tool Library Screen**
    *   **Layout**: Scrollable list of 200 Tools.
    *   **Interaction**:
        *   "Touch Off" Button: Captures current raw position as offset.
        *   "Apply" Button: Sets chosen tool as active.

### Phase 4: Settings & Metrology
*Status: ⏳ Pending*

**Goal**: Configuration screens for machine accuracy.

*   **[ ] Sensor Dashboard**
    *   **RPM Display**: Analog gauge widget + Digital readout.
    *   **Probe Status**: LED indicator for probe contact.
*   **[ ] Calibration Screen**
    *   **Linear Error**: Table view (Measured vs Displayed) -> Auto-calculate correction factor.
    *   **Segmented Error**: Graph view of error curve.

### Phase 5: Electronic Leadscrew (ELS Integration)
*Status: 🔮 Future / FPGA*

**Goal**: HMI control panel for the FPGA subsystem.

*   **[ ] ELS Control Panel**
    *   **Inputs**: Thread Pitch (mm/rev), Feeds (mm/min).
    *   **Mode Select**: Threading / Turning / Facing.
    *   **Action**: "Send Config" button (transmits parameters to FPGA).
    *   **Feedback**: Status LED (Sync Lock, Motor Run).

---

## 🎨 Design System (ISA-101)
*   **Palette**: Background `#202020`, Text `#FFFFFF`, Highlights `#00FF00`, Alerts `#FF4400`.
*   **Typography**: Roboto / Inter (Condensed for numbers).
*   **Touch Targets**: Minimum 15mm x 15mm hit areas.
>>>>>>> 625e49b23c69bc162984814d752430cb36e84e83
