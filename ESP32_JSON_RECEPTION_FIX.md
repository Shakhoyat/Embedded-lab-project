# ESP32 JSON Reception Fix

## Problem Identified
The ESP32 was **not receiving the complete JSON payload** from Arduino Mega, resulting in corrupted/truncated data in `received_by_esp.json`.

### Root Causes:
1. **Serial Buffer Overflow**: ESP32's default Serial2 RX buffer (256 bytes) was too small for the 2048-byte JSON payload
2. **String Buffer Limitation**: `readStringUntil()` has limited buffer capacity and can't handle large payloads efficiently
3. **No delay for data arrival**: Large JSON packets need time to fully arrive before reading
4. **Insufficient JSON document size**: StaticJsonDocument was borderline (2048 bytes) without overhead margin

## Fixes Applied

### 1. Increased Serial2 RX Buffer (setup function)
```cpp
Serial2.setRxBufferSize(4096);  // Increased from default 256 to 4096 bytes
```
**Why**: Hardware buffer must be large enough to hold the entire incoming message before reading.

### 2. Improved Serial Reading Logic (loop function)
**Changes**:
- Added `delay(50)` to wait for complete data arrival
- Pre-allocated String memory with `reserve(2500)`
- Read character-by-character instead of `readStringUntil()`
- Added length tracking and safety checks
- Enhanced debug output with byte counts

**Why**: Character-by-character reading is more reliable for large payloads and avoids String class limitations.

### 3. Increased JSON Document Size (handleDataMessage)
```cpp
DynamicJsonDocument doc(3072);  // Increased from StaticJsonDocument<2048>
```
**Why**: 
- Arduino sends 2048 bytes of JSON
- ArduinoJson needs extra overhead for parsing (~30-50%)
- DynamicJsonDocument is more flexible for large payloads
- 3072 bytes (3KB) provides safe margin

### 4. Enhanced Error Reporting
Added detailed error messages showing:
- JSON payload length
- First 200 characters for debugging
- Specific error types

## Expected Results

### Before Fix:
```json
{
    "bedroom": {
        "temperature":l":45}}  // CORRUPTED & TRUNCATED
```

### After Fix:
```json
{
    "bedroom": {
        "temperature": 0,
        "gasLevel": 274,
        "flameDetected": false,
        "isEmergency": false,
        "isDangerous": true,
        "sensorTypes": "DS18B20,MQ2,Flame",
        "hasComponents": "Buzzer,LEDs"
    },
    "parking": { ... },
    "centralGas": { ... },
    "thresholds": { ... }
}
```

## Testing Steps

1. **Upload the fixed code to ESP32**
2. **Monitor Serial output** - Check for:
   - "Serial2 RX buffer set to 4096 bytes" message
   - "JSON payload size: XXXX bytes" showing complete size
   - No JSON parsing errors
3. **Verify data integrity** - All 4 segments (kitchen, bedroom, parking, central gas) should be complete
4. **Check Firebase** - Data should upload completely with all thresholds

## Additional Recommendations

### If still experiencing issues:

1. **Reduce send frequency** (Arduino):
   ```cpp
   const unsigned long SEND_INTERVAL = 5000;  // Increase from 3000 to 5000ms
   ```

2. **Add flow control** between Arduino and ESP32:
   - Arduino waits for ACK before sending next packet
   - Already implemented but verify timing

3. **Check physical connections**:
   - Arduino TX1 (pin 18) → ESP32 RX2 (pin 16)
   - Arduino RX1 (pin 19) → ESP32 TX2 (pin 17)
   - Common ground between Arduino and ESP32

4. **Monitor Serial output**:
   ```
   JSON data received from Arduino Mega (XXXX bytes)
   JSON payload size: XXXX bytes
   Data processing completed
   ```

## Performance Impact

- **Memory**: +4KB RAM for Serial2 buffer, +1KB for JSON processing
- **Latency**: +50ms delay for data arrival (negligible for 3-second intervals)
- **Reliability**: Significantly improved - should now handle 100% of packets correctly

## Notes

- ESP32 has 520KB RAM, so 5KB extra usage is minimal
- The 50ms delay is only applied when data arrives, not blocking normal operation
- Character-by-character reading is more CPU intensive but more reliable
- DynamicJsonDocument auto-manages memory allocation

---

**Status**: ✅ Ready to test
**Impact**: 🔧 Critical fix for data integrity
**Tested**: Pending field test
