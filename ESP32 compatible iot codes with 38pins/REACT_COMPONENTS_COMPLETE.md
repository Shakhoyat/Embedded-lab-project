# Additional React Components

## 📊 `src/components/SensorChart.jsx`

```javascript
import React, { useState, useEffect } from 'react';
import { Paper, Typography, Box, ToggleButton, ToggleButtonGroup } from '@mui/material';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { database, ref, onValue } from '../firebase/config';

const SensorChart = ({ zone, zoneName, buildingId }) => {
  const [historicalData, setHistoricalData] = useState([]);
  const [selectedSensor, setSelectedSensor] = useState('temperature');
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // Fetch historical data
    const historyRef = ref(database, `buildings/${buildingId}/history/${zoneName}`);
    
    const unsubscribe = onValue(historyRef, (snapshot) => {
      const data = snapshot.val();
      if (data) {
        const dataArray = Object.keys(data)
          .map(timestamp => ({
            timestamp: parseInt(timestamp),
            time: new Date(parseInt(timestamp) * 1000).toLocaleTimeString(),
            ...data[timestamp]
          }))
          .sort((a, b) => a.timestamp - b.timestamp)
          .slice(-20); // Last 20 data points
        
        setHistoricalData(dataArray);
      }
      setLoading(false);
    });

    return () => unsubscribe();
  }, [buildingId, zoneName]);

  const handleSensorChange = (event, newSensor) => {
    if (newSensor !== null) {
      setSelectedSensor(newSensor);
    }
  };

  const getSensorConfig = (sensorType) => {
    const configs = {
      temperature: {
        dataKey: 'temperature',
        name: 'Temperature (°C)',
        stroke: '#ff7300',
        unit: '°C'
      },
      smoke: {
        dataKey: 'smoke',
        name: 'Smoke Level',
        stroke: '#8884d8',
        unit: ''
      },
      airQuality: {
        dataKey: 'airQuality',
        name: 'Air Quality (PPM)',
        stroke: '#82ca9d',
        unit: 'PPM'
      },
      flame: {
        dataKey: 'flame',
        name: 'Flame Sensor',
        stroke: '#ff0000',
        unit: ''
      }
    };
    return configs[sensorType];
  };

  const config = getSensorConfig(selectedSensor);

  return (
    <Paper sx={{ p: 3 }}>
      <Typography variant="h6" gutterBottom>
        {zone.metadata?.name || zoneName} - Sensor History
      </Typography>

      <Box sx={{ mb: 2 }}>
        <ToggleButtonGroup
          value={selectedSensor}
          exclusive
          onChange={handleSensorChange}
          size="small"
        >
          <ToggleButton value="temperature">Temperature</ToggleButton>
          <ToggleButton value="smoke">Smoke</ToggleButton>
          <ToggleButton value="airQuality">Air Quality</ToggleButton>
          <ToggleButton value="flame">Flame</ToggleButton>
        </ToggleButtonGroup>
      </Box>

      {loading ? (
        <Typography>Loading chart data...</Typography>
      ) : historicalData.length === 0 ? (
        <Typography>No historical data available</Typography>
      ) : (
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={historicalData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis 
              dataKey="time" 
              tick={{ fontSize: 12 }}
            />
            <YAxis 
              label={{ value: config.unit, angle: -90, position: 'insideLeft' }}
            />
            <Tooltip />
            <Legend />
            <Line 
              type="monotone" 
              dataKey={config.dataKey} 
              stroke={config.stroke} 
              name={config.name}
              strokeWidth={2}
              dot={{ r: 3 }}
              activeDot={{ r: 5 }}
            />
          </LineChart>
        </ResponsiveContainer>
      )}

      <Typography variant="caption" color="text.secondary" sx={{ mt: 2, display: 'block' }}>
        Showing last 20 readings
      </Typography>
    </Paper>
  );
};

export default SensorChart;
```

---

## 📋 `src/components/AlertsList.jsx`

