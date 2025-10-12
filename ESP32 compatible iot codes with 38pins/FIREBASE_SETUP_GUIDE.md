# Firebase + React Web Dashboard Setup Guide

## 📋 Complete System Architecture

```
┌─────────────┐         ┌──────────────┐         ┌─────────────────┐
│  ESP32      │────────▶│   Firebase   │◀────────│  React Web App  │
│  Sensors    │  WiFi   │   Realtime   │  WebSocket │   Dashboard   │
│  (IoT)      │         │   Database   │         │   (3D View)     │
└─────────────┘         └──────────────┘         └─────────────────┘
                              │
                              │ Cloud Functions
                              ▼
                     ┌─────────────────────┐
                     │  Notifications      │
                     │  - Fire Brigade     │
                     │  - Owner Phone      │
                     │  - Email Alerts     │
                     └─────────────────────┘
```

---

## 🔥 Part 1: Firebase Setup

### Step 1: Create Firebase Project

1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Click "Add Project"
3. Enter project name: `smart-fire-alarm-system`
4. Disable Google Analytics (optional)
5. Click "Create Project"

### Step 2: Enable Realtime Database

1. In Firebase Console, go to **Build → Realtime Database**
2. Click "Create Database"
3. Select location (closest to you)
4. Start in **Test Mode** (for development)
5. Click "Enable"

### Step 3: Get Database Credentials

```javascript
// Your Firebase config will look like:
const firebaseConfig = {
  apiKey: "AIzaSyXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
  authDomain: "your-project.firebaseapp.com",
  databaseURL: "https://your-project-default-rtdb.firebaseio.com",
  projectId: "your-project",
  storageBucket: "your-project.appspot.com",
  messagingSenderId: "123456789",
  appId: "1:123456789:web:xxxxxxxxxxxxx"
};
```

### Step 4: Get Database Secret (for ESP32)

1. Go to **Project Settings** (⚙️ icon)
2. Select **Service Accounts** tab
3. Click **Database Secrets**
4. Copy the secret (long alphanumeric string)
5. Use this in your ESP32 code as `FIREBASE_AUTH`

**⚠️ Security Note:** For production, use Firebase Authentication instead of database secrets.

### Step 5: Configure Database Rules

Go to **Realtime Database → Rules** and paste:

```json
{
  "rules": {
    "buildings": {
      "$buildingId": {
        ".read": true,
        ".write": true,
        "zones": {
          "$zoneId": {
            ".indexOn": ["status", "timestamp"]
          }
        },
        "alerts": {
          ".indexOn": ["timestamp", "severity", "acknowledged"]
        },
        "emergencies": {
          ".indexOn": ["timestamp", "status"]
        }
      }
    }
  }
}
```

**For Production, use authenticated rules:**

```json
{
  "rules": {
    "buildings": {
      "$buildingId": {
        ".read": "auth != null",
        ".write": "auth != null && (auth.uid == $buildingId || root.child('admins').child(auth.uid).exists())",
        "zones": {
          "$zoneId": {
            ".read": "auth != null",
            ".write": "auth != null"
          }
        }
      }
    }
  }
}
```

---

## 📊 Part 2: Firebase Database Structure

### Optimized Data Structure for Real-time Updates:

