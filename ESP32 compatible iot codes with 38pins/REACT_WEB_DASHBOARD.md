# React Web Dashboard - Complete Project Structure

## 🚀 Quick Start

```bash
# Create React app
npx create-react-app fire-alarm-dashboard
cd fire-alarm-dashboard

# Install dependencies
npm install firebase three @react-three/fiber @react-three/drei
npm install recharts react-router-dom react-toastify
npm install @mui/material @mui/icons-material @emotion/react @emotion/styled

# Start development server
npm start
```

---

## 📁 Project Structure

```
fire-alarm-dashboard/
├── public/
│   ├── index.html
│   └── favicon.ico
├── src/
│   ├── components/
│   │   ├── Dashboard.jsx           # Main dashboard
│   │   ├── Building3D.jsx          # 3D building visualization
│   │   ├── ZoneCard.jsx            # Individual zone display
│   │   ├── AlertsList.jsx          # Alerts history
│   │   ├── SensorChart.jsx         # Real-time charts
│   │   ├── EvacuationMap.jsx       # Evacuation routes
│   │   ├── EmergencyPanel.jsx      # Emergency controls
│   │   └── Navbar.jsx              # Navigation bar
│   ├── firebase/
│   │   └── config.js               # Firebase configuration
│   ├── hooks/
│   │   ├── useFirebaseData.js      # Real-time data hook
│   │   └── useAlerts.js            # Alerts hook
│   ├── utils/
│   │   ├── statusColors.js         # Color coding
│   │   └── notifications.js        # Browser notifications
│   ├── App.js
│   ├── App.css
│   └── index.js
├── package.json
└── README.md
```

---

## 📄 Complete Code Files

### 1. `src/firebase/config.js`

```javascript
import { initializeApp } from 'firebase/app';
import { getDatabase, ref, onValue, set, push } from 'firebase/database';

// Your Firebase configuration
const firebaseConfig = {
  apiKey: "YOUR_API_KEY",
  authDomain: "your-project.firebaseapp.com",
  databaseURL: "https://your-project-default-rtdb.firebaseio.com",
  projectId: "your-project-id",
  storageBucket: "your-project.appspot.com",
  messagingSenderId: "123456789",
  appId: "1:123456789:web:xxxxxxxxxxxxx"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

export { database, ref, onValue, set, push };
export default app;
```

---

### 2. `src/hooks/useFirebaseData.js`

```javascript
import { useState, useEffect } from 'react';
import { database, ref, onValue } from '../firebase/config';

export const useFirebaseData = (buildingId) => {
  const [zones, setZones] = useState({});
  const [alerts, setAlerts] = useState([]);
  const [emergency, setEmergency] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // Listen to zones data
    const zonesRef = ref(database, `buildings/${buildingId}/zones`);
    const unsubscribeZones = onValue(zonesRef, (snapshot) => {
      const data = snapshot.val();
      setZones(data || {});
      setLoading(false);
    });

    // Listen to alerts
    const alertsRef = ref(database, `buildings/${buildingId}/alerts`);
    const unsubscribeAlerts = onValue(alertsRef, (snapshot) => {
      const data = snapshot.val();
      if (data) {
        const alertsArray = Object.keys(data).map(key => ({
          id: key,
          ...data[key]
        })).sort((a, b) => b.timestamp - a.timestamp);
        setAlerts(alertsArray);
      } else {
        setAlerts([]);
      }
    });

    // Listen to emergency
    const emergencyRef = ref(database, `buildings/${buildingId}/emergencies/active`);
    const unsubscribeEmergency = onValue(emergencyRef, (snapshot) => {
      const data = snapshot.val();
      setEmergency(data);
    });

    // Cleanup subscriptions
    return () => {
      unsubscribeZones();
      unsubscribeAlerts();
      unsubscribeEmergency();
    };
  }, [buildingId]);

  return { zones, alerts, emergency, loading };
};
```

---

### 3. `src/components/Dashboard.jsx`

