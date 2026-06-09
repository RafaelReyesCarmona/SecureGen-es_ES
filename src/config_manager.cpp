#include "config_manager.h"
#include "config.h"
#include "log_manager.h"
#include "crypto_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

ConfigManager::ConfigManager() {
    // Constructor
}

void ConfigManager::begin() {
    // No specific begin logic needed for now, LittleFS is initialized elsewhere
}

Theme ConfigManager::loadTheme() {
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                String themeStr = doc[THEME_CONFIG_KEY] | "dark"; // Default to "dark"
                LOG_INFO("ConfigManager", "Loaded theme: " + themeStr);
                if (themeStr == "light") {
                    _currentTheme = Theme::LIGHT;
                } else {
                    _currentTheme = Theme::DARK;
                }
                LOG_INFO("ConfigManager", "Theme set to: " + String((_currentTheme == Theme::LIGHT) ? "LIGHT" : "DARK"));
            } else {
                LOG_ERROR("ConfigManager", "Failed to deserialize config file: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_ERROR("ConfigManager", "Failed to open config file for reading");
        }
    } else {
        LOG_INFO("ConfigManager", "Config file does not exist. Using default theme");
    }
    return _currentTheme;
}

void ConfigManager::saveTheme(Theme theme) {
    LOG_INFO("ConfigManager", "saveTheme() called with theme: " + String((theme == Theme::LIGHT) ? "LIGHT" : "DARK"));
    _currentTheme = theme;
    JsonDocument doc;

    // Load existing config to preserve other settings
    if (LittleFS.exists(CONFIG_FILE)) {
        LOG_DEBUG("ConfigManager", "Config file exists. Loading existing settings...");
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            deserializeJson(doc, configFile);
            configFile.close();
            LOG_DEBUG("ConfigManager", "Existing config loaded");
        } else {
            LOG_WARNING("ConfigManager", "Failed to open config file for reading during save. Creating new doc");
        }
    } else {
        LOG_DEBUG("ConfigManager", "Config file does not exist. Creating new doc");
    }

    doc[THEME_CONFIG_KEY] = (theme == Theme::LIGHT) ? "light" : "dark"; // Use lowercase

    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        serializeJson(doc, configFile);
        configFile.close();
        LOG_INFO("ConfigManager", "Theme saved successfully to config file");
    } else {
        LOG_ERROR("ConfigManager", "Failed to open config file for writing");
    }
}

String ConfigManager::loadBleDeviceName() {
    if (LittleFS.exists(BLE_CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(BLE_CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                String deviceName = doc["device_name"] | DEFAULT_BLE_DEVICE_NAME; // Default
                LOG_INFO("ConfigManager", "Loaded BLE device name: " + deviceName);
                _currentBleDeviceName = deviceName;
                configFile.close();
                return deviceName;
            } else {
                LOG_ERROR("ConfigManager", "Failed to deserialize BLE config file: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_ERROR("ConfigManager", "Failed to open BLE config file for reading");
        }
    } else {
        LOG_INFO("ConfigManager", "BLE config file does not exist. Using default name");
    }
    _currentBleDeviceName = DEFAULT_BLE_DEVICE_NAME;
    return _currentBleDeviceName;
}

void ConfigManager::saveBleDeviceName(const String& deviceName) {
    LOG_INFO("ConfigManager", "saveBleDeviceName() called with name: " + deviceName);
    _currentBleDeviceName = deviceName;
    JsonDocument doc;
    doc["device_name"] = deviceName;

    fs::File configFile = LittleFS.open(BLE_CONFIG_FILE, "w");
    if (configFile) {
        serializeJson(doc, configFile);
        configFile.close();
        LOG_INFO("ConfigManager", "BLE device name saved successfully to config file");
    } else {
        LOG_ERROR("ConfigManager", "Failed to open BLE config file for writing");
    }
}

String ConfigManager::getStartupMode() {
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                String mode = doc["startup_mode"] | "totp"; // Default to TOTP
                configFile.close();
                LOG_INFO("ConfigManager", "Loaded startup mode: " + mode);
                return mode;
            } else {
                LOG_ERROR("ConfigManager", "Failed to parse startup mode config: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_ERROR("ConfigManager", "Failed to open config file for startup mode reading");
        }
    } else {
        LOG_INFO("ConfigManager", "No config file found. Using default startup mode: totp");
    }
    return "totp"; // Default fallback
}

bool ConfigManager::saveStartupMode(const String& mode) {
    LOG_INFO("ConfigManager", "saveStartupMode() called with mode: " + mode);
    JsonDocument doc;
    
    // Load existing config to preserve other settings
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "Failed to parse existing config, creating new: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_WARNING("ConfigManager", "Failed to open existing config file for reading");
        }
    }
    
    doc["startup_mode"] = mode;
    
    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        size_t bytesWritten = serializeJson(doc, configFile);
        configFile.close();
        if (bytesWritten > 0) {
            LOG_INFO("ConfigManager", "Startup mode saved successfully");
            return true;
        } else {
            LOG_ERROR("ConfigManager", "Failed to write startup mode config data");
            return false;
        }
    } else {
        LOG_ERROR("ConfigManager", "Failed to open config file for startup mode writing");
        return false;
    }
}