```javascript
import React, { useState } from 'react';
import { 
  Paper, 
  Typography, 
  Box, 
  List, 
  ListItem, 
  ListItemText, 
  Chip,
  Button,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Divider,
  Alert
} from '@mui/material';
import { 
  Warning, 
  Error, 
  CheckCircle,
  LocationOn,
  AccessTime 
} from '@mui/icons-material';
import { database, ref, set } from '../firebase/config';

const AlertsList = ({ alerts, buildingId }) => {
  const [selectedAlert, setSelectedAlert] = useState(null);
  const [dialogOpen, setDialogOpen] = useState(false);

  const handleAlertClick = (alert) => {
    setSelectedAlert(alert);
    setDialogOpen(true);
  };

  const handleAcknowledge = async (alertId) => {
    try {
      await set(
        ref(database, `buildings/${buildingId}/alerts/${alertId}/acknowledged`),
        true
      );
      setDialogOpen(false);
    } catch (error) {
      console.error('Error acknowledging alert:', error);
    }
  };

  const handleResolve = async (alertId) => {
    try {
      await set(
        ref(database, `buildings/${buildingId}/alerts/${alertId}/resolved`),
        true
      );
      setDialogOpen(false);
    } catch (error) {
      console.error('Error resolving alert:', error);
    }
  };

  const getSeverityIcon = (severity) => {
    switch (severity) {
      case 'CRITICAL':
        return <Error color="error" />;
      case 'WARNING':
        return <Warning color="warning" />;
      default:
        return <CheckCircle color="success" />;
    }
  };

  const getSeverityColor = (severity) => {
    switch (severity) {
      case 'CRITICAL':
        return 'error';
      case 'WARNING':
        return 'warning';
      default:
        return 'success';
    }
  };

  const formatTimestamp = (timestamp) => {
    const date = new Date(timestamp * 1000);
    return date.toLocaleString();
  };

  const getRelativeTime = (timestamp) => {
    const now = Date.now();
    const alertTime = timestamp * 1000;
    const diff = now - alertTime;
    
    const minutes = Math.floor(diff / 60000);
    const hours = Math.floor(diff / 3600000);
    const days = Math.floor(diff / 86400000);
    
    if (minutes < 1) return 'Just now';
    if (minutes < 60) return `${minutes} min ago`;
    if (hours < 24) return `${hours} hour${hours > 1 ? 's' : ''} ago`;
    return `${days} day${days > 1 ? 's' : ''} ago`;
  };

  return (
    <>
      <Paper sx={{ p: 3 }}>
        <Typography variant="h5" gutterBottom>
          🚨 Alerts History
        </Typography>
        
        <Typography variant="body2" color="text.secondary" gutterBottom>
          Total Alerts: {alerts.length} | 
          Unacknowledged: {alerts.filter(a => !a.acknowledged).length} | 
          Unresolved: {alerts.filter(a => !a.resolved).length}
        </Typography>

        <Divider sx={{ my: 2 }} />

        {alerts.length === 0 ? (
          <Alert severity="success">
            No alerts! All zones are safe. ✅
          </Alert>
        ) : (
          <List>
            {alerts.map((alert) => (
              <ListItem
                key={alert.id}
                sx={{
                  border: 1,
                  borderColor: getSeverityColor(alert.severity) + '.main',
                  borderRadius: 1,
                  mb: 2,
                  bgcolor: alert.resolved ? '#f5f5f5' : 'background.paper',
                  opacity: alert.resolved ? 0.6 : 1,
                  cursor: 'pointer',
                  '&:hover': { bgcolor: 'action.hover' }
                }}
                onClick={() => handleAlertClick(alert)}
              >
                <Box sx={{ display: 'flex', alignItems: 'flex-start', width: '100%', gap: 2 }}>
                  {getSeverityIcon(alert.severity)}
                  
                  <ListItemText
                    primary={
                      <Box display="flex" alignItems="center" gap={1}>
                        <Typography variant="h6">
                          {alert.type.replace(/_/g, ' ')}
                        </Typography>
                        <Chip 
                          label={alert.severity} 
                          color={getSeverityColor(alert.severity)}
                          size="small"
                        />
                        {alert.acknowledged && (
                          <Chip label="Acknowledged" size="small" color="info" />
                        )}
                        {alert.resolved && (
                          <Chip label="Resolved" size="small" color="success" />
                        )}
                      </Box>
                    }
                    secondary={
                      <Box sx={{ mt: 1 }}>
                        <Typography variant="body2" color="text.primary">
                          {alert.message}
                        </Typography>
                        <Box display="flex" alignItems="center" gap={2} mt={1}>
                          <Box display="flex" alignItems="center" gap={0.5}>
                            <LocationOn fontSize="small" />
                            <Typography variant="caption">
                              {alert.zoneName} (Floor {alert.floor})
                            </Typography>
                          </Box>
                          <Box display="flex" alignItems="center" gap={0.5}>
                            <AccessTime fontSize="small" />
                            <Typography variant="caption">
                              {getRelativeTime(alert.timestamp)}
                            </Typography>
                          </Box>
                        </Box>
                      </Box>
                    }
                  />
                </Box>
              </ListItem>
            ))}
          </List>
        )}
      </Paper>

      {/* Alert Detail Dialog */}
      <Dialog 
        open={dialogOpen} 
        onClose={() => setDialogOpen(false)}
        maxWidth="md"
        fullWidth
      >
        {selectedAlert && (
          <>
            <DialogTitle>
              <Box display="flex" alignItems="center" gap={1}>
                {getSeverityIcon(selectedAlert.severity)}
                <Typography variant="h5">
                  {selectedAlert.type.replace(/_/g, ' ')}
                </Typography>
              </Box>
            </DialogTitle>
            <DialogContent>
              <Box sx={{ mb: 2 }}>
                <Chip 
                  label={selectedAlert.severity} 
                  color={getSeverityColor(selectedAlert.severity)}
                />
                {selectedAlert.acknowledged && (
                  <Chip label="Acknowledged" color="info" sx={{ ml: 1 }} />
                )}
                {selectedAlert.resolved && (
                  <Chip label="Resolved" color="success" sx={{ ml: 1 }} />
                )}
              </Box>

              <Typography variant="body1" paragraph>
                <strong>Message:</strong> {selectedAlert.message}
              </Typography>

              <Divider sx={{ my: 2 }} />

              <Typography variant="h6" gutterBottom>Location</Typography>
              <Typography variant="body2">
                Zone: {selectedAlert.zoneName}<br />
                Floor: {selectedAlert.floor}<br />
                Time: {formatTimestamp(selectedAlert.timestamp)}
              </Typography>

              <Divider sx={{ my: 2 }} />

              <Typography variant="h6" gutterBottom>Sensor Data at Alert Time</Typography>
              <Box sx={{ bgcolor: '#f5f5f5', p: 2, borderRadius: 1 }}>
                <Typography variant="body2">
                  🌡️ Temperature: {selectedAlert.sensorData?.temperature}°C<br />
                  💨 Smoke Level: {selectedAlert.sensorData?.smoke}<br />
                  🌫️ Air Quality: {selectedAlert.sensorData?.airQuality?.toFixed(1)} PPM<br />
                  🔥 Flame Sensor: {selectedAlert.sensorData?.flame}
                </Typography>
              </Box>
            </DialogContent>
            <DialogActions>
              {!selectedAlert.acknowledged && (
                <Button 
                  onClick={() => handleAcknowledge(selectedAlert.id)}
                  color="primary"
                  variant="contained"
                >
                  Acknowledge
                </Button>
              )}
              {!selectedAlert.resolved && (
                <Button 
                  onClick={() => handleResolve(selectedAlert.id)}
                  color="success"
                  variant="contained"
                >
                  Mark as Resolved
                </Button>
              )}
              <Button onClick={() => setDialogOpen(false)}>Close</Button>
            </DialogActions>
          </>
        )}
      </Dialog>
    </>
  );
};

export default AlertsList;
```