```json
{
  "buildings": {
    "building_001": {
      "metadata": {
        "name": "Apartment Complex A",
        "address": "123 Main Street",
        "floors": 10,
        "totalZones": 30
      },
      
      "zones": {
        "kitchen": {
          "metadata": {
            "name": "Kitchen",
            "floor": 3,
            "type": "kitchen",
            "apartmentUnit": "3A"
          },
          "sensors": {
            "temperature": 24.5,
            "smoke": 450,
            "airQuality": 350.2,
            "flame": 3500,
            "status": "safe",
            "timestamp": 1697123456
          },
          "status": "safe",
          "lastUpdate": 1697123456
        },
        
        "living_room": {
          "metadata": {
            "name": "Living Room",
            "floor": 3,
            "type": "living_room",
            "apartmentUnit": "3A"
          },
          "sensors": {
            "temperature": 23.0,
            "smoke": 380,
            "airQuality": 320.5,
            "flame": 4000,
            "status": "safe",
            "timestamp": 1697123456
          },
          "status": "safe"
        },
        
        "bedroom_1": {
          "metadata": {
            "name": "Bedroom 1",
            "floor": 3,
            "type": "bedroom"
          },
          "sensors": { /* ... */ },
          "status": "safe"
        }
      },
      
      "alerts": {
        "kitchen_1697123456": {
          "type": "HIGH_TEMPERATURE",
          "severity": "WARNING",
          "zone": "kitchen",
          "zoneName": "Kitchen",
          "floor": 3,
          "message": "Kitchen temperature elevated.",
          "timestamp": 1697123456,
          "acknowledged": false,
          "resolved": false,
          "sensorData": {
            "temperature": 45.5,
            "smoke": 1200,
            "airQuality": 650.0,
            "flame": 2800
          }
        },
        
        "living_room_1697123500": {
          "type": "FIRE",
          "severity": "CRITICAL",
          "zone": "living_room",
          "zoneName": "Living Room",
          "floor": 3,
          "message": "Fire detected! Immediate evacuation required!",
          "timestamp": 1697123500,
          "acknowledged": false,
          "resolved": false,
          "sensorData": { /* ... */ }
        }
      },
      
      "emergencies": {
        "active": {
          "active": true,
          "type": "FIRE_BREAKOUT",
          "zone": "living_room",
          "zoneName": "Living Room",
          "floor": 3,
          "timestamp": 1697123500,
          "status": "pending",
          "notifyFireBrigade": true,
          "notifyOwner": true,
          "notifySecurity": true,
          "evacuationRequired": true
        }
      },
      
      "notifications": {
        "owner": {
          "pending": true,
          "message": "⚠️ Kitchen temperature: 45.5°C",
          "zone": "Kitchen",
          "timestamp": 1697123456
        }
      },
      
      "history": {
        "kitchen": {
          "1697123456": {
            "temperature": 24.5,
            "smoke": 450,
            "airQuality": 350.2,
            "flame": 3500,
            "status": "safe"
          }
        }
      },
      
      "emergencyContacts": {
        "fireBrigade": {
          "phone": "+1234567890",
          "name": "City Fire Department",
          "priority": 1
        },
        "apartmentOwner": {
          "phone": "+9876543210",
          "email": "owner@apartment.com",
          "name": "John Doe"
        },
        "security": {
          "phone": "+1122334455",
          "name": "Building Security"
        }
      },
      
      "evacuation": {
        "routes": {
          "floor_3": {
            "primaryExit": "Stairway A - East Wing",
            "secondaryExit": "Stairway B - West Wing",
            "assemblyPoint": "Parking Lot - North Side"
          }
        }
      }
    }
  }
}
```

---

## 🔔 Part 3: Firebase Cloud Functions (Notifications)

### Install Firebase CLI:

```bash
npm install -g firebase-tools
firebase login
firebase init functions
```

### Create `functions/index.js`:

