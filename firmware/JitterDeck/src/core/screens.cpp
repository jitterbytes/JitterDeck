#include <Arduino.h>
#include <U8g2lib.h>

#include "screens.h"
#include "fsm.h"
#include "button.h"
#include "nav_stack.h"
#include "menu_defs.h"
#include "i2c_scanner.h"
#include "lis2mdl.h"    // added this

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// =============================================
//  toolName()
//  Maps every ToolID to a display string.
//  Used by the stub renderer for unimplemented
//  tools and by any screen needing a title.
// =============================================

static const char* toolName(ToolID id)
{
    switch (id)
    {
        case TOOL_I2C_SCANNER:       return "I2C Scanner";
        case TOOL_I2C_LIS2MDL:       return "LIS2MDL";  // added this
        case TOOL_I2C_CUSTOM:        return "Custom I2C";
        case TOOL_SPI_MAX6675:       return "MAX6675";
        case TOOL_SPI_MAX31855:      return "MAX31855";
        case TOOL_SPI_W25Q:          return "W25Q Flash";
        case TOOL_SPI_RC522:         return "RFID RC522";
        case TOOL_SPI_CUSTOM:        return "Custom SPI";
        case TOOL_UART_MONITOR:      return "Serial Monitor";
        case TOOL_UART_ECHO:         return "Serial Echo";
        case TOOL_UART_BAUD_TEST:    return "Baud Rate Tester";
        case TOOL_UART_NEO6M:        return "NEO-6M GPS";
        case TOOL_UART_PMS5003:      return "PMS5003";
        case TOOL_UART_FINGERPRINT:  return "Fingerprint";
        case TOOL_UART_CUSTOM:       return "Custom UART";
        case TOOL_1W_SEARCH:         return "Device Search";
        case TOOL_1W_DS18B20:        return "DS18B20";
        case TOOL_1W_CUSTOM:         return "Custom 1-Wire";
        case TOOL_ADC_MONITOR:       return "Live ADC Monitor";
        case TOOL_ADC_VOLTAGE:       return "Voltage Meter";
        case TOOL_ADC_GRAPH:         return "ADC Graph";
        case TOOL_ADC_LM35:          return "LM35";
        case TOOL_ADC_MQ2:           return "MQ2";
        case TOOL_ADC_LDR:           return "LDR";
        case TOOL_ADC_POT:           return "Potentiometer";
        case TOOL_ADC_CUSTOM:        return "Custom Analog";
        case TOOL_DIO_INPUT_MON:     return "Input Monitor";
        case TOOL_DIO_OUTPUT_TOGGLE: return "Output Toggle";
        case TOOL_DIO_PULSE_COUNT:   return "Pulse Counter";
        case TOOL_DIO_PWM_GEN:       return "PWM Generator";
        case TOOL_DIO_PIR:           return "PIR Sensor";
        case TOOL_DIO_IR_OBSTACLE:   return "IR Obstacle";
        case TOOL_DIO_REED_SW:       return "Reed Switch";
        case TOOL_DIO_LIMIT_SW:      return "Limit Switch";
        case TOOL_DIO_CUSTOM:        return "Custom Digital";
        case TOOL_GEN_HCSR04:        return "HC-SR04";
        case TOOL_GEN_JSNSR04T:      return "JSN-SR04T";
        case TOOL_GEN_DHT11:         return "DHT11";
        case TOOL_GEN_DHT22:         return "DHT22";
        case TOOL_GEN_AM2302:        return "AM2302";
        case TOOL_WIFI_SCANNER:      return "WiFi Scanner";
        case TOOL_WIFI_RSSI:         return "RSSI Meter";
        case TOOL_WIFI_AP_INFO:      return "AP Information";
        case TOOL_WIFI_CHANNEL:      return "Channel Viewer";
        case TOOL_WIFI_PACKET:       return "Packet Counter";
        case TOOL_BLE_SCANNER:       return "BLE Scanner";
        case TOOL_BLE_ADV:           return "Advertisements";
        case TOOL_BLE_DEV_INFO:      return "Device Info";
        case TOOL_BLE_RSSI:          return "RSSI Monitor";
        case TOOL_BLE_BEACON:        return "Beacon Viewer";
        case TOOL_SYS_OLED_TEST:     return "OLED Test";
        case TOOL_SYS_BTN_TEST:      return "Button Test";
        case TOOL_SYS_GPIO_VIEW:     return "GPIO Viewer";
        case TOOL_SYS_BATTERY:       return "Battery Status";
        case TOOL_SYS_MEMORY:        return "Memory Usage";
        case TOOL_SYS_DEV_INFO:      return "Device Info";
        case TOOL_SYS_FW_VER:        return "Firmware Version";
        case TOOL_SET_BRIGHTNESS:    return "Brightness";
        case TOOL_SET_CONTRAST:      return "Contrast";
        case TOOL_SET_TIMEOUT:       return "Screen Timeout";
        case TOOL_SET_ABOUT:         return "About";
        default:                     return "Unknown Tool";
    }
}

// =============================================
//  Stub renderer
//  Used for every tool that isn't implemented yet
// =============================================

static void drawStub(ToolID id)
{
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 10, toolName(id));
    u8g2.drawHLine(0, 13, DISPLAY_W);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(4, 30, "[ Not implemented ]");
    u8g2.drawStr(4, 44, "TAP: Back");
}

// =============================================
//  onToolEnter()
//  Called once when transitioning INTO a tool.
//  Use to reset tool-local state before first draw.
// =============================================

void onToolEnter(ToolID id)
{
    switch (id)
    {
        case TOOL_I2C_SCANNER: resetI2CScanner(); break;
        case TOOL_I2C_LIS2MDL:  resetLIS2MDL();    break;   // added this
        default: break;
    }
}

// =============================================
//  drawToolScreen()
//  Called every loop() tick while in SCREEN_TOOL
// =============================================

void drawToolScreen()
{
    const ToolID id = getActiveTool();

    switch (id)
    {
        case TOOL_I2C_SCANNER:
            drawI2CScanner();
            break;
        
        case TOOL_I2C_LIS2MDL:  // added this
            drawLIS2MDL();
            break;

        // == All other tools: stub until implemented ==================
        default:
            u8g2.clearBuffer();
            drawStub(id);
            u8g2.sendBuffer();
            break;
    }
}

// =============================================
//  handleToolScreen()
//  Called every loop() tick while in SCREEN_TOOL
// =============================================

void handleToolScreen()
{
    const ToolID id = getActiveTool();

    switch (id)
    {
        case TOOL_I2C_SCANNER:
            handleI2CScanner();
            break;

        case TOOL_I2C_LIS2MDL:  // added this
            handleLIS2MDL();
            break;

        // == Stub handler: TAP SELECT → back to menu ==================
        default:
        {
            const ButtonEvent evSelect = btnSelect();
            if (evSelect == BTN_EVT_SHORT_PRESS)
            {
                setActiveTool(TOOL_NONE);
                setScreen(SCREEN_MENU);
            }
            break;
        }
    }
}