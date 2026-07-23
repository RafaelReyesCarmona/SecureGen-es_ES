#include "usb_hid_manager.h"

#ifdef BOARD_HAS_USB_HID

UsbHidManager::UsbHidManager() {}

bool UsbHidManager::begin() {
  if (_started) return true;
  _keyboard.begin();
  //_keyboard.begin(KeyboardLayout_es_ES);
  USB.begin();
  _started = true;
  delay(100);
  return true;
}

void UsbHidManager::end() {
  _started = false;
}

bool UsbHidManager::isConnected() {
  if (!_started) return false;
  return tud_hid_ready();
}
void UsbHidManager::sendKey(char k, uint8_t m) {
  if (m) {
    _keyboard.press(m); 
    delay(10); 
  } 
  _keyboard.press(k); 
  delay(10); 
  _keyboard.releaseAll();
}

void UsbHidManager::sendPassword(const char* password) {
  if (!_started) return;
      while (*password) {
        char c = (unsigned char)*password;
        if (c == 0xC3) {
          password++;
          uint8_t c2 = (unsigned char)*password;
          switch (c2) {
            case 0xB1: sendKey(';',NO_KEY_MOD); break;      // key 'ñ'
            case 0x91: sendKey(';',KEY_LEFT_SHIFT); break;  // key 'Ñ'
            case 0xA1: sendKey('\'',NO_KEY_MOD); sendKey('a',NO_KEY_MOD); break;     // key 'á' 
            case 0x81: sendKey('\'',NO_KEY_MOD); sendKey('a',KEY_LEFT_SHIFT); break; // key 'Á'
            case 0xA9: sendKey('\'',NO_KEY_MOD); sendKey('e',NO_KEY_MOD); break;     // key 'é' 
            case 0x89: sendKey('\'',NO_KEY_MOD); sendKey('e',KEY_LEFT_SHIFT); break; // key 'É'
            case 0xAD: sendKey('\'',NO_KEY_MOD); sendKey('i',NO_KEY_MOD); break;     // key 'í' 
            case 0x8D: sendKey('\'',NO_KEY_MOD); sendKey('i',KEY_LEFT_SHIFT); break; // key 'Í'
            case 0xB3: sendKey('\'',NO_KEY_MOD); sendKey('o',NO_KEY_MOD); break;     // key 'ó' 
            case 0x93: sendKey('\'',NO_KEY_MOD); sendKey('o',KEY_LEFT_SHIFT); break; // key 'Ó'
            case 0xBA: sendKey('\'',NO_KEY_MOD); sendKey('u',NO_KEY_MOD); break;     // key 'ú' 
            case 0x9A: sendKey('\'',NO_KEY_MOD); sendKey('u',KEY_LEFT_SHIFT); break; // key 'Ú'
            case 0xA2: sendKey('[',KEY_LEFT_SHIFT); sendKey('a',NO_KEY_MOD); break;     // key 'â' 
            case 0x82: sendKey('[',KEY_LEFT_SHIFT); sendKey('a',KEY_LEFT_SHIFT); break;     // key 'Â' 
            case 0xAA: sendKey('[',KEY_LEFT_SHIFT); sendKey('e',NO_KEY_MOD); break;     // key 'ê' 
            case 0x8A: sendKey('[',KEY_LEFT_SHIFT); sendKey('e',KEY_LEFT_SHIFT); break;     // key 'Ê' 
            case 0xAE: sendKey('[',KEY_LEFT_SHIFT); sendKey('i',NO_KEY_MOD); break;     // key 'î' 
            case 0x8E: sendKey('[',KEY_LEFT_SHIFT); sendKey('i',KEY_LEFT_SHIFT); break;     // key 'Î' 
            case 0xB4: sendKey('[',KEY_LEFT_SHIFT); sendKey('o',NO_KEY_MOD); break;     // key 'ô' 
            case 0x94: sendKey('[',KEY_LEFT_SHIFT); sendKey('o',KEY_LEFT_SHIFT); break;     // key 'Ô' 
            case 0xBB: sendKey('[',KEY_LEFT_SHIFT); sendKey('u',NO_KEY_MOD); break;     // key 'û' 
            case 0x9B: sendKey('[',KEY_LEFT_SHIFT); sendKey('u',KEY_LEFT_SHIFT); break;     // key 'Û' 
            case 0xA4: sendKey('\'',KEY_LEFT_SHIFT); sendKey('a',NO_KEY_MOD); break;     // key 'ä' 
            case 0x84: sendKey('\'',KEY_LEFT_SHIFT); sendKey('a',KEY_LEFT_SHIFT); break;     // key 'Ä' 
            case 0xAB: sendKey('\'',KEY_LEFT_SHIFT); sendKey('e',NO_KEY_MOD); break;     // key 'ë' 
            case 0x8B: sendKey('\'',KEY_LEFT_SHIFT); sendKey('e',KEY_LEFT_SHIFT); break;     // key 'Ë' 
            case 0xAF: sendKey('\'',KEY_LEFT_SHIFT); sendKey('i',NO_KEY_MOD); break;     // key 'ï' 
            case 0x8F: sendKey('\'',KEY_LEFT_SHIFT); sendKey('i',KEY_LEFT_SHIFT); break;     // key 'Ï' 
            case 0xB6: sendKey('\'',KEY_LEFT_SHIFT); sendKey('o',NO_KEY_MOD); break;     // key 'ö' 
            case 0x96: sendKey('\'',KEY_LEFT_SHIFT); sendKey('o',KEY_LEFT_SHIFT); break;     // key 'Ö' 
            case 0xBC: sendKey('\'',KEY_LEFT_SHIFT); sendKey('u',NO_KEY_MOD); break;     // key 'ü' 
            case 0x9C: sendKey('\'',KEY_LEFT_SHIFT); sendKey('u',KEY_LEFT_SHIFT); break;     // key 'Ü' 
          } // end switch c2
        } // end if
        //if  (c == 0xC2) {
          //password++;
          //uint8_t c2 = (unsigned char)*password;
          //switch (c2) {
            //case 0xBA: sendKey('`',NO_KEY_MOD); break;      // key 'º'
            //case 0xAA: sendKey('`',KEY_RIGHT_SHIFT); break;  // key 'ª' special key for system.
          //} // end switch c2
        //} // end if
        switch(c) {
          case '@': sendKey('2',KEY_RIGHT_ALT); break;
          case '#': sendKey('3',KEY_RIGHT_ALT); break;
          case '&': sendKey('6',KEY_LEFT_SHIFT); break;
          case '/': sendKey('7',KEY_LEFT_SHIFT); break;
          case '(': sendKey('8',KEY_LEFT_SHIFT); break;
          case ')': sendKey('9',KEY_LEFT_SHIFT); break;
          case '=': sendKey('0',KEY_LEFT_SHIFT); break;
          case '?': sendKey('_',KEY_LEFT_SHIFT); break;
          case '-': sendKey('/',NO_KEY_MOD); break;
          case '_': sendKey('/',KEY_LEFT_SHIFT); break;
          case ';': sendKey(',',KEY_LEFT_SHIFT); break;
          case ':': sendKey('.',KEY_LEFT_SHIFT); break;
          default:
            _keyboard.write((uint8_t)(c));
        }
        password++;
      }
  //_keyboard.write(password,sizeof(password));
  //_keyboard.print(String(password));
}

