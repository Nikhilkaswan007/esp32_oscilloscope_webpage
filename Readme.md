# ESP32 Web Oscilloscope

A professional-grade web-based oscilloscope using ESP32 and ADS1115 ADC with adaptive time scaling and real-time signal analysis.

## 🌟 Features

### Core Functionality
- **Real-time voltage measurement** up to 8kHz sampling rate
- **Web-based interface** accessible from any device
- **Adaptive time scaling** - automatically adjusts display for optimal waveform visibility
- **Professional trigger system** with normal and auto modes
- **Multiple voltage scales** from 0.5V/div to 20V/div
- **Waveform analysis** with automatic detection of signal types

### Advanced Features
- **Auto-scaling time base** - prevents waveform compression at high frequencies
- **Frequency detection** with real-time updates
- **Peak-to-peak measurement**
- **Waveform type detection** (Sine, PWM, Triangle, DC, etc.)
- **Trigger level control** with visual indicator
- **Professional oscilloscope grid display**

## 📋 Requirements

### Hardware
- **ESP32 Development Board** (any variant)
- **ADS1115 16-bit ADC** module
- **Jumper wires** for connections
- **Signal source** (function generator, sensors, etc.)
- **Optional:** Voltage divider circuit for higher voltage measurements

### Software
- **Arduino IDE** with ESP32 board support
- **Required Libraries:**
  - WiFi (ESP32 built-in)
  - WebServer (ESP32 built-in)
  - ArduinoJson
  - Wire (built-in)
  - Adafruit_ADS1X15

## 🔧 Hardware Setup

### Wiring Diagram
```
ESP32          ADS1115
-----          -------
3.3V    <-->   VDD
GND     <-->   GND
GPIO21  <-->   SDA
GPIO22  <-->   SCL
               A0 (Signal Input)
```

### Input Protection (Recommended)
For signals above 4.096V, use a voltage divider:
```
Signal Input ----[R1]----+----[R2]---- GND
                          |
                          +---- A0 (ADS1115)
```
- For 0-10V input: R1 = 6.8kΩ, R2 = 3.3kΩ
- For 0-20V input: R1 = 16kΩ, R2 = 4kΩ

## ⚙️ Software Installation

