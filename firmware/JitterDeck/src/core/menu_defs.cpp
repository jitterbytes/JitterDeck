#include "menu_defs.h"

// =============================
// I2C Tools 
// =============================

// I2C Child Node
static const MenuItem I2C_SENSOR_PROFILES[] =
{
    { "LIS2MDL",          TOOL_I2C_LIS2MDL,  nullptr, 0 },  // added this
    { "Custom I2C",       TOOL_I2C_CUSTOM,   nullptr, 0 },
};

// I2C Branch Node
static const MenuItem I2C_MENU[] =
{
    { "I2C Scanner",      TOOL_I2C_SCANNER,  nullptr,           0 },
    { "Sensor Profiles",  TOOL_NONE,         I2C_SENSOR_PROFILES, 2 },  // updated the count
};

// =============================
// SPI Tools
// =============================

// SPI Child Node
static const MenuItem SPI_SENSOR_PROFILES[] =
{
    { "MAX6675",          TOOL_SPI_MAX6675,  nullptr, 0 },
    { "MAX31855",         TOOL_SPI_MAX31855, nullptr, 0 },
    { "W25Q Flash",       TOOL_SPI_W25Q,     nullptr, 0 },
    { "RFID RC522",       TOOL_SPI_RC522,    nullptr, 0 },
    { "Custom SPI",       TOOL_SPI_CUSTOM,   nullptr, 0 },
};

// SPI Branch Node
static const MenuItem SPI_MENU[] =
{
    { "Sensor Profiles",  TOOL_NONE,         SPI_SENSOR_PROFILES, 5 },
};

// =============================
// UART Tools
// =============================

// UART Child Node
static const MenuItem UART_SENSOR_PROFILES[] =
{
    { "NEO-6M GPS",       TOOL_UART_NEO6M,        nullptr, 0 },
    { "PMS5003",          TOOL_UART_PMS5003,       nullptr, 0 },
    { "Fingerprint",      TOOL_UART_FINGERPRINT,   nullptr, 0 },
    { "Custom UART",      TOOL_UART_CUSTOM,        nullptr, 0 },
};

// UART Branch Node
static const MenuItem UART_MENU[] =
{
    { "Serial Monitor",   TOOL_UART_MONITOR,   nullptr,              0 },
    { "Serial Echo",      TOOL_UART_ECHO,      nullptr,              0 },
    { "Baud Rate Tester", TOOL_UART_BAUD_TEST, nullptr,              0 },
    { "Sensor Profiles",  TOOL_NONE,           UART_SENSOR_PROFILES, 4 },
};

// =============================
// 1-Wire Tools
// =============================

// 1-Wire Child Node
static const MenuItem ONEWIRE_SENSOR_PROFILES[] =
{
    { "DS18B20",          TOOL_1W_DS18B20, nullptr, 0 },
    { "Custom 1-Wire",    TOOL_1W_CUSTOM,  nullptr, 0 },
};

// 1-Wire Branch Node
static const MenuItem ONEWIRE_MENU[] =
{
    { "Device Search",    TOOL_1W_SEARCH, nullptr,                0 },
    { "Sensor Profiles",  TOOL_NONE,      ONEWIRE_SENSOR_PROFILES, 2 },
};

// =============================
// Analog Tools
// =============================

// Analog Child Node
static const MenuItem ANALOG_SENSOR_PROFILES[] =
{
    { "LM35",             TOOL_ADC_LM35,   nullptr, 0 },
    { "MQ2",              TOOL_ADC_MQ2,    nullptr, 0 },
    { "LDR",              TOOL_ADC_LDR,    nullptr, 0 },
    { "Potentiometer",    TOOL_ADC_POT,    nullptr, 0 },
    { "Custom Analog",    TOOL_ADC_CUSTOM, nullptr, 0 },
};

// Analog Branch Node
static const MenuItem ANALOG_MENU[] =
{
    { "Live ADC Monitor", TOOL_ADC_MONITOR,  nullptr,               0 },
    { "Voltage Meter",    TOOL_ADC_VOLTAGE,  nullptr,               0 },
    { "ADC Graph",        TOOL_ADC_GRAPH,    nullptr,               0 },
    { "Sensor Profiles",  TOOL_NONE,         ANALOG_SENSOR_PROFILES, 5 },
};

// =============================
// Digital IO Tools
// =============================

// Digital Child Node
static const MenuItem DIO_SENSOR_PROFILES[] =
{
    { "PIR Sensor",       TOOL_DIO_PIR,         nullptr, 0 },
    { "IR Obstacle",      TOOL_DIO_IR_OBSTACLE, nullptr, 0 },
    { "Reed Switch",      TOOL_DIO_REED_SW,     nullptr, 0 },
    { "Limit Switch",     TOOL_DIO_LIMIT_SW,    nullptr, 0 },
    { "Custom Digital",   TOOL_DIO_CUSTOM,      nullptr, 0 },
};