#ifdef BOARD_HAS_USB_HID
String ConfigManager::getDefaultHidMode() {
    // Try per-space HID mode file first
    String hidModePath = CryptoManager::getInstance().getSpacePath("hid_mode");
    
    if (LittleFS.exists(hidModePath)) {
        fs::File hf = LittleFS.open(hidModePath, "r");
        if (hf) {
            JsonDocument hdoc;
            if (deserializeJson(hdoc, hf) == DeserializationError::Ok) {
                String mode = hdoc["hid_mode"] | "";
                if (mode == "ble" || mode == "usb") {
                    hf.close();
                    LOG_INFO("ConfigManager", "Loaded HID mode from per-space file: " + mode);
                    return mode;
                }
            }
            hf.close();
        }
    }
    
    // Fallback to global config for backward compatibility
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            if (deserializeJson(doc, configFile) == DeserializationError::Ok) {
                String mode = doc["default_hid_mode"] | "ble";
                configFile.close();
                LOG_INFO("ConfigManager", "Loaded default HID mode from global config: " + mode);
                return mode;
            }
            configFile.close();
        }
    }
    return "ble";
}

bool ConfigManager::saveDefaultHidMode(const String& mode) {
    LOG_INFO("ConfigManager", "saveDefaultHidMode() called with mode: " + mode);
    
    // Save to per-space file
    String hidModePath = CryptoManager::getInstance().getSpacePath("hid_mode");
    JsonDocument hdoc;
    hdoc["hid_mode"] = mode;
    
    fs::File hf = LittleFS.open(hidModePath, "w");
    if (hf) {
        size_t bytesWritten = serializeJson(hdoc, hf);
        hf.close();
        
        if (bytesWritten > 0) {
            LOG_INFO("ConfigManager", "HID mode saved to per-space file: " + mode);
            
            // Also update global config for Space A (backward compatibility)
            if (CryptoManager::getInstance().getActiveSpace() == ActiveSpace::A) {
                JsonDocument doc;
                if (LittleFS.exists(CONFIG_FILE)) {
                    fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
                    if (configFile) {
                        deserializeJson(doc, configFile);
                        configFile.close();
                    }
                }
                doc["default_hid_mode"] = mode;
                fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
                if (configFile) {
                    serializeJson(doc, configFile);
                    configFile.close();
                }
            }
            
            return true;
        }
    }
    return false;
}
#endif

String ConfigManager::getBootMode() {
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                String mode = doc["boot_mode"] | "wifi";
                configFile.close();
                LOG_INFO("ConfigManager", "Loaded boot mode: " + mode);
                return mode;
            } else {
                LOG_ERROR("ConfigManager", "Failed to parse boot mode config: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_ERROR("ConfigManager", "Failed to open config file for boot mode reading");
        }
    } else {
        LOG_INFO("ConfigManager", "No config file found. Using default boot mode: wifi");
    }
    return "wifi";
}

bool ConfigManager::saveBootMode(const String& mode) {
    LOG_INFO("ConfigManager", "saveBootMode() called with mode: " + mode);
    JsonDocument doc;
    
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "Failed to parse existing config, creating new: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_WARNING("ConfigManager", "Failed to open existing config file for reading");
        }
    }
    
    doc["boot_mode"] = mode;
    
    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        size_t bytesWritten = serializeJson(doc, configFile);
        configFile.close();
        if (bytesWritten > 0) {
            LOG_INFO("ConfigManager", "Boot mode saved successfully");
            return true;
        } else {
            LOG_ERROR("ConfigManager", "Failed to write boot mode config data");
            return false;
        }
    } else {
        LOG_ERROR("ConfigManager", "Failed to open config file for boot mode writing");
        return false;
    }
}

