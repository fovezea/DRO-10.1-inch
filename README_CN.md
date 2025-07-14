# JC8012P4A1C LVGL 9.2 开发板模板

[英文版本](./README.md)

本项目为 JC8012P4A1C 10.1 英寸 ESP32-P4 开发板提供了一个干净的 LVGL 9.2 应用开发模板。所有演示代码已被移除，为构建自定义 LVGL 应用提供了最小化、即用的基础框架。

**主要特性：**
- LVGL 9.2 完全配置且可用
- 包含所有硬件驱动（显示、触摸、音频、传感器）
- 干净的主应用程序，最小化UI
- 为 1280x800 10.1" 显示器优化
- 保留所有 BSP（板级支持包）组件
- 无演示代码 - 可直接用于自定义开发

## 硬件设置

### 准备工作

* JC8012P4A1C 10.1 英寸 ESP32-P4 开发板（ESP32-P4-Function-EV-Board 的中国制造版本）
* 10.1 英寸 1280x800 LCD 显示器，支持 MIPI-DSI 接口
* 触摸控制器（GT911 或兼容型号）
* USB-C 电缆用于供电和编程

### 硬件连接

* **步骤 1**. 通过 MIPI-DSI 接口连接显示器
* **步骤 2**. 通过 I2C 连接触摸控制器
* **步骤 3**. 将 USB-C 电缆连接到 `USB-UART` 端口
* **步骤 4**. 开启开发板电源

### ESP-IDF 要求

- ESP-IDF v5.3 或更高版本（已在 v5.4.2 测试）
- 请参照 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html) 设置开发环境

## 开发指南

### 编译项目

```bash
idf.py build
```

### 烧录和监控

```bash
idf.py -p PORT flash monitor
```

### 项目结构

```
├── main/
│   ├── main.c                 # 干净的主应用程序
│   └── CMakeLists.txt         # 编译配置
├── managed_components/        # LVGL 和驱动组件
├── common_components/         # BSP 和硬件驱动  
├── sdkconfig.defaults         # 干净的项目配置
└── partitions.csv            # Flash 分区布局
```

### 包含的驱动和组件

- **显示器**: 1280x800 MIPI-DSI LCD 驱动
- **触摸**: GT911 电容触摸控制器
- **音频**: 多编解码器支持（ES8311、ES7210、ES7243 等）
- **存储**: SD 卡和 SPIFFS 文件系统
- **摄像头**: ISP 和 MIPI-CSI 支持
- **连接**: I2C、SPI、UART 等

### 开始开发

项目在启动时创建一个简单的欢迎屏幕。要开始开发您的应用程序：

1. 修改 `main/main.c` 来实现您的自定义UI
2. 使用所有可用的 LVGL 9.2 组件和功能
3. 通过 BSP 函数访问硬件
4. 根据需要添加您的应用程序逻辑

### 使用示例

```c
// 在 main.c 中，将 create_basic_ui() 替换为您的自定义UI
static void create_my_app_ui(void)
{
    lv_obj_t *scr = lv_screen_active();
    
    // 您的自定义 LVGL 代码
    lv_obj_t *button = lv_btn_create(scr);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, "我的应用");
    lv_obj_center(button);
}
```

### 配置

要修改 LVGL 或硬件设置：
```bash
idf.py menuconfig
```

导航到：
- `Component config > LVGL` 用于 LVGL 设置
- `Component config > Board Support Package(ESP32-P4)` 用于硬件设置

## 技术支持

如有问题：
- 硬件相关问题：查看您的 JC8012P4A1C 开发板文档
- LVGL 问题：访问 [LVGL 文档](https://docs.lvgl.io/)
- ESP-IDF 问题：查看 [ESP-IDF 文档](https://docs.espressif.com/projects/esp-idf/)

## 许可证

本项目遵循与原始 ESP-IDF 示例相同的许可条款。