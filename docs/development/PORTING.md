# SecureGen Porting Guide

> How to port SecureGen firmware to a new ESP32 board.  
> Target audience: Arduino enthusiasts and embedded developers.

---

## Table of Contents

1. [Hardware Requirements](#1-hardware-requirements)
2. [Architecture Overview](#2-architecture-overview)
3. [Step-by-Step: Adding a New Board](#3-step-by-step-adding-a-new-board)
4. [Which `#ifdef` to Touch — and Which Not](#4-which-ifdef-to-touch--and-which-not)
5. [Porting Checklist](#5-porting-checklist)
6. [Common Mistakes & Anti-Patterns](#6-common-mistakes--anti-patterns)

---

## 1. Hardware Requirements

### Mandatory

| Requirement | Reason |
|---|---|
| ESP32 or ESP32-S3 SoC | mbedTLS, LittleFS, BLE HID stack |
| SPI or Parallel TFT display | UI rendering (TFT_eSPI / LGFX) |
| ≥ 4 MB Flash | Firmware + LittleFS partition |
| ≥ 2 physical buttons | Navigation + factory reset (both held 5s) |
| Hardware entropy source | mbedTLS CTR_DRBG — available on all ESP32 variants |

### Strongly Recommended

| Requirement | Reason |
|---|---|
| 8 MB PSRAM | Large JSON documents, password vault; required for long payloads |
| USB HID support (ESP32-S3) | Type passwords/TOTP codes as keyboard |
| DS3231 RTC module | TOTP in AP/Offline mode without NTP |

### Not Supported

- ESP8266 — no hardware entropy, no BLE, insufficient RAM
- ESP32-C3 / C6 single-core — untested, BLE HID stack behaviour unknown
- Boards without a display — UI is display-first, no headless mode

---

## 2. Architecture Overview

Understanding these layers saves you from breaking security when porting.

```
┌─────────────────────────────────────────────┐
│              platformio.ini                  │
│   defines: ARDUINO_LILYGO_T_DISPLAY_S3      │  ← compile-time board flag
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│         include/boards/board_*.h             │  ← board constants (pins, dims)
│   BOARD_HAS_USB_HID, LCD_POWER_ON_PIN, …    │  ← capability flags derived here
└──────────────────┬──────────────────────────┘
                   │
┌──────────────────▼──────────────────────────┐
│              Source files                    │
│  Use BOARD_HAS_USB_HID, not raw board flag  │  ← feature flags, not board names
└─────────────────────────────────────────────┘
```

**Rule:** source files should know about *features*, not *board names*.  
`#ifdef BOARD_HAS_USB_HID` ✅ — `#ifdef ARDUINO_LILYGO_T_DISPLAY_S3` in business logic ⚠️

---

## 3. Step-by-Step: Adding a New Board

### Step 1 — Create board header

Copy the closest existing header and adapt it:

```bash
cp include/boards/board_t_display_s3.h include/boards/board_myboard.h
```

Mandatory constants to define:

```cpp
// include/boards/board_myboard.h

#pragma once

// --- Display ---
#define BOARD_TFT_WIDTH      240      // physical pixels
#define BOARD_TFT_HEIGHT     135
#define BOARD_TFT_BL_PIN     4        // backlight GPIO

// --- Buttons ---
#define BOARD_BTN_TOP        35       // top button GPIO
#define BOARD_BTN_BOTTOM     0        // bottom button GPIO (also deep sleep wake pin)

// --- Power ---
#define LCD_POWER_ON_PIN     10       // -1 if not present

// --- Capability flags ---
#define BOARD_HAS_USB_HID    0        // 1 only for ESP32-S3 with native USB
#define BOARD_HAS_PSRAM      0        // 1 if PSRAM available and initialized
```

### Step 2 — Register in platformio.ini

Add a new build environment:

```ini
[env:myboard]
platform = espressif32
board = <your_board_id>
build_flags =
    ${common.build_flags}
    -DMYBOARD_FLAG=1          ; your unique identifier
    -DSECURE_LAYER_ENABLED=1  ; always keep this
framework = arduino
```

> **Do not remove `-DSECURE_LAYER_ENABLED=1`.**  
> This enables AES-256-GCM encryption, ECDH key exchange, and tunnel decryption.  
> Without it the web interface sends all data in plaintext.

### Step 3 — Add board detection guard

In `include/boards/board_myboard.h` add the detection block, then include it from the main board selector (if one exists), or add detection directly:

```cpp
// At the top of board_myboard.h
#ifdef MYBOARD_FLAG
```

### Step 4 — Handle deep sleep wake pin

The bottom button (GPIO0 on reference boards) wakes the device from deep sleep.  
ESP32 and ESP32-S3 use **different APIs** — this is already abstracted in `main.cpp` and `pin_manager.cpp` via `ARDUINO_LILYGO_T_DISPLAY_S3`. For a new board you add another branch:

```cpp
#ifdef MYBOARD_FLAG
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_X, 0); // adjust GPIO
#elif defined(ARDUINO_LILYGO_T_DISPLAY_S3)
    esp_sleep_enable_ext1_wakeup((1ULL << GPIO_NUM_0), ESP_EXT1_WAKEUP_ANY_LOW);
#else
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
#endif
```

> **Tip:** This pattern repeats 3 times in `main.cpp` (:1208, :1533, :1599) and once in `pin_manager.cpp` (:218). Change all four.

### Step 5 — Adapt display geometry

UI layout uses `tft->height()` and `tft->width()` dynamically in most places, but some offsets are hardcoded per board. Search for display-specific blocks:

```bash
grep -n "ARDUINO_LILYGO_T_DISPLAY_S3" \
  src/display_manager.cpp src/splash_manager.cpp src/main.cpp
```

Add your board's geometry in the same `#ifdef` chain where needed.

### Step 6 — USB HID (ESP32-S3 only)

If your board has native USB:

```cpp
// in board_myboard.h
#define BOARD_HAS_USB_HID 1
```

All USB HID code is already guarded by `#ifdef BOARD_HAS_USB_HID` in `web_server.cpp` and `main.cpp`. No other changes needed — the flag does the work.

If your board does **not** have USB HID:

```cpp
#define BOARD_HAS_USB_HID 0
```

Zero is enough — do not delete or comment out the `#ifdef BOARD_HAS_USB_HID` blocks.

### Step 7 — Verify LittleFS partition

Check `partitions_16mb.csv` matches your flash size. If your board has less than 16 MB flash, create a matching partition table. Minimum recommended:

| Partition | Size |
|---|---|
| app0 | 2 MB |
| app1 | 2 MB |
| littlefs | 8 MB |

LittleFS stores all encrypted data — do not shrink it below 4 MB.

---

## 4. Which `#ifdef` to Touch — and Which Not

### Safe to add your board to ✅

| Location | What it guards | Action |
|---|---|---|
| `main.cpp` — sleep wake pin | ext0 vs ext1 API | Add `#elif defined(MYBOARD_FLAG)` |
| `display_manager.cpp` — geometry | pixel offsets | Add `#elif` with your dimensions |
| `splash_manager.cpp` — splash size | image resolution | Add `#elif` with your resolution |
| `pin_manager.cpp` :146 — star/selector Y | UI layout | Add `#elif` with your offsets |
| `web_server.cpp` :3881, :5899, :7650 — board name | JSON `"board"` field | Add `#elif` with your board name |

### Do not touch ❌

| Flag | Reason |
|---|---|
| `SECURE_LAYER_ENABLED` | Always must be 1 in production. Removing `#else` branches breaks nothing but removing the flag disables all encryption. |
| `BOARD_HAS_USB_HID` | Already correct abstraction — set it in `board_*.h`, never override in source. |
| `DEBUG_BUILD` | Dev-only. Never define in production `platformio.ini`. |
| `ARDUINO_USB_CDC_ON_BOOT` | Low-level ESP-IDF flag, managed by board definition. |

### Pattern to follow

```cpp
// ✅ Correct — feature flag in source
#ifdef BOARD_HAS_USB_HID
    usbHIDManager.typeString(password);
#endif

// ⚠️ Acceptable — board name only for identity/display
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    doc["board"] = "T-Display S3";
#elif defined(MYBOARD_FLAG)
    doc["board"] = "My Board Name";
#else
    doc["board"] = "T-Display ESP32";
#endif

// ❌ Wrong — board name used to gate a feature
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    usbHIDManager.typeString(password); // should be BOARD_HAS_USB_HID instead
#endif
```

---

## 5. Porting Checklist

### Hardware

- [ ] ESP32 or ESP32-S3 SoC confirmed
- [ ] Flash ≥ 4 MB (≥ 16 MB recommended)
- [ ] Two physical buttons available and mapped
- [ ] Display connected and TFT_eSPI / LGFX driver working
- [ ] Deep sleep wake pin identified (bottom button GPIO)
- [ ] PSRAM available? → set `BOARD_HAS_PSRAM`
- [ ] Native USB? → set `BOARD_HAS_USB_HID`

### Code

- [ ] `include/boards/board_myboard.h` created with all constants
- [ ] New environment added to `platformio.ini` with `-DSECURE_LAYER_ENABLED=1`
- [ ] Deep sleep wake pin added in `main.cpp` (3 locations) and `pin_manager.cpp` (1 location)
- [ ] Display geometry offsets checked in `display_manager.cpp`, `splash_manager.cpp`, `main.cpp`
- [ ] Board name added to JSON responses in `web_server.cpp` (3 locations)
- [ ] Factory reset tested (both buttons held 5 seconds on boot)

### Security — do not skip

- [ ] `-DSECURE_LAYER_ENABLED=1` present in build flags
- [ ] `-DDEBUG_BUILD` **absent** from production build flags
- [ ] First boot creates PIN and generates device key (check serial log)
- [ ] Web interface — ECDH key exchange completes (no `KeyExchange failed` in console)
- [ ] AES-256-GCM active — response body unreadable in Wireshark
- [ ] CSRF token verified — requests without `X-CSRF-Token` return 403

### Boot sequence verification (serial log)

```
[INFO] CryptoManager: CTR_DRBG initialized
[INFO] Main: Initializing Secure Layer Manager...
[INFO] Main: Secure Layer Manager initialized successfully
[INFO] WebServer: Server started
```

If `Secure Layer Manager initialized successfully` is missing — ECDH will fail and all requests will return `400 Decryption failed`.

---

## 6. Common Mistakes & Anti-Patterns

### ❌ Defining `DEBUG_BUILD` in production

```ini
; WRONG
build_flags = -DDEBUG_BUILD -DSECURE_LAYER_ENABLED=1
```

`DEBUG_BUILD` exposes test pages and debug endpoints. They are compiled out in production for a reason.

---

### ❌ Removing `SECURE_LAYER_ENABLED` to "simplify"

```ini
; WRONG — disables AES-GCM, ECDH, tunnel decryption
build_flags = -DBOARD_HAS_USB_HID=0
; without -DSECURE_LAYER_ENABLED=1
```

The web interface will still load but all API responses will be plaintext and all encrypted requests will fail with `400 Decryption failed`.

---

### ❌ Using board name flag instead of capability flag

```cpp
// WRONG
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
    usbHIDManager.begin();
#endif

// CORRECT
#ifdef BOARD_HAS_USB_HID
    usbHIDManager.begin();
#endif
```

Using the board name to gate features means your port won't enable USB HID even if your board supports it.

---

### ❌ Forgetting all 4 sleep wake locations

The deep sleep wake pin configuration repeats in 4 places. Missing even one causes the device to not wake from deep sleep on your board:

```
main.cpp  :1208  — pseudo-sleep path
main.cpp  :1533  — auto-lock deep sleep
main.cpp  :1599  — manual sleep
pin_manager.cpp :218 — sleep after PIN lockout
```

---

### ❌ Wrong display dimensions causing UI overflow

If `BOARD_TFT_HEIGHT` is wrong, PIN entry stars and selector will render outside the screen. Always verify with the actual `tft->height()` at runtime:

```cpp
// Temporary debug — remove after verification
LOG_INFO("Board", "Display: " + String(tft->width()) + "x" + String(tft->height()));
```

---

### ❌ Adding endpoints without registering in all 6 places

If you add a new API endpoint during porting, it must be registered in all six locations (see system documentation). Missing even one causes either plaintext exposure in traffic or `400 Decryption failed`.

---

## Contributing Your Port

If your port is working and stable, consider contributing it back:

1. Add `docs/development/boards/myboard.md` with hardware pinout and known limitations
2. Add your board to the build matrix in `platformio.ini`  
3. Open a PR with `[port]` prefix in the title

Please include in the PR:
- Serial log of successful first boot (PIN creation)
- Confirmation that ECDH key exchange completes in browser console
- Wireshark capture showing encrypted responses (bodies unreadable)

---

*Based on SecureGen firmware audit — May 2026*  
*See also: `docs/development/security/SECURITY_OVERVIEW.md`, `docs/development/ENDPOINTS.md`*

> For internal multi-board development rules, see [multi-board.md](./multi-board.md).