### 1. Install Arduino IDE
Download from [arduino.cc](https://www.arduino.cc/en/software)

### 2. Add ESP32 Board Support
1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add this URL to "Additional Board Manager URLs":
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search "ESP32" and install "ESP32 by Espressif Systems"

### 3. Install Required Libraries
Go to **Tools → Manage Libraries** and install:
- `ArduinoJson` by Benoit Blanchon
- `Adafruit ADS1X15` by Adafruit

### 4. Upload Code
1. Connect ESP32 to computer via USB
2. Select board: **Tools → Board → ESP32 Dev Module**
3. Select correct COM port: **Tools → Port**
4. Update WiFi credentials in code:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
5. Upload the code

## 🚀 Quick Start

### 1. Power On
- Connect ESP32 to power
- Wait for WiFi connection (check Serial Monitor for IP address)

### 2. Access Web Interface
- Open web browser
- Navigate to ESP32 IP address (e.g., `http://192.168.1.100`)

### 3. Connect Signal
- Connect your signal source to ADS1115 A0 pin
- Ensure signal is within 0-4.096V range (or use voltage divider)

### 4. Start Measuring
- Click **Start** button
- Adjust controls as needed
- Enable **Auto Scale** for automatic time base adjustment

## 🎛️ Controls Reference

### Sample Rate
- **500 Hz - 8000 Hz**: Adjusts how fast samples are taken
- Higher rates for fast signals, lower for slow signals

### Time Scale
- **Auto Scale**: Automatically adjusts for optimal waveform display
- **Manual**: Choose from 1ms to 5 seconds per division
- Shows 2-5 complete waveform cycles when auto-enabled

### Voltage Scale
- **0.5V/div to 20V/div**: Vertical scale adjustment
- Each division represents the selected voltage range

### Trigger System
- **Auto**: Continuous free-running display
- **Normal**: Waits for signal to cross trigger level
- **Trigger Level**: Adjustable voltage threshold (0-45V)

## 📊 Display Information

### Real-time Measurements
- **Current Voltage**: Instantaneous voltage reading
- **Peak-to-Peak**: Voltage difference between max and min
- **Frequency**: Detected signal frequency
- **Waveform Type**: Automatic signal classification
- **ADC Reading**: Raw ADC value
- **Effective Time Scale**: Current time base (auto or manual)

### Waveform Types Detected
- **Sine Wave**: Smooth sinusoidal signals
- **PWM**: Pulse width modulated signals with duty cycle
- **Triangle/Ramp**: Linear ramping waveforms
- **DC/Flat**: Constant voltage levels
- **Noise/Random**: Irregular or noisy signals
- **Complex/Other**: Mixed or complex waveforms

## 🔧 Configuration Options

### Calibration
Adjust the voltage multiplier in code based on your voltage divider:
```cpp
return voltage * 10.0; // Adjust this value
```

### Buffer Size
Change sample buffer size (default 500):
```cpp
const int BUFFER_SIZE = 500; // Increase for longer capture
```

### Sampling Rate Limits
Maximum safe sampling rate depends on your setup:
- **Basic setup**: 2-4 kHz
- **Optimized setup**: Up to 8 kHz

## 🔍 Troubleshooting

### Common Issues

**No WiFi Connection**
- Check SSID and password
- Ensure ESP32 is in range
- Check Serial Monitor for error messages

**No Signal Display**
- Verify ADS1115 wiring
- Check if ADS1115 is detected (Serial Monitor)
- Ensure signal is within voltage range
- Try different gain settings

**Unstable Readings**
- Add decoupling capacitors (100nF ceramic + 10µF electrolytic)
- Use shorter wires
- Add ground plane
- Shield signal wires

**Web Interface Not Loading**
- Check IP address in Serial Monitor
- Try different browser
- Clear browser cache
- Check firewall settings

### Voltage Range Issues
If measuring voltages outside 0-4.096V range:
1. Use appropriate voltage divider
2. Update calibration factor in code
3. Consider using different ADS1115 gain settings

## 📈 Performance Tips

### For High Frequency Signals
- Use higher sampling rates (4-8 kHz)
- Enable auto time scaling
- Use shorter connection wires
- Add signal conditioning if needed

### For Low Frequency Signals
- Use lower sampling rates (500-1000 Hz)
- Increase buffer size for longer captures
- Use manual time scaling for very slow signals

### For Noisy Environments
- Use twisted pair cables
- Add input filtering
- Use differential input if available
- Implement software filtering

## 🔧 Advanced Customization

### Adding Custom Voltage Measurement Class
Replace the basic voltage functions with your custom implementation:
```cpp
#include "voltage_measurement.h"
VoltageMeasurement voltmeter;

// In setup():
voltmeter.begin();

// Replace readVoltage() calls:
float voltage = voltmeter.readVoltage();
```

### Extending Waveform Analysis
Add custom waveform detection algorithms in the `detectWaveformType()` function.

### Adding Data Logging
Implement data logging by extending the `/data` endpoint to save measurements to SD card or cloud storage.

## 📝 License

This project is open source. Feel free to modify and distribute according to your needs.

## 🤝 Contributing

Contributions welcome! Areas for improvement:
- Additional waveform analysis algorithms
- Better signal conditioning
- Mobile interface optimization
- Data export features
- Spectrum analysis

## 📞 Support

For issues and questions:
1. Check troubleshooting section
2. Review hardware connections
3. Check Serial Monitor output
4. Verify library versions

## 🔄 Version History

### v2.0 (Current)
- Added adaptive time scaling
- Enhanced frequency detection
- Improved waveform analysis
- Better auto-scaling algorithms
- Professional oscilloscope features

### v1.0
- Basic oscilloscope functionality
- Web interface
- Manual controls
- Simple triggering

---

**Happy measuring! 📊⚡**