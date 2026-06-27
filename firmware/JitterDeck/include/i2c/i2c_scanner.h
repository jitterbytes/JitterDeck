#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

// =============================================
//  I2C Scanner tool
//  Scans 0x01–0x7F, identifies known devices,
//  displays results in a scrollable list.
//
//  Call from screens.cpp when TOOL_I2C_SCANNER
//  is active:
//    case TOOL_I2C_SCANNER:
//      handleI2CScanner();
//      drawI2CScanner();
//      break;
// =============================================

void drawI2CScanner();
void handleI2CScanner();
void resetI2CScanner();   // call when entering the tool

#endif