uint8_t ConfigManager::getDisplayRotation() {
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                uint8_t rotation = doc["display_rotation"] | 1; // Default to 1 (landscape)
                configFile.close();
                return rotation;
            } else {
                LOG_ERROR("ConfigManager", "Failed to parse display rotation config: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_ERROR("ConfigManager", "Failed to open config file for display rotation reading");
        }
    }
    return 1; // Default: rotation 1 (landscape, USB on right)
}

bool ConfigManager::saveDisplayRotation(uint8_t rotation) {
    LOG_INFO("ConfigManager", "saveDisplayRotation() called with rotation: " + String(rotation));
    JsonDocument doc;
    
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "Failed to parse existing config, creating new: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_WARNING("ConfigManager", "Failed to open existing config file for reading");
        }
    }
    
    doc["display_rotation"] = rotation;
    
    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        size_t bytesWritten = serializeJson(doc, configFile);
        configFile.close();
        if (bytesWritten > 0) {
            LOG_INFO("ConfigManager", "Display rotation saved successfully");
            return true;
        } else {
            LOG_ERROR("ConfigManager", "Failed to write display rotation config data");
            return false;
        }
    } else {
        LOG_ERROR("ConfigManager", "Failed to open config file for display rotation writing");
        return false;
    }
}



uint16_t ConfigManager::getWebServerTimeout() {
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                uint16_t timeout = doc["web_server_timeout"] | 10; // Default to 10 minutes
                configFile.close();
                LOG_INFO("ConfigManager", "Loaded web server timeout: " + String(timeout) + " minutes");
                return timeout;
            } else {
                LOG_ERROR("ConfigManager", "Failed to parse web server timeout config: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_ERROR("ConfigManager", "Failed to open config file for web server timeout reading");
        }
    } else {
        LOG_INFO("ConfigManager", "No config file found. Using default web server timeout: 10 minutes");
    }
    return 10; // Default fallback
}

void ConfigManager::setWebServerTimeout(uint16_t timeout) {
    LOG_INFO("ConfigManager", "setWebServerTimeout() called with value: " + String(timeout) + " minutes");
    JsonDocument doc;
    
    // Load existing config to preserve other settings
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "Failed to parse existing config, creating new: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_WARNING("ConfigManager", "Failed to open existing config file for reading");
        }
    }
    
    doc["web_server_timeout"] = timeout;
    
    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        size_t bytesWritten = serializeJson(doc, configFile);
        configFile.close();
        if (bytesWritten > 0) {
            LOG_INFO("ConfigManager", "Web server timeout saved successfully");
        } else {
            LOG_ERROR("ConfigManager", "Failed to write web server timeout to config file");
        }
    } else {
        LOG_ERROR("ConfigManager", "Failed to open config file for web server timeout writing");
    }
}

bool ConfigManager::getWebServerAutoStart() {
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                bool autoStart = doc["web_server_auto_start"] | false; // Default to false
                configFile.close();
                LOG_INFO("ConfigManager", "Loaded web server auto-start: " + String(autoStart ? "true" : "false"));
                return autoStart;
            } else {
                LOG_ERROR("ConfigManager", "Failed to parse web server auto-start config: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_ERROR("ConfigManager", "Failed to open config file for web server auto-start reading");
        }
    } else {
        LOG_INFO("ConfigManager", "No config file found. Using default web server auto-start: false");
    }
    return false; // Default fallback
}

void ConfigManager::setWebServerAutoStart(bool autoStart) {
    LOG_INFO("ConfigManager", "setWebServerAutoStart() called with value: " + String(autoStart ? "true" : "false"));
    JsonDocument doc;

    // Load existing config to preserve other settings
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "Failed to parse existing config, creating new: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_WARNING("ConfigManager", "Failed to open existing config file for reading");
        }
    }

    doc["web_server_auto_start"] = autoStart;

    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        size_t bytesWritten = serializeJson(doc, configFile);
        configFile.close();
        if (bytesWritten > 0) {
            LOG_INFO("ConfigManager", "Web server auto-start saved successfully");
        } else {
            LOG_ERROR("ConfigManager", "Failed to write web server auto-start config data");
        }
    } else {
        LOG_ERROR("ConfigManager", "Failed to open config file for web server auto-start writing");
    }
}

