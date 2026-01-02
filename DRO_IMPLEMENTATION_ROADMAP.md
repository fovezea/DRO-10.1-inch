# HMI-Centric DRO Implementation Roadmap

This roadmap defines the development of the **Digital Readout (DRO) HMI**. The design philosophy is strictly **"Smart Client, Dumb Backend"**.

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

---

## 📅 Implementation Phases (Screen-Based)

### Phase 1: Core Readout (The "Dashboard")
*Status: 🚧 In Progress*

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
