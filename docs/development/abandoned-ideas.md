# Abandoned Ideas

This document tracks features and approaches that were **never merged into the main codebase**. These ideas were either:
- Prototyped in separate branches and discarded
- Evaluated during planning and rejected before implementation
- Unlikely to be implemented in the future

**Important:** There are no traces of these features in the current project. This document exists to prevent revisiting already-rejected approaches and to document architectural decisions.

## Purpose

Recording abandoned ideas helps:
- Prevent wasting time on already-rejected approaches
- Document why certain features were not implemented
- Provide context for future contributors
- Preserve institutional knowledge about what was tried and failed

---

## Template

When adding an abandoned idea, use this format:

```markdown
### [Idea Name]
**Date:** YYYY-MM-DD  
**Proposed by:** [Name/Role]  
**Status:** Abandoned

**Description:**  
Brief description of the idea.

**Reason for Abandonment:**  
Why this approach was rejected (technical limitations, security concerns, complexity, etc.).

**Alternatives Considered:**  
What was chosen instead.
```

---

## Abandoned Ideas

### FIDO2 / U2F Hardware Security Key
**Date:** 2026-05-19  
**Status:** Abandoned

**Description:**  
Attempted to implement FIDO2/U2F protocol support to turn the device into a hardware security key. The goal was to create a composite USB HID device functioning as both keyboard (for password typing) and FIDO authenticator (for WebAuthn/U2F).

**Technical Approach:**
- Target: ESP32-S3 with native USB-OTG
- USB composite device: HID Keyboard + FIDO HID (CTAPHID protocol)
- Arduino framework

**Reason for Abandonment:**

1. **Composite USB Device Issues:**
   - Could not get composite HID device working reliably on Arduino framework
   - Required low-level USB descriptor manipulation not easily accessible

2. **Protocol Complexity:**
   - CTAPHID protocol implementation proved more complex than anticipated
   - Packet fragmentation, timing requirements, state machine management

**Alternatives Considered:**

1. **BLE FIDO (CTAP2.1)** — rejected due to additional complexity
2. **TOTP-only approach (current)** — chosen solution, simpler and works offline
3. **ESP-IDF rewrite** — would allow full USB control but requires abandoning Arduino

**Conclusion:**  
FIDO2 deserves a dedicated project. Current focus remains on robust TOTP/HOTP + Password Manager.

---

### Native HTTPS Web Server
**Date:** 2026-05-19  
**Status:** Abandoned

**Description:**  
Attempted to implement native HTTPS (TLS/SSL) for the web management interface using standard ESP32 TLS libraries. Goal was to provide browser-trusted encryption with proper certificates.

**Technical Approach:**
- ESPAsyncWebServer with mbedTLS integration
- Self-signed certificates generated on device
- Standard HTTPS on port 443

**Reason for Abandonment:**

1. **Memory Constraints on ESP32 (non-S3):**
   - TLS handshake requires ~40–50KB RAM for buffers
   - T-Display ESP32 has limited heap (~100KB free after boot)
   - Concurrent TLS sessions would cause out-of-memory crashes
   - Web interface requires multiple simultaneous connections (HTML, CSS, JS, API calls)

2. **Certificate Trust Issues:**
   - Self-signed certificates trigger browser warnings
   - No way to pre-install CA certificate on user devices
   - Users would need to manually accept certificate on every connection

3. **Arduino Framework Limitations:**
   - ESPAsyncWebServer TLS support incomplete
   - Required patching underlying libraries
   - Unstable behavior with concurrent connections

**Alternatives Considered:**

1. **HTTPS on S3 only** — rejected to maintain feature parity between boards
2. **External reverse proxy** — rejected, defeats "runs on device" principle
3. **Custom encryption layer (current)** — chosen solution

**Chosen Solution:**  
Implemented 8-layer custom security system instead of native HTTPS:
- ECDH P-256 key exchange
- AES-256-GCM transport encryption
- URL obfuscation with epoch rotation
- Header obfuscation
- Decoy traffic generation
- Method tunneling (POST-as-GET)
- Timing attack protection
- Honeypot endpoints

Works on both ESP32 and S3, no memory issues, no certificate warnings. Trade-off: not browser-native HTTPS, but provides equivalent encryption without TLS overhead.

---

### Cryptocurrency Wallet Module
**Date:** 2026-05-19  
**Status:** Deferred / Abandoned

**Description:**  
Considered adding cryptocurrency wallet functionality (private key storage, transaction signing) as a separate module for T-Display-S3 only. Would be isolated from main TOTP/Password Manager code with minimal integration points.

**Technical Approach:**
- S3-only feature (PSRAM requirement for crypto libraries)
- Separate firmware build or runtime module
- Support for BSC-20
- QR and USB HID code transaction signing workflow