String ConfigManager::loadMdnsHostname() {
    if (LittleFS.exists(MDNS_CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(MDNS_CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                String hostname = doc["hostname"] | DEFAULT_MDNS_HOSTNAME; // Default
                LOG_INFO("ConfigManager", "Loaded mDNS hostname: " + hostname);
                _currentMdnsHostname = hostname;
                configFile.close();
                return hostname;
            } else {
                LOG_ERROR("ConfigManager", "Failed to deserialize mDNS config file: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_ERROR("ConfigManager", "Failed to open mDNS config file for reading");
        }
    } else {
        LOG_INFO("ConfigManager", "mDNS config file does not exist. Using default name");
    }
    _currentMdnsHostname = DEFAULT_MDNS_HOSTNAME;
    return _currentMdnsHostname;
}

void ConfigManager::saveMdnsHostname(const String& hostname) {
    LOG_INFO("ConfigManager", "saveMdnsHostname() called with name: " + hostname);
    _currentMdnsHostname = hostname;
    JsonDocument doc;
    doc["hostname"] = hostname;

    fs::File configFile = LittleFS.open(MDNS_CONFIG_FILE, "w");
    if (configFile) {
        serializeJson(doc, configFile);
        configFile.close();
        LOG_INFO("ConfigManager", "mDNS hostname saved successfully to config file");
    } else {
        LOG_ERROR("ConfigManager", "Failed to open mDNS config file for writing");
    }
}

uint16_t ConfigManager::getDisplayTimeout() {
    // Use cached value if available
    if (_displayTimeoutCached) {
        return _cachedDisplayTimeout;
    }

    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                uint16_t timeout = doc["display_timeout"] | 30; // Default to 30 seconds
                configFile.close();
                // Cache the value
                _cachedDisplayTimeout = timeout;
                _displayTimeoutCached = true;
                LOG_INFO("ConfigManager", "Loaded display timeout: " + String(timeout) + " seconds");
                return timeout;
            } else {
                LOG_ERROR("ConfigManager", "Failed to parse display timeout config: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_ERROR("ConfigManager", "Failed to open config file for display timeout reading");
        }
    } else {
        LOG_INFO("ConfigManager", "No config file found. Using default display timeout: 30 seconds");
    }
    // Cache the default value
    _cachedDisplayTimeout = 30;
    _displayTimeoutCached = true;
    return 30; // Default fallback
}

String ConfigManager::getTimezone() {
    LOG_INFO("ConfigManager", "getTimezone() reading from global config: " + String(CONFIG_FILE));
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            String fileContent = configFile.readString();
            LOG_INFO("ConfigManager", "getTimezone() file content: " + fileContent);
            configFile.close();
            
            // Re-open for deserialization
            configFile = LittleFS.open(CONFIG_FILE, "r");
            if (configFile) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, configFile);
                configFile.close();
                if (error == DeserializationError::Ok) {
                    String tz = doc["timezone"] | "UTC0";
                    LOG_INFO("ConfigManager", "getTimezone() successfully loaded timezone: " + tz);
                    return tz;
                } else {
                    LOG_ERROR("ConfigManager", "getTimezone() failed to deserialize config: " + String(error.c_str()));
                }
            } else {
                LOG_ERROR("ConfigManager", "getTimezone() failed to re-open config file for parsing");
            }
        } else {
            LOG_ERROR("ConfigManager", "getTimezone() failed to open config file for reading");
        }
    } else {
        LOG_INFO("ConfigManager", "getTimezone() config file does not exist, using default UTC0");
    }
    return "UTC0";
}

bool ConfigManager::saveTimezone(const String& tz) {
    LOG_INFO("ConfigManager", "saveTimezone() called with timezone: " + tz);
    JsonDocument doc;
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            configFile.close();
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "saveTimezone() failed to parse existing config: " + String(error.c_str()));
            }
        }
    }
    
    doc["timezone"] = tz;
    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile) {
        LOG_ERROR("ConfigManager", "saveTimezone() failed to open config file for writing");
        return false;
    }
    
    size_t bytesWritten = serializeJson(doc, configFile);
    configFile.close();
    
    if (bytesWritten == 0) {
        LOG_ERROR("ConfigManager", "saveTimezone() failed to serialize/write config");
        return false;
    }
    
    // Log the new contents of the file
    configFile = LittleFS.open(CONFIG_FILE, "r");
    if (configFile) {
        String fileContent = configFile.readString();
        LOG_INFO("ConfigManager", "saveTimezone() verified file content after save: " + fileContent);
        configFile.close();
    }
    
    LOG_INFO("ConfigManager", "saveTimezone() successfully saved timezone: " + tz);
    return true;
}