void UsbHidManager::sendEnter() {
  if (!_started) return;
  _keyboard.write(KEY_RETURN);
  delay(50);
}
/*
uint8_t UsbHidManager::charToHidKey(char c) {
  if (c >= 'a' && c <= 'z') return c - 'a' + 0x04;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 0x04;
  if (c >= '1' && c <= '9') return c - '1' + 0x1E;
  if (c == '0') return 0x27;
  
  if (c == '!' || c == '@' || c == '#' || c == '$' || c == '%' ||
      c == '^' || c == '&' || c == '*' || c == '(' || c == ')')
    return charToHidKey("1234567890"[c == '!' ? 0 : c == '@' ? 1 :
                                      c == '#' ? 2 : c == '$' ? 3 : c == '%' ? 4 :
                                      c == '^' ? 5 : c == '&' ? 6 : c == '*' ? 7 :
                                      c == '(' ? 8 : 9]);
  
  if (c == ' ') return 0x2C;
  if (c == '\n') return 0x28;
  if (c == '\t') return 0x2B;
  if (c == '-' || c == '_') return 0x2D;
  if (c == '=' || c == '+') return 0x2E;
  if (c == '[' || c == '{') return 0x2F;
  if (c == ']' || c == '}') return 0x30;
  if (c == '\\' || c == '|') return 0x31;
  if (c == ';' || c == ':') return 0x33;
  if (c == '\'' || c == '"') return 0x34;
  if (c == '`' || c == '~') return 0x35;
  if (c == ',' || c == '<') return 0x36;
  if (c == '.' || c == '>') return 0x37;
  if (c == '/' || c == '?') return 0x38;
  
  return 0;
}

uint8_t UsbHidManager::charToModifier(char c) {
  if (c >= 'A' && c <= 'Z') return 0x02; // Left Shift
  if (c == '!' || c == '@' || c == '#' || c == '$' || c == '%' ||
      c == '^' || c == '&' || c == '*' || c == '(' || c == ')') return 0x02;
  if (c == '_' || c == '+' || c == '{' || c == '}' || c == '|' ||
      c == ':' || c == '"' || c == '~' || c == '<' || c == '>' ||
      c == '?') return 0x02;
  return 0x00;
}
*/
#endif // BOARD_HAS_USB_HID