```javascript
const functions = require('firebase-functions');
const admin = require('firebase-admin');
const twilio = require('twilio');
const nodemailer = require('nodemailer');

admin.initializeApp();

// Twilio Configuration (for SMS/Calls)
const twilioClient = twilio(
  'YOUR_TWILIO_ACCOUNT_SID',
  'YOUR_TWILIO_AUTH_TOKEN'
);
const TWILIO_PHONE = '+1234567890';

// Email Configuration
const mailTransport = nodemailer.createTransport({
  service: 'gmail',
  auth: {
    user: 'your-email@gmail.com',
    pass: 'your-app-password'
  }
});

// ===== FIRE BRIGADE EMERGENCY CALL =====
exports.notifyFireBrigade = functions.database
  .ref('/buildings/{buildingId}/emergencies/active')
  .onWrite(async (change, context) => {
    const emergency = change.after.val();
    
    if (!emergency || !emergency.active || !emergency.notifyFireBrigade) {
      return null;
    }

    const buildingId = context.params.buildingId;
    
    // Get emergency contact
    const contactSnapshot = await admin.database()
      .ref(`/buildings/${buildingId}/emergencyContacts/fireBrigade`)
      .once('value');
    
    const contact = contactSnapshot.val();
    
    if (!contact || !contact.phone) {
      console.error('Fire brigade contact not found!');
      return null;
    }

    // Make emergency call via Twilio
    try {
      const call = await twilioClient.calls.create({
        url: 'http://demo.twilio.com/docs/voice.xml', // Replace with TwiML URL
        to: contact.phone,
        from: TWILIO_PHONE,
        method: 'GET'
      });

      console.log('🚨 Emergency call initiated:', call.sid);

      // Send SMS as backup
      await twilioClient.messages.create({
        body: `🚨 FIRE EMERGENCY! Building: ${buildingId}, Zone: ${emergency.zoneName}, Floor: ${emergency.floor}. IMMEDIATE RESPONSE REQUIRED!`,
        to: contact.phone,
        from: TWILIO_PHONE
      });

      // Log notification
      await admin.database()
        .ref(`/buildings/${buildingId}/notifications/logs`)
        .push({
          type: 'FIRE_BRIGADE_CALL',
          recipient: contact.phone,
          status: 'sent',
          timestamp: Date.now(),
          callSid: call.sid
        });

      return { success: true, callSid: call.sid };
    } catch (error) {
      console.error('Error calling fire brigade:', error);
      return { success: false, error: error.message };
    }
  });

// ===== APARTMENT OWNER NOTIFICATION =====
exports.notifyApartmentOwner = functions.database
  .ref('/buildings/{buildingId}/notifications/owner')
  .onWrite(async (change, context) => {
    const notification = change.after.val();
    
    if (!notification || !notification.pending) {
      return null;
    }

    const buildingId = context.params.buildingId;
    
    // Get owner contact
    const contactSnapshot = await admin.database()
      .ref(`/buildings/${buildingId}/emergencyContacts/apartmentOwner`)
      .once('value');
    
    const contact = contactSnapshot.val();
    
    if (!contact) {
      console.error('Owner contact not found!');
      return null;
    }

    try {
      // Send SMS
      if (contact.phone) {
        await twilioClient.messages.create({
          body: notification.message,
          to: contact.phone,
          from: TWILIO_PHONE
        });
        console.log('📱 SMS sent to owner');
      }

      // Send Email
      if (contact.email) {
        await mailTransport.sendMail({
          from: '"Fire Alarm System" <noreply@firealarm.com>',
          to: contact.email,
          subject: `⚠️ Alert: ${notification.zone}`,
          html: `
            <h2>Fire Alarm System Alert</h2>
            <p><strong>Message:</strong> ${notification.message}</p>
            <p><strong>Zone:</strong> ${notification.zone}</p>
            <p><strong>Time:</strong> ${new Date(notification.timestamp * 1000).toLocaleString()}</p>
            <p>Please check your apartment immediately or contact building security.</p>
          `
        });
        console.log('📧 Email sent to owner');
      }

      // Mark as sent
      await admin.database()
        .ref(`/buildings/${buildingId}/notifications/owner/pending`)
        .set(false);

      return { success: true };
    } catch (error) {
      console.error('Error notifying owner:', error);
      return { success: false, error: error.message };
    }
  });

// ===== CRITICAL ALERT EMAIL =====
exports.sendCriticalAlertEmail = functions.database
  .ref('/buildings/{buildingId}/alerts/{alertId}')
  .onCreate(async (snapshot, context) => {
    const alert = snapshot.val();
    
    if (alert.severity !== 'CRITICAL') {
      return null;
    }

    const buildingId = context.params.buildingId;
    
    // Get all contacts
    const contactsSnapshot = await admin.database()
      .ref(`/buildings/${buildingId}/emergencyContacts`)
      .once('value');
    
    const contacts = contactsSnapshot.val();
    const emailList = [];
    
    if (contacts.apartmentOwner?.email) emailList.push(contacts.apartmentOwner.email);
    if (contacts.security?.email) emailList.push(contacts.security.email);

    if (emailList.length === 0) return null;

    try {
      await mailTransport.sendMail({
        from: '"Fire Alarm System" <noreply@firealarm.com>',
        to: emailList.join(','),
        subject: `🚨 CRITICAL ALERT: ${alert.type}`,
        html: `
          <div style="background-color: #ff0000; color: white; padding: 20px;">
            <h1>🚨 CRITICAL FIRE ALARM ALERT</h1>
          </div>
          <div style="padding: 20px;">
            <h2>Alert Details:</h2>
            <table>
              <tr><td><strong>Type:</strong></td><td>${alert.type}</td></tr>
              <tr><td><strong>Severity:</strong></td><td>${alert.severity}</td></tr>
              <tr><td><strong>Zone:</strong></td><td>${alert.zoneName}</td></tr>
              <tr><td><strong>Floor:</strong></td><td>${alert.floor}</td></tr>
              <tr><td><strong>Message:</strong></td><td>${alert.message}</td></tr>
              <tr><td><strong>Time:</strong></td><td>${new Date(alert.timestamp * 1000).toLocaleString()}</td></tr>
            </table>
            <h3>Sensor Readings:</h3>
            <ul>
              <li>Temperature: ${alert.sensorData.temperature}°C</li>
              <li>Smoke Level: ${alert.sensorData.smoke}</li>
              <li>Air Quality: ${alert.sensorData.airQuality} PPM</li>
              <li>Flame Sensor: ${alert.sensorData.flame}</li>
            </ul>
            <p style="color: red; font-weight: bold;">IMMEDIATE ACTION REQUIRED!</p>
          </div>
        `
      });

      console.log('📧 Critical alert email sent');
      return { success: true };
    } catch (error) {
      console.error('Error sending critical alert:', error);
      return { success: false, error: error.message };
    }
  });

// ===== AUTO-RESOLVE OLD ALERTS =====
exports.autoResolveAlerts = functions.pubsub
  .schedule('every 5 minutes')
  .onRun(async (context) => {
    const now = Date.now() / 1000;
    const oneHourAgo = now - 3600;

    const db = admin.database();
    const buildingsSnapshot = await db.ref('/buildings').once('value');
    const buildings = buildingsSnapshot.val();

    for (const buildingId in buildings) {
      const alertsSnapshot = await db
        .ref(`/buildings/${buildingId}/alerts`)
        .orderByChild('timestamp')
        .endAt(oneHourAgo)
        .once('value');

      const alerts = alertsSnapshot.val();
      
      if (alerts) {
        for (const alertId in alerts) {
          if (!alerts[alertId].resolved) {
            await db.ref(`/buildings/${buildingId}/alerts/${alertId}/resolved`).set(true);
            console.log(`Auto-resolved alert: ${alertId}`);
          }
        }
      }
    }

    return null;
  });
```