**Reason for Deferral/Abandonment:**

1. **Security Philosophy Conflict:**
   - Best practice: crypto keys should be on dedicated hardware wallet
   - Mixing password manager + crypto wallet on same device increases attack surface
   - Single compromise exposes both password vault and crypto assets

2. **Scope Creep:**
   - Would require blockchain libraries (secp256k1, keccak, etc.)
   - Transaction parsing and validation logic

3. **Hardware Limitations:**
   - No secure element for key isolation

4. **Focus Dilution:**
   - Core competency is TOTP/Password Manager
   - Crypto wallet is a separate product category
   - Better served by dedicated hardware wallets (Ledger, Trezor)

**Alternatives Considered:**

1. **Separate firmware variant** — rejected, maintenance burden
2. **Plugin architecture** — rejected, adds complexity to core
3. **No crypto wallet (current)** — chosen, focus on core functionality

**Conclusion:**  
Cryptocurrency keys deserve dedicated hardware. Current focus remains on TOTP/Password Manager. Users should use purpose-built hardware wallets for crypto assets.

---

### U2F over BLE
**Date:** 2026-05-19  
**Status:** Abandoned

**Description:**  
Considered implementing U2F (Universal 2nd Factor) protocol over Bluetooth Low Energy for both ESP32 and S3 boards. Would allow the device to act as a wireless security key for website authentication.

**Technical Approach:**
- U2F protocol over BLE (FIDO U2F 1.2 specification)
- Works on both T-Display ESP32 and S3 (BLE available on both)
- Button press for user presence confirmation

**Reason for Abandonment:**

1. **Protocol Deprecated:**
   - U2F officially deprecated in favor of FIDO2/WebAuthn
   - Most modern websites no longer support U2F
   - Chrome, Firefox, Edge prioritize FIDO2/CTAP2

2. **Limited Browser Support:**
   - U2F over BLE never gained widespread adoption
   - Most implementations are USB-only
   - Poor compatibility with mobile devices

3. **Obsolete Technology:**
   - Implementing deprecated protocol provides little value
   - Time better spent on current standards (TOTP already implemented)

**Alternatives Considered:**

1. **FIDO2 over BLE (CTAP2.1)** — rejected, too complex (see FIDO2/U2F entry above)
2. **TOTP-only (current)** — chosen, widely supported and simpler

**Conclusion:**  
U2F is obsolete. TOTP provides better compatibility and doesn't require protocol-specific browser support.

---

### Vibration Motor Feedback
**Date:** 2026-05-19  
**Status:** Abandoned

**Description:**  
Considered adding vibration motor for haptic feedback on button presses, successful authentication, and alerts. Would require external motor module connected via GPIO.

**Technical Approach:**
- Small vibration motor (3V coin type or linear actuator)
- PWM control via GPIO
- Optional module, user-installable
- Configurable patterns for different events

**Reason for Abandonment:**

1. **Power Consumption:**
   - Vibration motor draws 50–100mA during operation
   - Significantly reduces battery life
   - Defeats stealth/silent operation use case

2. **Hardware Complexity:**
   - Requires additional GPIO pin
   - Needs transistor driver circuit (motor current exceeds GPIO limit)
   - User must solder/connect external module
   - Increases assembly difficulty

3. **Use Case Conflict:**
   - Device designed for discreet use (pocket, desk)
   - Vibration draws attention, opposite of stealth
   - Screen feedback already sufficient

4. **Project Scope:**
   - Would require separate hardware module design
   - Configuration UI for vibration patterns
   - Power management integration
   - Testing across different motor types

**Alternatives Considered:**

1. **Optional module with enable/disable** — rejected, still adds complexity
2. **Visual feedback only (current)** — chosen, sufficient and power-efficient
3. **Audio beeper** — rejected, even less stealthy than vibration

**Conclusion:**  
Visual feedback (screen) is sufficient. Vibration motor conflicts with low-power and stealth operation goals.

---

### ESP32 Light Sleep (esp_light_sleep_start)
**Date:** 2026-05-19  
**Status:** Abandoned

**Description:**  
Attempted to use native ESP32 light sleep mode (`esp_light_sleep_start()`) for power saving during screen timeout. Would allow CPU to enter low-power state while maintaining RAM and waking on button press.

**Technical Approach:**
- `esp_light_sleep_start()` API from ESP-IDF
- Wake on GPIO (Button 2 press)
- Automatic CPU frequency reduction
- RAM retention during sleep

**Reason for Abandonment:**

