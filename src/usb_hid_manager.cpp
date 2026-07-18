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