### Deploy Cloud Functions:

```bash
npm install twilio nodemailer --save
firebase deploy --only functions
```

---

## 🔧 Part 4: ESP32 Configuration

### Update your ESP32 code with your credentials:

```cpp
// Line 25-26: WiFi Credentials
#define WIFI_SSID "YourWiFiName"
#define WIFI_PASSWORD "YourWiFiPassword"

// Line 29-30: Firebase Configuration
#define FIREBASE_HOST "your-project-id-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "your-database-secret-key"

// Line 33-36: Building Configuration
#define BUILDING_ID "building_001"
#define ZONE_ID "kitchen"  // kitchen, living_room, bedroom_1, etc.
#define ZONE_NAME "Kitchen"
#define FLOOR_NUMBER 3
```

### Install Required Libraries in Arduino IDE:

```
1. Firebase ESP32 Client by Mobizt
   - Sketch → Include Library → Manage Libraries
   - Search: "Firebase ESP32 Client"
   - Install version 4.3.0 or later

2. Already have:
   - LiquidCrystal I2C
   - OneWire
   - DallasTemperature
```

---

## 📱 Part 5: Setup Multiple ESP32 Devices (Multi-Zone)

For complete building coverage, deploy multiple ESP32s:

```cpp
// Kitchen ESP32:
#define ZONE_ID "kitchen"
#define ZONE_NAME "Kitchen"
#define FLOOR_NUMBER 3

// Living Room ESP32:
#define ZONE_ID "living_room"
#define ZONE_NAME "Living Room"
#define FLOOR_NUMBER 3

// Bedroom ESP32:
#define ZONE_ID "bedroom_1"
#define ZONE_NAME "Bedroom 1"
#define FLOOR_NUMBER 3

// Hallway ESP32:
#define ZONE_ID "hallway"
#define ZONE_NAME "Hallway"
#define FLOOR_NUMBER 3
```

