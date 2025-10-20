# Smart Building Monitoring System - Frontend

## Features

### 🏢 Real-time Monitoring Dashboard
- **4 Segment Monitoring**: Kitchen, Bedroom, Parking, Central Gas Chamber
- **Live Data Updates**: Real-time Firebase synchronization
- **Visual Status Indicators**: Color-coded cards (Green=Safe, Orange=Warning, Red=Emergency)
- **Comprehensive Sensor Data**: Temperature, Humidity, Gas Levels, Air Quality, Flame Detection

### 🚨 Emergency Alert System
- **Browser Notifications**: Desktop notifications for emergency alerts
- **Audio Alerts**: Sound notification for critical events
- **Emergency Timeline**: Recent alerts with detailed information
- **Fire Brigade Notifications**: Automatic KUET Fire Brigade alert tracking
- **Visual Indicators**: Pulsing animations for emergency states

### 📊 Segment-Specific Information
Each segment card displays:
- Current status (SAFE/WARNING/EMERGENCY/FIRE DETECTED)
- Temperature readings
- Humidity levels (Kitchen only)
- Gas level monitoring
- Air quality index (Kitchen only)
- Flame detection status
- Sensor types and components
- Last update timestamp

### 🎨 Modern UI/UX
- Dark gradient background with glassmorphism effects
- Responsive design for desktop and mobile
- Smooth animations and transitions
- Color-coded status indicators
- Real-time data updates
- Emergency state animations

## Setup Instructions

### 1. Install Dependencies
```bash
npm install
```

### 2. Firebase Configuration
The Firebase configuration is already set in `src/firebase/config.js`. Ensure your Firebase project is properly configured with:
- Realtime Database enabled
- Database rules allowing read/write access
- Proper authentication setup

### 3. Run Development Server
```bash
npm run dev
```

The application will start at `http://localhost:5173`

### 4. Build for Production
```bash
npm run build
```

### 5. Deploy
```bash
npm run preview
```

## Browser Notifications

To enable emergency notifications:
1. When you first load the page, allow notifications when prompted
2. Emergency alerts will show as desktop notifications
3. Audio alerts will play for critical events

## Alert Sound Setup

Replace the placeholder `public/alert-sound.mp3` with an actual alert sound file:
1. Download a suitable alert sound (MP3 format)
2. Place it in the `public` folder as `alert-sound.mp3`
3. Recommended: Short beep or siren sound (2-3 seconds)

## Firebase Data Structure

The frontend expects the following Firebase structure:

```
smartBuilding/
  ├── system/
  │   ├── systemEmergency: boolean
  │   ├── globalTemperature: number
  │   ├── globalHumidity: number
  │   ├── emergencyDuration: number
  │   ├── esp32Status: string
  │   ├── uptime: number
  │   ├── lastUpdated: string
  │   └── info/
  │       └── buildingName: string
  │
  ├── segments/
  │   ├── kitchen/
  │   │   ├── temperature: number
  │   │   ├── humidity: number
  │   │   ├── gasLevel: number
  │   │   ├── airQuality: number
  │   │   ├── flameDetected: boolean
  │   │   ├── isEmergency: boolean
  │   │   ├── isDangerous: boolean
  │   │   ├── status: string
  │   │   ├── sensorTypes: string
  │   │   ├── components: string
  │   │   └── lastUpdated: string
  │   │
  │   ├── bedroom/
  │   ├── parking/
  │   └── centralGas/
  │
  ├── alerts/
  │   └── [alertId]/
  │       ├── segment: string
  │       ├── type: string
  │       ├── cause: string
  │       ├── timestamp: string
  │       ├── acknowledged: boolean
  │       └── notification/
  │           ├── title: string
  │           └── body: string
  │
  └── emergency_notifications/
      └── [notificationId]/
          ├── recipient: string
          ├── timestamp: string
          └── details/
              ├── building: string
              ├── address: string
              ├── location: string
              └── cause: string
```

## Color Coding

- **Green (Safe)**: All parameters within normal range
- **Orange (Warning)**: Some parameters elevated but not critical
- **Red (Emergency)**: Critical conditions or flame detected
- **Pulsing Red Border**: Active emergency in progress

## Gas Level Thresholds

- **Normal**: 0-300
- **Warning**: 300-500
- **Critical**: 500+

## Air Quality Thresholds

- **Good**: 0-400
- **Warning**: 400-600
- **Critical**: 600+

## Temperature Thresholds

- **Low Alert**: < 10°C
- **Normal**: 10-35°C
- **High Alert**: 35-45°C
- **Critical**: > 45°C

## Troubleshooting

### No Data Showing
1. Check Firebase connection in browser console
2. Verify ESP32 is sending data to Firebase
3. Check Firebase database rules

### Notifications Not Working
1. Check browser notification permissions
2. Ensure HTTPS connection (required for notifications)
3. Check browser compatibility

### Alerts Not Appearing
1. Verify alert data exists in Firebase
2. Check console for JavaScript errors
3. Ensure alert structure matches expected format

## Browser Compatibility

- Chrome/Edge: Full support
- Firefox: Full support
- Safari: Full support (notifications may require additional permissions)
- Mobile browsers: Responsive design supported

## Performance Notes

- Real-time updates are optimized with Firebase listeners
- Alert history is limited to latest 10 items for performance
- Emergency notifications limited to latest 5 items
- Data updates happen automatically without page refresh

## Future Enhancements

- Historical data charts
- SMS alert integration
- Email notifications
- Multiple building support
- User authentication and roles
- Alert acknowledgment system
- Custom threshold configuration
- Export data to CSV/PDF