```javascript
import React, { useState, useEffect } from 'react';
import { useFirebaseData } from '../hooks/useFirebaseData';
import Building3D from './Building3D';
import ZoneCard from './ZoneCard';
import AlertsList from './AlertsList';
import SensorChart from './SensorChart';
import EmergencyPanel from './EmergencyPanel';
import { Grid, Container, Paper, Typography, Box, Tabs, Tab } from '@mui/material';
import { ToastContainer, toast } from 'react-toastify';
import 'react-toastify/dist/ReactToastify.css';

const Dashboard = () => {
  const buildingId = 'building_001';
  const { zones, alerts, emergency, loading } = useFirebaseData(buildingId);
  const [activeTab, setActiveTab] = useState(0);
  const [selectedZone, setSelectedZone] = useState(null);

  // Show notifications for critical alerts
  useEffect(() => {
    if (alerts.length > 0 && alerts[0].severity === 'CRITICAL') {
      const latestAlert = alerts[0];
      if (!latestAlert.notified) {
        toast.error(
          `🚨 ${latestAlert.type}: ${latestAlert.message}`,
          { autoClose: false }
        );
        
        // Browser notification
        if ('Notification' in window && Notification.permission === 'granted') {
          new Notification('🚨 CRITICAL ALERT', {
            body: `${latestAlert.zoneName}: ${latestAlert.message}`,
            icon: '/fire-icon.png',
            requireInteraction: true
          });
        }
      }
    }
  }, [alerts]);

  // Request notification permission
  useEffect(() => {
    if ('Notification' in window && Notification.permission === 'default') {
      Notification.requestPermission();
    }
  }, []);

  if (loading) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center" height="100vh">
        <Typography variant="h4">Loading Fire Alarm System...</Typography>
      </Box>
    );
  }

  const dangerZones = Object.keys(zones).filter(
    key => zones[key].status === 'danger'
  );
  const warningZones = Object.keys(zones).filter(
    key => zones[key].status === 'warning'
  );

  return (
    <Container maxWidth="xl" sx={{ mt: 4, mb: 4 }}>
      <ToastContainer position="top-right" />
      
      {/* Emergency Banner */}
      {emergency && emergency.active && (
        <EmergencyPanel emergency={emergency} buildingId={buildingId} />
      )}

      {/* Header */}
      <Paper sx={{ p: 3, mb: 3, bgcolor: '#1976d2', color: 'white' }}>
        <Typography variant="h3" gutterBottom>
          🔥 Intelligent Fire Alarm System
        </Typography>
        <Typography variant="h6">
          Building: {buildingId} | Active Zones: {Object.keys(zones).length}
        </Typography>
        <Box sx={{ mt: 2 }}>
          <Typography variant="body1">
            🟢 Safe: {Object.keys(zones).length - dangerZones.length - warningZones.length} | 
            🟡 Warning: {warningZones.length} | 
            🔴 Danger: {dangerZones.length}
          </Typography>
        </Box>
      </Paper>

      {/* Tabs */}
      <Paper sx={{ mb: 3 }}>
        <Tabs value={activeTab} onChange={(e, v) => setActiveTab(v)}>
          <Tab label="3D View" />
          <Tab label="Zone Details" />
          <Tab label="Alerts History" />
          <Tab label="Charts" />
        </Tabs>
      </Paper>

      {/* Tab Content */}
      {activeTab === 0 && (
        <Grid container spacing={3}>
          <Grid item xs={12} lg={8}>
            <Paper sx={{ p: 2, height: '600px' }}>
              <Typography variant="h5" gutterBottom>
                Building 3D Visualization
              </Typography>
              <Building3D 
                zones={zones} 
                onZoneClick={setSelectedZone}
                emergency={emergency}
              />
            </Paper>
          </Grid>
          <Grid item xs={12} lg={4}>
            {selectedZone ? (
              <ZoneCard zone={zones[selectedZone]} zoneName={selectedZone} />
            ) : (
              <Paper sx={{ p: 3 }}>
                <Typography variant="h6">
                  Click on a zone in the 3D view to see details
                </Typography>
              </Paper>
            )}
          </Grid>
        </Grid>
      )}

      {activeTab === 1 && (
        <Grid container spacing={3}>
          {Object.keys(zones).map(zoneId => (
            <Grid item xs={12} md={6} lg={4} key={zoneId}>
              <ZoneCard zone={zones[zoneId]} zoneName={zoneId} />
            </Grid>
          ))}
        </Grid>
      )}

      {activeTab === 2 && (
        <AlertsList alerts={alerts} buildingId={buildingId} />
      )}

      {activeTab === 3 && (
        <Grid container spacing={3}>
          {Object.keys(zones).map(zoneId => (
            <Grid item xs={12} lg={6} key={zoneId}>
              <SensorChart zone={zones[zoneId]} zoneName={zoneId} buildingId={buildingId} />
            </Grid>
          ))}
        </Grid>
      )}
    </Container>
  );
};

export default Dashboard;
```