---

## 🗺️ `src/components/EvacuationMap.jsx`

```javascript
import React from 'react';
import { Paper, Typography, Box, List, ListItem, ListItemIcon, ListItemText, Alert } from '@mui/material';
import { 
  ExitToApp, 
  DirectionsRun, 
  LocalParking, 
  Stairs,
  Warning
} from '@mui/icons-material';

const EvacuationMap = ({ emergency, zones }) => {
  const getEvacuationPlan = () => {
    // Determine affected floors
    const affectedFloors = new Set();
    Object.keys(zones).forEach(zoneId => {
      if (zones[zoneId].status === 'danger') {
        affectedFloors.add(zones[zoneId].metadata?.floor);
      }
    });

    return {
      primaryExit: 'Stairway A - East Wing',
      secondaryExit: 'Stairway B - West Wing',
      assemblyPoint: 'Parking Lot - North Side',
      affectedFloors: Array.from(affectedFloors),
      doNotUse: ['Elevator', 'Main Lobby (if smoke present)']
    };
  };

  const plan = getEvacuationPlan();

  return (
    <Paper sx={{ p: 3 }}>
      <Typography variant="h5" gutterBottom color="error">
        🚨 EVACUATION PLAN
      </Typography>

      {emergency && emergency.active && (
        <Alert severity="error" sx={{ mb: 2 }}>
          <strong>EMERGENCY ACTIVE!</strong> Follow evacuation procedures immediately!
        </Alert>
      )}

      <Box sx={{ mb: 3 }}>
        <Typography variant="h6" gutterBottom>
          Affected Areas:
        </Typography>
        <Typography variant="body1">
          {plan.affectedFloors.length > 0 
            ? `Floors: ${plan.affectedFloors.join(', ')}`
            : 'No active danger zones'}
        </Typography>
      </Box>

      <Typography variant="h6" gutterBottom>
        Evacuation Routes:
      </Typography>

      <List>
        <ListItem>
          <ListItemIcon>
            <ExitToApp color="success" fontSize="large" />
          </ListItemIcon>
          <ListItemText
            primary="Primary Exit"
            secondary={plan.primaryExit}
            primaryTypographyProps={{ variant: 'h6', color: 'success.main' }}
          />
        </ListItem>

        <ListItem>
          <ListItemIcon>
            <ExitToApp color="primary" fontSize="large" />
          </ListItemIcon>
          <ListItemText
            primary="Secondary Exit"
            secondary={plan.secondaryExit}
            primaryTypographyProps={{ variant: 'h6', color: 'primary.main' }}
          />
        </ListItem>

        <ListItem>
          <ListItemIcon>
            <LocalParking color="info" fontSize="large" />
          </ListItemIcon>
          <ListItemText
            primary="Assembly Point"
            secondary={plan.assemblyPoint}
            primaryTypographyProps={{ variant: 'h6', color: 'info.main' }}
          />
        </ListItem>
      </List>

      <Alert severity="warning" sx={{ mt: 2 }}>
        <Typography variant="subtitle2" gutterBottom>
          <strong>DO NOT USE:</strong>
        </Typography>
        <ul style={{ margin: 0, paddingLeft: '20px' }}>
          {plan.doNotUse.map((item, index) => (
            <li key={index}>{item}</li>
          ))}
        </ul>
      </Alert>

      <Box sx={{ mt: 3, p: 2, bgcolor: '#e3f2fd', borderRadius: 1 }}>
        <Typography variant="h6" gutterBottom>
          <DirectionsRun sx={{ mr: 1, verticalAlign: 'middle' }} />
          Evacuation Instructions:
        </Typography>
        <ol style={{ margin: 0, paddingLeft: '20px' }}>
          <li>Stay calm and alert others nearby</li>
          <li>Leave all belongings behind</li>
          <li>Use stairs - DO NOT use elevators</li>
          <li>Close doors behind you (do not lock)</li>
          <li>If smoke is present, stay low to the ground</li>
          <li>Check doors for heat before opening</li>
          <li>Proceed to nearest safe exit</li>
          <li>Gather at assembly point</li>
          <li>Do not re-enter until cleared by authorities</li>
        </ol>
      </Box>

      <Box sx={{ mt: 3, p: 2, bgcolor: '#fff3e0', borderRadius: 1 }}>
        <Typography variant="h6" gutterBottom color="warning.main">
          <Stairs sx={{ mr: 1, verticalAlign: 'middle' }} />
          If Exit is Blocked:
        </Typography>
        <ul style={{ margin: 0, paddingLeft: '20px' }}>
          <li>Return to your room/apartment</li>
          <li>Seal door gaps with wet towels</li>
          <li>Signal from window for help</li>
          <li>Call emergency services (Fire Brigade)</li>
          <li>Stay near window for rescue</li>
        </ul>
      </Box>
    </Paper>
  );
};

export default EvacuationMap;
```