bool ConfigManager::saveDisplayTimeout(uint16_t timeout) {
    LOG_INFO("ConfigManager", "saveDisplayTimeout() called with value: " + String(timeout) + " seconds");

    // Invalidate cache since we're changing the value
    _displayTimeoutCached = false;

    JsonDocument doc;

    // Load existing config to preserve other settings
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "Failed to parse existing config, creating new: " + String(error.c_str()));
            }
            configFile.close();
        } else {
            LOG_WARNING("ConfigManager", "Failed to open existing config file for reading");
        }
    }

    doc["display_timeout"] = timeout;

    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        size_t bytesWritten = serializeJson(doc, configFile);
        configFile.close();
        if (bytesWritten > 0) {
            LOG_INFO("ConfigManager", "Display timeout saved successfully");
            return true;
        } else {
            LOG_ERROR("ConfigManager", "Failed to write display timeout to config file");
            return false;
        }
    } else {
        LOG_ERROR("ConfigManager", "Failed to open config file for display timeout writing");
        return false;
    }
}

uint32_t ConfigManager::getAutoLockTimeout() {
    if (_autoLockTimeoutCached) {
        return _cachedAutoLockTimeout;
    }

    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                uint32_t timeout = doc["auto_lock_timeout"] | 0;
                configFile.close();
                _cachedAutoLockTimeout = timeout;
                _autoLockTimeoutCached = true;
                LOG_INFO("ConfigManager", "Loaded auto lock timeout: " + String(timeout) + " seconds");
                return timeout;
            }
            configFile.close();
        }
    }

    _cachedAutoLockTimeout = 0;
    _autoLockTimeoutCached = true;
    LOG_INFO("ConfigManager", "No config file found. Using default auto lock timeout: Never");
    return 0;
}

bool ConfigManager::saveAutoLockTimeout(uint32_t timeout) {
    LOG_INFO("ConfigManager", "saveAutoLockTimeout() called with value: " + String(timeout) + " seconds");

    _autoLockTimeoutCached = false;

    JsonDocument doc;

    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "Failed to parse existing config, creating new: " + String(error.c_str()));
            }
            configFile.close();
        }
    }

    doc["auto_lock_timeout"] = timeout;

    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        size_t bytesWritten = serializeJson(doc, configFile);
        configFile.close();
        if (bytesWritten > 0) {
            LOG_INFO("ConfigManager", "Auto lock timeout saved successfully");
            return true;
        } else {
            LOG_ERROR("ConfigManager", "Failed to write auto lock timeout to config file");
            return false;
        }
    } else {
        LOG_ERROR("ConfigManager", "Failed to open config file for auto lock timeout writing");
        return false;
    }
}

uint16_t ConfigManager::getDimTimeout() {
    if (_dimTimeoutCached) {
        return _cachedDimTimeout;
    }
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, configFile);
            if (error == DeserializationError::Ok) {
                uint16_t timeout = doc["dim_timeout"] | 0;
                configFile.close();
                _cachedDimTimeout = timeout;
                _dimTimeoutCached = true;
                LOG_INFO("ConfigManager", "Loaded dim timeout: " + String(timeout) + " seconds");
                return timeout;
            }
            configFile.close();
        }
    }
    _cachedDimTimeout = 0;
    _dimTimeoutCached = true;
    return 0; // Default: Never dim
}

bool ConfigManager::saveDimTimeout(uint16_t timeout) {
    LOG_INFO("ConfigManager", "saveDimTimeout() called with value: " + String(timeout) + " seconds");

    _dimTimeoutCached = false;

    JsonDocument doc;

    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            deserializeJson(doc, configFile);
            configFile.close();
        }
    }

    doc["dim_timeout"] = timeout;

    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        size_t bytesWritten = serializeJson(doc, configFile);
        configFile.close();
        return bytesWritten > 0;
    }
    return false;
}