---

### 4. `src/components/Building3D.jsx`

```javascript
import React, { useRef, useState } from 'react';
import { Canvas } from '@react-three/fiber';
import { OrbitControls, Text, Box, Html } from '@react-three/drei';
import * as THREE from 'three';

const Floor = ({ position, zones, onZoneClick, floorNumber }) => {
  return (
    <group position={position}>
      {/* Floor base */}
      <Box args={[20, 0.2, 20]} position={[0, 0, 0]}>
        <meshStandardMaterial color="#cccccc" />
      </Box>
      
      {/* Floor label */}
      <Text
        position={[-11, 1, 0]}
        fontSize={1}
        color="black"
      >
        Floor {floorNumber}
      </Text>
      
      {/* Zones (rooms) */}
      {zones.map((zone, index) => (
        <Zone
          key={zone.id}
          position={zone.position}
          zone={zone}
          onClick={() => onZoneClick(zone.id)}
        />
      ))}
    </group>
  );
};

const Zone = ({ position, zone, onClick }) => {
  const [hovered, setHovered] = useState(false);
  
  const getColor = (status) => {
    switch (status) {
      case 'danger': return '#ff0000';
      case 'warning': return '#ffaa00';
      default: return '#00ff00';
    }
  };

  const color = getColor(zone.data.status);
  const isEmergency = zone.data.status === 'danger';

  return (
    <group position={position}>
      <Box
        args={[4, 3, 4]}
        onClick={onClick}
        onPointerOver={() => setHovered(true)}
        onPointerOut={() => setHovered(false)}
      >
        <meshStandardMaterial
          color={color}
          emissive={color}
          emissiveIntensity={isEmergency ? 0.5 : 0}
          transparent
          opacity={hovered ? 0.9 : 0.7}
        />
      </Box>
      
      {/* Zone label */}
      <Html center>
        <div style={{
          background: 'rgba(0,0,0,0.8)',
          color: 'white',
          padding: '5px 10px',
          borderRadius: '5px',
          pointerEvents: 'none',
          fontSize: '12px',
          whiteSpace: 'nowrap'
        }}>
          <strong>{zone.data.metadata?.name || zone.id}</strong>
          <br />
          {zone.data.sensors?.temperature}°C
          {isEmergency && <div style={{color: '#ff0000'}}>🚨 ALERT!</div>}
        </div>
      </Html>
      
      {/* Emergency beacon */}
      {isEmergency && (
        <mesh position={[0, 2, 0]}>
          <sphereGeometry args={[0.3, 16, 16]} />
          <meshStandardMaterial
            color="#ff0000"
            emissive="#ff0000"
            emissiveIntensity={1}
          />
        </mesh>
      )}
    </group>
  );
};

const Building3D = ({ zones, onZoneClick, emergency }) => {
  // Organize zones by floor
  const floorZones = {};
  Object.keys(zones).forEach(zoneId => {
    const floor = zones[zoneId].metadata?.floor || 1;
    if (!floorZones[floor]) floorZones[floor] = [];
    
    // Calculate position for each zone
    const index = floorZones[floor].length;
    const row = Math.floor(index / 4);
    const col = index % 4;
    
    floorZones[floor].push({
      id: zoneId,
      data: zones[zoneId],
      position: [-6 + col * 4, 1.5, -6 + row * 4]
    });
  });

  return (
    <Canvas camera={{ position: [25, 20, 25], fov: 50 }}>
      <ambientLight intensity={0.5} />
      <pointLight position={[10, 30, 10]} intensity={1} />
      <directionalLight position={[-10, 20, 5]} intensity={0.5} />
      
      {/* Draw floors */}
      {Object.keys(floorZones).sort().map((floor, index) => (
        <Floor
          key={floor}
          position={[0, index * 5, 0]}
          zones={floorZones[floor]}
          onZoneClick={onZoneClick}
          floorNumber={floor}
        />
      ))}
      
      {/* Evacuation paths (if emergency) */}
      {emergency && emergency.active && (
        <group>
          {/* Draw green exit path */}
          <arrowHelper
            args={[
              new THREE.Vector3(1, 0, 0),
              new THREE.Vector3(-10, 0, 0),
              15,
              0x00ff00,
              2,
              2
            ]}
          />
          <Text position={[-10, 8, 0]} fontSize={1.5} color="#00ff00">
            ← EMERGENCY EXIT
          </Text>
        </group>
      )}
      
      <OrbitControls enablePan={true} enableZoom={true} enableRotate={true} />
      <gridHelper args={[50, 50]} />
    </Canvas>
  );
};

export default Building3D;
```