---

## 📊 Data Flow Summary

```
1. ESP32 reads sensors every 5 seconds
2. ESP32 uploads to Firebase every 10 seconds
3. Firebase Cloud Functions monitor for emergencies
4. Critical alerts trigger:
   - Phone call to Fire Brigade
   - SMS to Apartment Owner
   - Email notifications
   - Web dashboard updates in real-time
5. React web app displays:
   - Live sensor values
   - 3D building visualization
   - Alert history
   - Evacuation routes
```

---

## 🔐 Security Best Practices

1. **Use Firebase Authentication** for production
2. **Secure database rules** with user authentication
3. **Encrypt sensitive data** like phone numbers
4. **Use HTTPS** for all web communications
5. **Rotate API keys** regularly
6. **Limit database access** by user role

---

## 🧪 Testing Your Setup

### Test Firebase Connection:

1. Upload ESP32 code
2. Open Serial Monitor (115200 baud)
3. Check for "✅ WiFi Connected!"
4. Check for "✅ Firebase configured!"
5. Go to Firebase Console → Realtime Database
6. Watch for live data appearing

### Test Alerts:

1. Heat the temperature sensor with your hand
2. Blow smoke near MQ2 sensor
3. Light a match near flame sensor
4. Check Firebase Console for alerts appearing
5. Check if Cloud Functions trigger (see Logs)

---

## 📞 Emergency Contact Setup

Add to Firebase manually or through web app:

```json
{
  "buildings": {
    "building_001": {
      "emergencyContacts": {
        "fireBrigade": {
          "phone": "+1234567890",
          "name": "City Fire Department"
        },
        "apartmentOwner": {
          "phone": "+9876543210",
          "email": "owner@example.com",
          "name": "John Doe"
        },
        "security": {
          "phone": "+1122334455",
          "email": "security@building.com",
          "name": "Security Desk"
        }
      }
    }
  }
}
```

---

## 💰 Cost Estimation

### Firebase Pricing (Free Tier):
- **Realtime Database**: 1GB storage, 10GB download/month
- **Cloud Functions**: 2M invocations/month
- **Hosting**: 10GB storage, 360MB/day transfer

### Twilio Pricing:
- **Phone Calls**: ~$0.013/minute
- **SMS**: ~$0.0075/message
- **Trial Account**: $15 credit

### Estimated Monthly Cost (Small Building):
- **Firebase**: $0 (within free tier)
- **Twilio**: ~$5-10 (assuming 20-30 alerts/month)
- **Total**: **~$10/month**

---

Next: **React Web Dashboard Setup** →
