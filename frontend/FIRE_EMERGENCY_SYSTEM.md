# Enhanced Fire Emergency Notification System

## Overview
This enhanced Smart Building Monitor includes a comprehensive fire emergency notification system designed specifically for managers and emergency responders.

## Key Features

### 🔥 **Fire Emergency Detection**
- Real-time monitoring of flame sensors across all building segments
- Immediate detection and classification of fire emergencies
- Automatic escalation for unacknowledged alerts

### 🚨 **Multi-Channel Notifications**
- **Browser Notifications**: Persistent, high-priority alerts with custom sounds
- **Visual Overlays**: Full-screen emergency modals that demand attention
- **Audio Alerts**: Distinct sounds for different emergency types
- **Escalation System**: Automatic escalation to backup contacts after configurable delay

### 👨‍💼 **Manager Dashboard**
- Secure manager authentication (Demo: username: `manager`, password: `emergency123`)
- Real-time emergency management controls
- Direct emergency services dispatch
- Building evacuation controls
- Emergency contact management

### 📱 **Emergency Overlay Features**
- **Auto-Dispatch Timer**: Automatically contacts emergency services if not acknowledged within 30 seconds
- **Critical Actions**: Direct buttons for fire department dispatch, building evacuation, security alerts
- **Location Details**: Precise building location and segment information
- **Emergency Timeline**: Complete timeline of incident detection and response

### 🔔 **Notification Management**
- Customizable notification settings
- Alert acknowledgment system
- Emergency history tracking
- Escalation timer management

## Fire Emergency Response Workflow

1. **Detection**: Fire/flame sensors detect emergency condition
2. **Immediate Alert**: Browser notification with sound alert appears
3. **Manager Notification**: If manager mode is enabled, enhanced notifications are triggered
4. **Emergency Overlay**: Full-screen emergency modal appears for critical fires
5. **Auto-Escalation**: If not acknowledged within 30 seconds, emergency services are automatically contacted
6. **Response Actions**: Manager can dispatch emergency services, trigger building evacuation, or acknowledge false alarms

## Emergency Actions Available

### 🚒 **Fire Department Dispatch**
- Direct contact to KUET Area Fire Station (01303488507)
- Automatic notification with building details and emergency location
- Estimated response time tracking

### 📢 **Building Evacuation**
- Immediate alert to all building residents
- Emergency announcement system activation
- Evacuation route guidance

### 🔧 **Building Safety Controls**
- Gas supply shutoff controls
- Emergency power management
- Security team notification
- Management team alerts

## Contact Information
- **Building**: KUET Smart Apartment Complex
- **Address**: Fulbarigate, Khulna, KUET Area
- **Fire Department**: 01303488507
- **Admin Contact**: +8801XXXXXXXXX
- **Building Management**: admin@kuetapartment.com

## Technical Implementation

### Components
- `EnhancedSmartBuildingMonitor.jsx`: Main enhanced dashboard with manager controls
- `NotificationManager.jsx`: Advanced notification management system
- `EmergencyOverlay.jsx`: Full-screen fire emergency interface
- `SmartBuildingMonitor.jsx`: Core monitoring dashboard (enhanced with callbacks)

### Dependencies
- React 19.1.1
- Firebase 12.4.0
- Tailwind CSS 3.4.18

### Audio Assets Required
Place these audio files in `/public/sounds/`:
- `emergency-siren.mp3`: Fire emergency alarm
- `warning-beep.mp3`: Warning alert sound
- `acknowledgment.mp3`: Confirmation sound

### Icon Assets Required
Place these icon files in `/public/`:
- `fire-emergency-icon.png`: Fire emergency notification icon
- `fire-badge.png`: Notification badge icon

## Manager Authentication
For demonstration purposes, use these credentials:
- **Username**: `manager`
- **Password**: `emergency123`

In a production environment, implement proper authentication with:
- Secure password hashing
- Multi-factor authentication
- Role-based access controls
- Session management

## Firebase Integration
The system saves emergency data to these Firebase paths:
- `/smartBuilding/managerAlerts`: Manager-specific emergency alerts
- `/smartBuilding/alerts`: General system alerts
- `/smartBuilding/emergency_notifications`: Fire brigade notifications

## Browser Compatibility
- Requires modern browser with Web Notifications API support
- Works best with Chrome, Firefox, Safari, Edge
- Mobile responsive design for tablet/phone access

## Emergency Testing
To test the emergency system:
1. Enable Manager Mode
2. Trigger a fire condition in the Arduino system
3. Observe the notification workflow
4. Test acknowledgment and dispatch functions

⚠️ **Important**: In a real emergency, always follow proper evacuation procedures and contact emergency services immediately.