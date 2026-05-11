# API Endpoints

**Last updated:** March 2026
**Adding new endpoints:** see checklist in [`docs/security/security-model.md`](../security/security-model.md#new-endpoint-checklist)

All endpoints operate over HTTP with application-level encryption (AES-256-GCM). Logical paths are mapped to obfuscated hex paths at runtime — use `/api/client/config` to resolve pre-auth paths, and the tunnel endpoint for all others after key exchange.

---

## Security notation

| Symbol | Meaning |
|--------|---------|
| 🔐 | Session cookie required |
| 🛡️ | CSRF token required |
| 🔒 | Transported through encrypted tunnel |

All authenticated endpoints are also URL-obfuscated and method-tunneled unless noted otherwise.

---

## Bootstrap (public)

### GET /api/client/config
No auth required. Returns the three obfuscated paths needed to initiate a session (3 of 38 total mappings). All other mappings are available only after authentication — in production builds, all post-auth API calls are routed through the single tunnel endpoint so the client never needs the full mapping table.

```json
{ "k": "<keyexchange_path>", "t": "<tunnel_path>", "l": "<login_path>" }
```

### GET /api/obfuscation/mappings
Available in DEBUG builds only (`#ifdef DEBUG_BUILD`). No authentication required. Returns all 38 endpoint mappings for development and testing. Not present in production builds.

### POST /api/secure/keyexchange 🔐 🔗
ECDH P-256 key exchange. Client sends ephemeral public key, server responds with its public key. Both sides derive the AES-256 session key independently via HKDF.

Request: `{ "client_id": "...", "client_public_key": "04..." }`  
Response: `{ "server_public_key": "04..." }`

---

## Authentication

### GET /register · GET /login
Public HTML pages. `/register` redirects to `/login` if a user already exists. `/login` redirects to `/` if already authenticated.

### POST /register
Public. Creates the admin account. Only works if no user is registered.  
Request: `{ "username": "...", "password": "..." }`  
Password is hashed with PBKDF2-HMAC-SHA256 (`PBKDF2_ITERATIONS_LOGIN` iterations, ~2.7s).

### POST /login 🔗
Public path (received via bootstrap). Verifies password, creates encrypted session, sets `HttpOnly; SameSite=Strict` cookie.  
Request: `{ "username": "...", "password": "..." }`

### POST /logout 🔐 🔒
Destroys session cookie and deletes the encrypted session file.

---

## TOTP / HOTP Keys

### GET /api/keys 🔐 🔒
Returns list of all keys. Secrets are not included in the response.

```json
{
  "keys": [
    { "name": "GitHub", "type": "T", "algorithm": "SHA1", "digits": 6, "period": 30 }
  ]
}
```

`type`: `"T"` = TOTP, `"H"` = HOTP.

### POST /api/add 🔐 🛡️ 🔒
Adds a new TOTP or HOTP key.

```json
{ "name": "...", "secret": "BASE32...", "type": "T", "algorithm": "SHA1", "digits": 6, "period": 30 }
```

### POST /api/remove 🔐 🛡️ 🔒
Deletes key by index.  
Request: `{ "index": 0 }`

### POST /api/show_qr 🔐 🛡️ 🔒
Triggers QR code display on device screen for 30 seconds. Returns the `otpauth://` URI.  
Request: `{ "index": 0 }`  
Response: `{ "success": true, "uri": "otpauth://totp/..." }`

### POST /api/hotp/generate 🔐 🛡️ 🔒
Increments HOTP counter and returns the new code.  
Request: `{ "index": 0 }`  
Response: `{ "success": true, "code": "123456", "counter": 5 }`

### POST /api/keys/reorder 🔐 🛡️ 🔒
Reorders keys. `order` is an array of current indices in desired order.  
Request: `{ "order": [2, 0, 1] }`

---

## Passwords

### GET /api/passwords 🔐 🔒
Returns password list metadata. Passwords are not included — use `/api/passwords/get` to retrieve a specific entry.

```json
{
  "passwords": [
    {
      "name": "Gmail",
      "category": "web",
      "strength": 2,
      "pw_hash": "a1b2c3d4"
    }
  ]
}
```

Fields: `name` (string), `category` ("web"|"app"|"local"|"key"|""), `strength` (0–3), `pw_hash` (first 8 bytes of SHA-256, hex, for duplicate detection). Passwords are never included in the list response — use `/api/passwords/get`.

### POST /api/passwords/add 🔐 🛡️ 🔒
Request: `{ "name": "...", "password": "...", "category": "web" }`

Note: `category` optional — omit or pass "" for no category.

### POST /api/passwords/get 🔐 🛡️ 🔒
Returns the plaintext password for one entry.  
Request: `{ "index": 0 }`  
Response: `{ "success": true, "password": "..." }`

### POST /api/passwords/update 🔐 🛡️ 🔒
Request: `{ "index": 0, "name": "...", "password": "...", "category": "web" }`

### POST /api/passwords/delete 🔐 🛡️ 🔒
Request: `{ "index": 0 }`

### POST /api/passwords/reorder 🔐 🛡️ 🔒
Reorder the password list.

**Request:** 
```json
{ "order": [2, 0, 1] }
```
Array of original indices in desired new order.

---

## Import / Export

Import/export requires explicit activation first — it is disabled by default.

### POST /api/enable_import_export 🔐 🛡️ 🔒
Enables import/export for 5 minutes.

### GET /api/import_export_status 🔐 🔒
Response: `{ "enabled": true, "expires_in": 240 }`

### POST /api/export 🔐 🛡️ 🔒
Exports encrypted TOTP keys. Uses `PBKDF2_ITERATIONS_EXPORT` for key derivation, AES-256-CBC.  
Request: `{ "password": "..." }`  
Response: `{ "success": true, "encrypted_data": "...", "salt": "..." }`

### POST /api/import 🔐 🛡️ 🔒
Request: `{ "password": "...", "encrypted_data": "...", "salt": "..." }`

### POST /api/passwords/export 🔐 🛡️ 🔒
Same encryption scheme as `/api/export` but for the password store.  
Request: `{ "password": "..." }`

---

## Configuration

### GET /api/config 🔐 🔒
Response: `{ "timeout": 10, "auto_start": false }`  
`timeout` is web server auto-shutdown in minutes.

### GET|POST /api/theme 🔐 (🛡️ on POST) 🔒
Get or set display theme.  
Values: `"dark"`, `"light"`.

### GET|POST /api/display_settings 🔐 (🛡️ on POST) 🔒
Get or set screen timeout and auto lock timeout.

GET response:
```json
{ "display_timeout": 30, "auto_lock_timeout": 300, "dim_timeout": 15 }
```

`display_timeout` — seconds until screen turns off. Valid values: `0`, `15`, `30`, `60`, `300`, `1800`.

`auto_lock_timeout` — seconds until device enters deep sleep and wipes RAM. Valid values: `0`, `300`, `900`, `1800`, `3600`, `14400`.

`dim_timeout` — seconds until display brightness drops to 20% (auto-dim). Valid values: `0` (disabled), `5`, `10`, `15`, `30`, `60`, `300`. Constraint: `dim_timeout < display_timeout < auto_lock_timeout` (when each is non-zero).

POST request: `{ "display_timeout": 30, "auto_lock_timeout": 300, "dim_timeout": 15 }`  
POST response: `{ "success": true, "message": "Display settings saved successfully!", "timeout": 30, "auto_lock_timeout": 300, "dim_timeout": 15 }`

### GET|POST /api/clock_settings 🔐 (🛡️ on POST) 🔒
Get or set POSIX timezone string.  
Example: `{ "timezone": "EST5EDT,M3.2.0,M11.1.0" }`

### GET|POST /api/display/rotation 🔐 (🛡️ on POST) 🔒
Get or set the screen rotation.

**GET response:**
```json
{ "rotation": 1 }
```

**POST request:**
```json
{ "rotation": 3 }
```

Values: `1` = Normal landscape (default, USB on right), `3` = Flipped 180°.  
Full range `0–3` supported via API (0=portrait, 2=portrait inverted) but only 1 and 3 are exposed in the web UI.

Change applies immediately — display redraws and button mappings swap automatically.  
Available on both T-Display ESP32 and T-Display-S3.

### GET|POST /api/ble_settings 🔐 (🛡️ on POST) 🔒
Get or set BLE device name.

### GET|POST /api/mdns_settings 🔐 (🛡️ on POST) 🔒
Get or set mDNS hostname (used as `<hostname>.local`).

### GET|POST /api/session_duration 🔐 (🛡️ on POST) 🔒
Get or set session lifetime in hours.  
Options: until reboot, 1, 6, 24, 72.

### GET|POST /api/boot-mode 🔐 (🛡️ on POST) 🔒
Get or set the default network mode used on boot timeout.

GET response: `{ "boot_mode": "wifi" }`  
POST request: `{ "boot_mode": "wifi" }` — accepted values: `"wifi"`, `"ap"`, `"offline"`.  
POST response: `{ "success": true, "boot_mode": "wifi" }`

The selected mode becomes the timeout default during the boot prompt (2-second window). The other two modes remain selectable via physical buttons. Takes effect on next reboot. Factory default: `"wifi"`.

### GET|POST /api/hid-mode 🔐 🛡️ 🔒
**S3 only.** Get or set the default HID output mode used when sending passwords via hardware keyboard emulation.

GET response: `{ "hid_mode": "ble" }`  
POST request: `{ "hid_mode": "usb" }` — accepted values: `"ble"`, `"usb"`.  
POST response: `{ "success": true, "hid_mode": "usb" }`  
Error: `{ "error": "Invalid hid_mode" }`

The setting determines which mode is pre-selected when the device shows the HID output prompt (triggered by holding both buttons in PASSWORD mode). The user can either wait for the auto-selection based on this default, or press a button to switch to the other mode before transmission begins.

> Note: CSRF token is required on both GET and POST — unlike most read-only GET endpoints.  
> Stored in `config.json` as `default_hid_mode`. Factory default: `"ble"`.

---

## Security Settings

### GET /api/pincode_settings 🔐 🔒
Response: `{ "device_pin_enabled": true, "ble_pin_enabled": false, "pin_length": 6 }`

### POST /api/pincode_settings 🔐 🛡️ 🔒
Enable requires factory reset confirmation. Disable requires physical PIN entry on device.

### POST /api/ble_pin_update 🔐 🛡️ 🔒
Request: `{ "ble_pin_enabled": true, "ble_pin": "123456" }`

### POST /api/duress_pin_update 🔐 🛡️ 🔒
Set or disable the Duress PIN.

**Enable / set:**
```json
{ "duress_pin_enabled": "true", "duress_pin": "123456" }
```

**Disable:**
```json
{ "duress_pin_enabled": "false" }
```

The Duress PIN must be the same length as the current Startup PIN (4–10 digits) and must consist of digits only. When entered at startup, the device shows "PIN OK" and then permanently erases all data (keys, passwords, device key, WiFi, BLE NVS, sessions, config) before restarting.
The entire LittleFS partition (~3.9 MB) is also wiped at the hardware level (`esp_partition_erase_range`) — file recovery via flash reader is not possible.

Status fields returned by `GET /api/pincode_settings`:
- `duressPinEnabled` — `true` / `false`
- `duressPinConfigured` — whether `/duress_pin.hash` exists and is valid

**Response 200:** `{ "success": true, "message": "Duress PIN saved successfully!" }`  
**Response 400:** `{ "success": false, "message": "Duress PIN must be N digits" }`

### POST /api/change_password 🔐 🛡️ 🔒
Request: `{ "current_password": "...", "new_password": "..." }`

### POST /api/wifi_credentials 🔐 🛡️ 🔒

Updates the WiFi client credentials used when connecting to an external network.  
Does not affect the current connection — changes apply after reboot.

Request:
```json
{
  "ssid": "MyNetwork",
  "password": "secret123",
  "confirm_password": "secret123"
}
```

Validation: `ssid` required; `password` must match `confirm_password`; if password  
is non-empty, minimum 8 characters (empty password = open network).

Response 200: `{ "success": true, "message": "WiFi credentials saved. Reboot to apply." }`  
Response 400: `{ "success": false, "message": "..." }` — validation error  
Response 500: `{ "success": false, "message": "Failed to save WiFi credentials" }`

### POST /api/change_ap_password 🔐 🛡️ 🔒
Request: `{ "new_password": "..." }`

---

## Utility

### GET /api/csrf_token 🔐 🔒
Returns CSRF token for the current session.  
Response: `{ "csrf_token": "..." }`

### POST /api/activity 🔐 🔒
Resets the web server auto-shutdown timer. Called periodically by the frontend.

### POST /api/clear_ble_clients 🔐 🛡️ 🔒
Clears all BLE bonded devices.

### GET /api/battery 🔐 🔒
Returns current battery status. Polled by the web UI every 30 seconds.

Response 200:
```json
{ "level": 87, "charging": false }
```

`level` — integer 0–100. Derived from voltage range 3200–3800 mV mapped linearly.  
`charging` — bool. `true` if measured voltage exceeds 4.15 V (threshold-based, no dedicated pin).

Response 503: `{ "error": "Battery manager not available" }`

> Note: CSRF token is not required — this is a read-only GET endpoint. Authentication
> is verified at the tunnel dispatcher outer level, not inside the endpoint handler.

---

## Error responses

```json
{ "success": false, "message": "..." }
```

| Code | When |
|------|------|
| 400 | Invalid input or malformed JSON |
| 401 | Not authenticated |
| 403 | CSRF token missing or invalid |
| 404 | Endpoint not found |
| 500 | Server-side failure |