---

## 🎨 `src/App.css`

```css
.App {
  min-height: 100vh;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  padding: 20px;
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.7;
  }
}

.emergency-banner {
  animation: pulse 1s infinite;
}

/* Scrollbar styling */
::-webkit-scrollbar {
  width: 10px;
}

::-webkit-scrollbar-track {
  background: #f1f1f1;
}

::-webkit-scrollbar-thumb {
  background: #888;
  border-radius: 5px;
}

::-webkit-scrollbar-thumb:hover {
  background: #555;
}

/* Loading animation */
@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

.loading-spinner {
  border: 4px solid #f3f3f3;
  border-top: 4px solid #3498db;
  border-radius: 50%;
  width: 40px;
  height: 40px;
  animation: spin 1s linear infinite;
  margin: 20px auto;
}
```

---

## 📦 `package.json`

```json
{
  "name": "fire-alarm-dashboard",
  "version": "1.0.0",
  "private": true,
  "dependencies": {
    "@emotion/react": "^11.11.1",
    "@emotion/styled": "^11.11.0",
    "@mui/icons-material": "^5.14.18",
    "@mui/material": "^5.14.18",
    "@react-three/drei": "^9.92.0",
    "@react-three/fiber": "^8.15.11",
    "firebase": "^10.7.0",
    "react": "^18.2.0",
    "react-dom": "^18.2.0",
    "react-router-dom": "^6.20.0",
    "react-scripts": "5.0.1",
    "react-toastify": "^9.1.3",
    "recharts": "^2.10.3",
    "three": "^0.159.0"
  },
  "scripts": {
    "start": "react-scripts start",
    "build": "react-scripts build",
    "test": "react-scripts test",
    "eject": "react-scripts eject"
  },
  "eslintConfig": {
    "extends": [
      "react-app"
    ]
  },
  "browserslist": {
    "production": [
      ">0.2%",
      "not dead",
      "not op_mini all"
    ],
    "development": [
      "last 1 chrome version",
      "last 1 firefox version",
      "last 1 safari version"
    ]
  }
}
```