---

### 5. `src/components/ZoneCard.jsx`

```javascript
import React from 'react';
import { Paper, Typography, Box, Chip, Divider } from '@mui/material';
import { 
  Thermostat, 
  SmokingRooms, 
  Air, 
  LocalFireDepartment 
} from '@mui/icons-material';

const ZoneCard = ({ zone, zoneName }) => {
  const getStatusColor = (status) => {
    switch (status) {
      case 'danger': return 'error';
      case 'warning': return 'warning';
      default: return 'success';
    }
  };

  const getStatusLabel = (status) => {
    switch (status) {
      case 'danger': return '🔴 DANGER';
      case 'warning': return '🟡 WARNING';
      default: return '🟢 SAFE';
    }
  };

  const formatTimestamp = (timestamp) => {
    if (!timestamp) return 'N/A';
    const date = new Date(timestamp * 1000);
    return date.toLocaleString();
  };

  return (
    <Paper sx={{ p: 3, height: '100%' }}>
      <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
        <Typography variant="h5">
          {zone.metadata?.name || zoneName}
        </Typography>
        <Chip
          label={getStatusLabel(zone.status)}
          color={getStatusColor(zone.status)}
          size="large"
        />
      </Box>

      <Typography variant="body2" color="text.secondary" gutterBottom>
        Floor: {zone.metadata?.floor} | Type: {zone.metadata?.type}
      </Typography>

      <Divider sx={{ my: 2 }} />

      <Typography variant="h6" gutterBottom>Sensor Readings</Typography>

      <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
        <Box display="flex" alignItems="center" justifyContent="space-between">
          <Box display="flex" alignItems="center" gap={1}>
            <Thermostat color="primary" />
            <Typography>Temperature:</Typography>
          </Box>
          <Typography variant="h6">
            {zone.sensors?.temperature !== undefined 
              ? `${zone.sensors.temperature}°C`
              : 'N/A'}
          </Typography>
        </Box>

        <Box display="flex" alignItems="center" justifyContent="space-between">
          <Box display="flex" alignItems="center" gap={1}>
            <SmokingRooms color="primary" />
            <Typography>Smoke Level:</Typography>
          </Box>
          <Typography variant="h6">
            {zone.sensors?.smoke || 'N/A'}
          </Typography>
        </Box>

        <Box display="flex" alignItems="center" justifyContent="space-between">
          <Box display="flex" alignItems="center" gap={1}>
            <Air color="primary" />
            <Typography>Air Quality:</Typography>
          </Box>
          <Typography variant="h6">
            {zone.sensors?.airQuality 
              ? `${zone.sensors.airQuality.toFixed(0)} PPM`
              : 'N/A'}
          </Typography>
        </Box>

        <Box display="flex" alignItems="center" justifyContent="space-between">
          <Box display="flex" alignItems="center" gap={1}>
            <LocalFireDepartment color="primary" />
            <Typography>Flame Sensor:</Typography>
          </Box>
          <Typography variant="h6">
            {zone.sensors?.flame || 'N/A'}
          </Typography>
        </Box>
      </Box>

      <Divider sx={{ my: 2 }} />

      <Typography variant="caption" color="text.secondary">
        Last Update: {formatTimestamp(zone.sensors?.timestamp)}
      </Typography>
    </Paper>
  );
};

export default ZoneCard;
```

