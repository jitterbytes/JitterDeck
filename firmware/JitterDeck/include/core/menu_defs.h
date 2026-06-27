#ifndef MENU_DEFS_H
#define MENU_DEFS_H

#include <stdint.h>

// =========================================================
// Tool IDs - All the leaf nodes in the menu tree, 
// Each maps to a dedicated tool screen in screem.cpp
// =========================================================

enum ToolID : uint8_t
{
    TOOL_NONE = 0,

    // I2C
    TOOL_I2C_SCANNER,
    TOOL_I2C_BME280,
    TOOL_I2C_BMP280,
    TOOL_I2C_MPU6050,
    TOOL_I2C_BH1750,
    TOOL_I2C_AHT20,
    TOOL_I2C_SHT31,
    TOOL_I2C_LIS2MDL,   // added this
    TOOL_I2C_SSD1306,
    TOOL_I2C_CUSTOM,

    // SPI
    TOOL_SPI_MAX6675,
    TOOL_SPI_MAX31855,
    TOOL_SPI_W25Q,
    TOOL_SPI_RC522,
    TOOL_SPI_CUSTOM,

    // UART
    TOOL_UART_MONITOR,
    TOOL_UART_ECHO,
    TOOL_UART_BAUD_TEST,
    TOOL_UART_NEO6M,
    TOOL_UART_PMS5003,
    TOOL_UART_FINGERPRINT,
    TOOL_UART_CUSTOM,

    // 1-Wire
    TOOL_1W_SEARCH,
    TOOL_1W_DS18B20,
    TOOL_1W_CUSTOM,

    // Analog
    TOOL_ADC_MONITOR,
    TOOL_ADC_VOLTAGE,
    TOOL_ADC_GRAPH,
    TOOL_ADC_LM35,
    TOOL_ADC_MQ2,
    TOOL_ADC_LDR,
    TOOL_ADC_POT,
    TOOL_ADC_CUSTOM,

    // Digital IO
    TOOL_DIO_INPUT_MON,
    TOOL_DIO_OUTPUT_TOGGLE,
    TOOL_DIO_PULSE_COUNT,
    TOOL_DIO_PWM_GEN,
    TOOL_DIO_PIR,
    TOOL_DIO_IR_OBSTACLE,
    TOOL_DIO_REED_SW,
    TOOL_DIO_LIMIT_SW,
    TOOL_DIO_CUSTOM,

    // General Sensors
    TOOL_GEN_HCSR04,
    TOOL_GEN_JSNSR04T,
    TOOL_GEN_DHT11,
    TOOL_GEN_DHT22,
    TOOL_GEN_AM2302,

    // WiFi
    TOOL_WIFI_SCANNER,
    TOOL_WIFI_RSSI,
    TOOL_WIFI_AP_INFO,
    TOOL_WIFI_CHANNEL,
    TOOL_WIFI_PACKET,

    // BLE
    TOOL_BLE_SCANNER,
    TOOL_BLE_ADV,
    TOOL_BLE_DEV_INFO,
    TOOL_BLE_RSSI,
    TOOL_BLE_BEACON,

    // System
    TOOL_SYS_OLED_TEST,
    TOOL_SYS_BTN_TEST,
    TOOL_SYS_GPIO_VIEW,
    TOOL_SYS_BATTERY,
    TOOL_SYS_MEMORY,
    TOOL_SYS_DEV_INFO,
    TOOL_SYS_FW_VER,

    // Settings
    TOOL_SET_BRIGHTNESS,
    TOOL_SET_CONTRAST,
    TOOL_SET_TIMEOUT,
    TOOL_SET_ABOUT,

    TOOL_COUNT
};

// ====================================================================
// MenuItem - All the above nodes each have their indiviual struct 
// if toolID != TOOL_NONE .The leaf node launches a screen
// if toolID == TOOL_NONE .Branch node points to the childern menu
// ====================================================================

struct MenuItem
{
    const char*        label;        // display string
    ToolID             toolID;       // TOOL_NONE if branch
    const MenuItem*    children;     // child array if branch, else nullptr
    uint8_t            childCount;   // length of children array
};

// =======================================================
// Forward declarations - defined in menu_defs.cpp
// =======================================================

extern const MenuItem MENU_MAIN[];
extern const uint8_t  MENU_MAIN_COUNT;

#endif