**Hardware Incompatibility with Battery Power:**
- Light sleep causes **POWER_ON reset** when running on battery
- Root cause: voltage drop on CPU wake
  - CPU wakes from light sleep → current spike
  - Battery internal resistance + board power rail → voltage sag
  - Voltage drops below brownout threshold
  - ESP32 resets instead of resuming
- Issue does not occur on USB power (stable 5V rail)
- Would require hardware modification (bulk capacitor on power rail)

**Chosen Solution — Custom "Pseudo-Sleep":**

Implemented custom low-power mode without using `esp_light_sleep_start()`:

1. **Reduce CPU frequency:** `setCpuFrequencyMhz(40)` — 240MHz → 40MHz
2. **Turn off display:** `displayManager.turnOff()` + TFT sleep mode
3. **Polling loop:** Check buttons every 100ms (no GPIO interrupt wake)
4. **Wake on button press:** Any button restores 240MHz and turns display on
5. **Auto-lock support:** Deep sleep after timeout if configured

**Benefits:**
- No voltage drop issues on battery power
- Stable wake behavior
- Still achieves significant power savings (~60% reduction)
- Works identically on USB and battery

**Trade-offs:**
- CPU not fully asleep (40MHz polling vs true light sleep)
- Slightly higher power consumption than true light sleep
- But: reliable operation on battery is more important

**Conclusion:**  
Native light sleep incompatible with battery operation due to hardware limitations. Custom pseudo-sleep provides reliable power saving without reset issues.

---

### Web-Based Log Viewer
**Date:** 2026-05-19  
**Status:** Abandoned

**Description:**  
Considered adding a log viewer page in the web cabinet to display device logs (debug, info, warnings, errors) in real-time or from a buffer. Would help with remote debugging and troubleshooting.

**Technical Approach:**
- Circular buffer in RAM for recent log entries
- Web endpoint `/api/logs` returning JSON array
- Auto-refresh log viewer page in web interface
- Optional log level filtering

**Reason for Abandonment:**

**Web Server Already Logs Everything:**
- ESPAsyncWebServer logs all requests to Serial
- Includes: endpoints, methods, response codes, timing
- LogManager already outputs to Serial Monitor
- Redundant to duplicate logs to web interface

**Memory Constraints:**
- Circular log buffer would consume 5–10KB RAM
- Each log entry: timestamp + component + message (~100–200 bytes)
- Limited benefit on ESP32 (tight memory budget)

**Security Concerns:**
- Logs may contain sensitive debugging information
- Endpoint paths, timing data, error messages
- Additional attack surface for information disclosure

**Alternative — Serial Monitor (Current):**
- Connect via USB, open Serial Monitor in PlatformIO/Arduino IDE
- Full log history with timestamps
- No memory overhead
- No security risk

**Conclusion:**  
Serial Monitor provides better logging experience without memory overhead or security risks. Web-based logs are redundant.

---

### Password Age Tracking (Time Since Added)
**Date:** 2026-05-19  
**Status:** Abandoned

**Description:**  
Considered adding "time since added" or "password age" display in Password Manager mode. Would show how long ago each password was created (e.g., "Added 45 days ago") to help users identify old passwords that need rotation.

**Technical Approach:**
- Store `created_at` timestamp (Unix time) with each password entry
- Display relative time on device screen and in web interface
- Optional "old password" warning after configurable threshold (e.g., 90 days)

**Reason for Abandonment:**

**Time Sync Unreliability:**

Device knows real time only in specific scenarios:
1. **WiFi mode** — after NTP sync (but WiFi disconnects after sync for power saving)
2. **RTC module enabled** — if DS3231 is connected and calibrated
3. **Web cabinet open** — client browser can send current time

Most common use cases have **no reliable time source:**
- **AP mode** — no NTP, system clock starts at epoch 0
- **Offline mode** — no network, no time sync
- **After deep sleep** — system clock resets to epoch 0
- **Without RTC module** — majority of users don't have DS3231

**Result:**
- Feature would work only in minority of cases (WiFi + NTP, or RTC module)
- Would show incorrect timestamps most of the time
- "Added 54 years ago" (epoch 0 bug) worse than no timestamp
- More exceptions than working cases

**Alternatives Considered:**

1. **Relative counter (boot cycles)** — rejected, meaningless metric
2. **Manual "last changed" field** — rejected, user burden
3. **No timestamp (current)** — chosen, avoids incorrect data

**Conclusion:**  
Reliable time tracking requires consistent time source. Device operates offline too often for timestamps to be useful. Feature would confuse users more than help.

---

## Contributing

When documenting an abandoned idea:
1. Be objective and factual
2. Include technical reasoning
3. Reference related issues/PRs if applicable
4. Update the date and status
5. Keep descriptions concise but complete