---

### 6. `src/components/EmergencyPanel.jsx`

```javascript
import React from 'react';
import { Paper, Typography, Box, Button, Alert } from '@mui/material';
import { Phone, Email, LocationOn } from '@mui/icons-material';

const EmergencyPanel = ({ emergency, buildingId }) => {
  const handleAcknowledge = () => {
    // Acknowledge emergency in Firebase
    console.log('Emergency acknowledged');
  };

  return (
    <Paper 
      sx={{ 
        p: 3, 
        mb: 3, 
        bgcolor: '#ff0000', 
        color: 'white',
        animation: 'pulse 1s infinite'
      }}
    >
      <Typography variant="h4" gutterBottom>
        🚨 EMERGENCY ACTIVE 🚨
      </Typography>
      
      <Box sx={{ my: 2 }}>
        <Typography variant="h6">
          Type: {emergency.type}
        </Typography>
        <Typography variant="body1">
          Zone: {emergency.zoneName} | Floor: {emergency.floor}
        </Typography>
        <Typography variant="body1">
          Status: {emergency.status?.toUpperCase()}
        </Typography>
      </Box>

      <Box sx={{ display: 'flex', gap: 2, flexWrap: 'wrap' }}>
        {emergency.notifyFireBrigade && (
          <Alert severity="error" icon={<Phone />}>
            Fire Brigade: NOTIFIED
          </Alert>
        )}
        {emergency.notifyOwner && (
          <Alert severity="warning" icon={<Email />}>
            Owner: NOTIFIED
          </Alert>
        )}
        {emergency.evacuationRequired && (
          <Alert severity="error" icon={<LocationOn />}>
            EVACUATION REQUIRED - USE DESIGNATED EXITS
          </Alert>
        )}
      </Box>

      <Box sx={{ mt: 2 }}>
        <Button 
          variant="contained" 
          color="warning" 
          size="large"
          onClick={handleAcknowledge}
        >
          Acknowledge Emergency
        </Button>
      </Box>

      <style jsx>{`
        @keyframes pulse {
          0%, 100% { opacity: 1; }
          50% { opacity: 0.8; }
        }
      `}</style>
    </Paper>
  );
};

export default EmergencyPanel;
```

---

### 7. `src/App.js`

```javascript
import React from 'react';
import { ThemeProvider, createTheme } from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';
import Dashboard from './components/Dashboard';
import './App.css';

const theme = createTheme({
  palette: {
    mode: 'light',
    primary: {
      main: '#1976d2',
    },
    error: {
      main: '#ff0000',
    },
    warning: {
      main: '#ffaa00',
    },
    success: {
      main: '#00ff00',
    },
  },
});

function App() {
  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <div className="App">
        <Dashboard />
      </div>
    </ThemeProvider>
  );
}

export default App;
```

---

## 🚀 Deployment

```bash
# Build for production
npm run build

# Deploy to Firebase Hosting
firebase init hosting
firebase deploy --only hosting
```

Your web dashboard will be live at: `https://your-project.web.app`

---

**Continue to: AlertsList, SensorChart, and EvacuationMap components** →