---

## 🚀 Quick Setup Commands

```bash
# Create the project
npx create-react-app fire-alarm-dashboard
cd fire-alarm-dashboard

# Install all dependencies
npm install firebase three @react-three/fiber @react-three/drei recharts react-router-dom react-toastify @mui/material @mui/icons-material @emotion/react @emotion/styled

# Copy all component files to their respective locations

# Start the development server
npm start
```

Your React dashboard will be running at `http://localhost:3000`

---

## 🔥 Features Summary

✅ **Real-time sensor monitoring** from multiple ESP32 devices  
✅ **3D building visualization** with color-coded zones  
✅ **Interactive zone details** with live sensor readings  
✅ **Alerts history** with acknowledgment system  
✅ **Historical charts** for trend analysis  
✅ **Evacuation maps** with safe exit routes  
✅ **Emergency notifications** (browser alerts + sound)  
✅ **Responsive design** for desktop and mobile  
✅ **Firebase real-time sync** with automatic updates  

---

## 📱 Mobile Responsive

The dashboard is fully responsive and works on:
- Desktop browsers
- Tablets
- Mobile phones (iOS & Android)

---

## 🔐 Security Tips

1. Add Firebase Authentication
2. Implement user roles (Admin, Viewer, Emergency Responder)
3. Secure database rules
4. Enable HTTPS only
5. Add rate limiting for API calls

---

**Your complete IoT Fire Alarm System is ready! 🎉**