// Digital Branch Node
static const MenuItem DIO_MENU[] =
{
    { "Input Monitor",    TOOL_DIO_INPUT_MON,     nullptr,            0 },
    { "Output Toggle",    TOOL_DIO_OUTPUT_TOGGLE, nullptr,            0 },
    { "Pulse Counter",    TOOL_DIO_PULSE_COUNT,   nullptr,            0 },
    { "PWM Generator",    TOOL_DIO_PWM_GEN,       nullptr,            0 },
    { "Sensor Profiles",  TOOL_NONE,              DIO_SENSOR_PROFILES, 5 },
};

// =============================
// General Sensors
// =============================

// General Sensors Branch Node
static const MenuItem GENERAL_MENU[] =
{
    { "HC-SR04",          TOOL_GEN_HCSR04,   nullptr, 0 },
    { "JSN-SR04T",        TOOL_GEN_JSNSR04T, nullptr, 0 },
    { "DHT11",            TOOL_GEN_DHT11,    nullptr, 0 },
    { "DHT22",            TOOL_GEN_DHT22,    nullptr, 0 },
    { "AM2302",           TOOL_GEN_AM2302,   nullptr, 0 },
};

// =============================
// WiFi Tools
// =============================

// WiFi Branch Node
static const MenuItem WIFI_MENU[] =
{
    { "WiFi Scanner",     TOOL_WIFI_SCANNER, nullptr, 0 },
    { "RSSI Meter",       TOOL_WIFI_RSSI,    nullptr, 0 },
    { "AP Information",   TOOL_WIFI_AP_INFO, nullptr, 0 },
    { "Channel Viewer",   TOOL_WIFI_CHANNEL, nullptr, 0 },
    { "Packet Counter",   TOOL_WIFI_PACKET,  nullptr, 0 },
};

// =============================
// BLE Tools
// =============================

// BLE Branch Node
static const MenuItem BLE_MENU[] =
{
    { "BLE Scanner",      TOOL_BLE_SCANNER,  nullptr, 0 },
    { "Advertisements",   TOOL_BLE_ADV,      nullptr, 0 },
    { "Device Info",      TOOL_BLE_DEV_INFO, nullptr, 0 },
    { "RSSI Monitor",     TOOL_BLE_RSSI,     nullptr, 0 },
    { "Beacon Viewer",    TOOL_BLE_BEACON,   nullptr, 0 },
};

// =============================
// System Tools
// =============================

// System Tools Branch Node
static const MenuItem SYSTEM_MENU[] =
{
    { "OLED Test",        TOOL_SYS_OLED_TEST, nullptr, 0 },
    { "Button Test",      TOOL_SYS_BTN_TEST,  nullptr, 0 },
    { "GPIO Viewer",      TOOL_SYS_GPIO_VIEW, nullptr, 0 },
    { "Battery Status",   TOOL_SYS_BATTERY,   nullptr, 0 },
    { "Memory Usage",     TOOL_SYS_MEMORY,    nullptr, 0 },
    { "Device Info",      TOOL_SYS_DEV_INFO,  nullptr, 0 },
    { "Firmware Version", TOOL_SYS_FW_VER,    nullptr, 0 },
};

// =============================
// Settings
// =============================

// Setting Child Node
static const MenuItem SETTINGS_DISPLAY[] =
{
    { "Brightness",       TOOL_SET_BRIGHTNESS, nullptr, 0 },
    { "Contrast",         TOOL_SET_CONTRAST,   nullptr, 0 },
    { "Screen Timeout",   TOOL_SET_TIMEOUT,    nullptr, 0 },
};

// Setting Branch Node
static const MenuItem SETTINGS_MENU[] =
{
    { "Display",          TOOL_NONE,           SETTINGS_DISPLAY, 3 },
    { "About",            TOOL_SET_ABOUT,      nullptr,          0 },
};

// =============================
// Root - Main Menu
// =============================

const MenuItem MENU_MAIN[] =
{
    { "I2C Tools",        TOOL_NONE, I2C_MENU,     2 },
    { "SPI Tools",        TOOL_NONE, SPI_MENU,     1 },
    { "UART Tools",       TOOL_NONE, UART_MENU,    4 },
    { "1-Wire Tools",     TOOL_NONE, ONEWIRE_MENU, 2 },
    { "Analog Tools",     TOOL_NONE, ANALOG_MENU,  4 },
    { "Digital IO",       TOOL_NONE, DIO_MENU,     5 },
    { "General Sensors",  TOOL_NONE, GENERAL_MENU, 5 },
    { "WiFi Tools",       TOOL_NONE, WIFI_MENU,    5 },
    { "BLE Tools",        TOOL_NONE, BLE_MENU,     5 },
    { "System Tools",     TOOL_NONE, SYSTEM_MENU,  7 },
    { "Settings",         TOOL_NONE, SETTINGS_MENU, 2 },
};

const uint8_t MENU_MAIN_COUNT = 11;