// Session Duration Configuration
ConfigManager::SessionDuration ConfigManager::getSessionDuration() {

    fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile) {
        LOG_INFO("ConfigManager", "Session duration config not found, using default: 6 hours");
        _currentSessionDuration = SIX_HOURS;
        return _currentSessionDuration;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error != DeserializationError::Ok) {
        LOG_WARNING("ConfigManager", "Failed to parse session duration config: " + String(error.c_str()));
        _currentSessionDuration = SIX_HOURS;
        return _currentSessionDuration;
    }

    int durationValue = doc["session_duration"] | SIX_HOURS;
    
    // Validate duration value
    switch(durationValue) {
        case UNTIL_REBOOT:
        case ONE_HOUR:
        case SIX_HOURS:
        case TWENTY_FOUR_HOURS:
        case THREE_DAYS:
            _currentSessionDuration = static_cast<SessionDuration>(durationValue);
            break;
        default:
            LOG_WARNING("ConfigManager", "Invalid session duration value: " + String(durationValue) + ", using default");
            _currentSessionDuration = SIX_HOURS;
    }

    LOG_INFO("ConfigManager", "Loaded session duration: " + String(_currentSessionDuration) + " hours");
    return _currentSessionDuration;
}

void ConfigManager::setSessionDuration(SessionDuration duration) {
    
    DynamicJsonDocument doc(1024);
    
    // Load existing config
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "Failed to parse existing config for session duration: " + String(error.c_str()));
            }
            configFile.close();
        }
    }

    doc["session_duration"] = static_cast<int>(duration);

    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (configFile) {
        size_t bytesWritten = serializeJson(doc, configFile);
        configFile.close();
        if (bytesWritten > 0) {
            _currentSessionDuration = duration;
            LOG_INFO("ConfigManager", "Session duration saved successfully: " + String(duration) + " hours");
        } else {
            LOG_ERROR("ConfigManager", "Failed to write session duration to config file");
        }
    } else {
        LOG_ERROR("ConfigManager", "Failed to open config file for session duration writing");
    }
}

unsigned long ConfigManager::getSessionLifetimeSeconds() {
    SessionDuration duration = getSessionDuration();
    
    switch(duration) {
        case UNTIL_REBOOT:
            return 0; // Special case - handled separately in validation
        case ONE_HOUR:
            return 3600; // 1 hour
        case SIX_HOURS:
            return 21600; // 6 hours  
        case TWENTY_FOUR_HOURS:
            return 86400; // 24 hours
        case THREE_DAYS:
            return 259200; // 3 days
        default:
            return 21600; // Default 6 hours
    }
}

// 📡 WiFi AP Password Configuration
String ConfigManager::loadApPassword() {
    LOG_DEBUG("ConfigManager", "loadApPassword() called");

    fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
    if (!configFile) {
        LOG_WARNING("ConfigManager", "Config file not found, returning default AP password");
        return "12345678"; // Пароль по умолчанию (min 8 chars для WiFi)
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        LOG_ERROR("ConfigManager", "Failed to parse config file: " + String(error.c_str()));
        return "12345678";
    }

    String apPassword = doc["apPassword"] | "";
    
    if (apPassword.isEmpty()) {
        // Нет сохраненного пароля, возвращаем дефолт
        return "12345678";
    }
    
    // Validate password length (WiFi AP requires 8-63 chars)
    if (apPassword.length() < 8 || apPassword.length() > 63) {
        LOG_WARNING("ConfigManager", "Invalid AP password length, using default");
        return "12345678";
    }
    
    LOG_INFO("ConfigManager", "AP Password loaded: " + String(apPassword.length()) + " chars");
    return apPassword;
}

bool ConfigManager::saveApPassword(const String& password) {
    LOG_INFO("ConfigManager", "saveApPassword() called");

    JsonDocument doc;
    
    // Загружаем существующий конфиг
    if (LittleFS.exists(CONFIG_FILE)) {
        fs::File configFile = LittleFS.open(CONFIG_FILE, "r");
        if (configFile) {
            DeserializationError error = deserializeJson(doc, configFile);
            if (error != DeserializationError::Ok) {
                LOG_WARNING("ConfigManager", "Failed to parse existing config, creating new: " + String(error.c_str()));
            }
            configFile.close();
        }
    }

    // Обновляем значение (plaintext)
    doc["apPassword"] = password;

    // Сохраняем обратно
    fs::File configFile = LittleFS.open(CONFIG_FILE, "w");
    if (!configFile) {
        LOG_ERROR("ConfigManager", "Failed to open config file for writing");
        return false;
    }

    size_t bytesWritten = serializeJson(doc, configFile);
    configFile.close();
    
    if (bytesWritten > 0) {
        LOG_INFO("ConfigManager", "AP Password saved successfully");
        return true;
    } else {
        LOG_ERROR("ConfigManager", "Failed to write AP password to config file");
        return false;
    }
}
