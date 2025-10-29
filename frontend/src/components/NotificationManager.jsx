import { useEffect, useState, useRef } from 'react';
import { database } from '../firebase/config';
import { ref, push, serverTimestamp } from 'firebase/database';

/**
 * Advanced Notification Manager for Fire Emergency Alerts
 * Provides multiple notification channels for manager alerting
 */
function NotificationManager({
    alerts = [],
    emergencyNotifications = [],
    systemEmergency = false,
    segments = {},
    onManagerAlert
}) {
    const [notificationSettings, setNotificationSettings] = useState({
        browserNotifications: true,
        soundAlerts: true,
        emailAlerts: false, // Would need backend service
        smsAlerts: false,   // Would need backend service
        escalationEnabled: true,
        escalationDelay: 300000, // 5 minutes
        managerContacts: [
            { name: 'Primary Manager', email: 'manager@company.com', phone: '+1234567890' },
            { name: 'Secondary Manager', email: 'backup@company.com', phone: '+1234567891' }
        ]
    });

    const [activeAlerts, setActiveAlerts] = useState([]);
    const [managerNotificationsSent, setManagerNotificationsSent] = useState(new Set());
    const [escalationTimers, setEscalationTimers] = useState(new Map());
    const [alertHistory, setAlertHistory] = useState([]);

    // Audio refs for different alert types
    const emergencyAudioRef = useRef(null);
    const warningAudioRef = useRef(null);
    const ackAudioRef = useRef(null);

    // Auto-dismiss timers
    const [dismissTimers, setDismissTimers] = useState(new Map());

    useEffect(() => {
        // Initialize audio elements
        emergencyAudioRef.current = new Audio('/sounds/emergency-siren.mp3');
        warningAudioRef.current = new Audio('/sounds/warning-beep.mp3');
        ackAudioRef.current = new Audio('/sounds/acknowledgment.mp3');

        // Set audio properties
        [emergencyAudioRef, warningAudioRef, ackAudioRef].forEach(audioRef => {
            if (audioRef.current) {
                audioRef.current.loop = false;
                audioRef.current.volume = 0.7;
                audioRef.current.preload = 'auto';
            }
        });

        return () => {
            // Cleanup timers
            escalationTimers.forEach(timer => clearTimeout(timer));
            dismissTimers.forEach(timer => clearTimeout(timer));
        };
    }, []);

    // Monitor for new fire emergencies
    useEffect(() => {
        const fireEmergencies = alerts.filter(alert =>
            (alert.type === 'EMERGENCY' || alert.type === 'ARDUINO_EMERGENCY') &&
            (alert.cause?.includes('Fire') || alert.cause?.includes('Flame') || alert.segment)
        );

        fireEmergencies.forEach(alert => {
            if (!managerNotificationsSent.has(alert.id)) {
                handleFireEmergencyAlert(alert);
                setManagerNotificationsSent(prev => new Set([...prev, alert.id]));
            }
        });
    }, [alerts]);

    // Handle fire emergency with comprehensive notification
    const handleFireEmergencyAlert = async (alert) => {
        const emergencyData = {
            id: alert.id,
            timestamp: new Date().toISOString(),
            type: 'FIRE_EMERGENCY',
            segment: alert.segment,
            cause: alert.cause,
            severity: 'CRITICAL',
            status: 'ACTIVE',
            managerNotified: false,
            escalated: false
        };

        setActiveAlerts(prev => [...prev, emergencyData]);

        // Immediate notifications
        await sendImmediateNotifications(emergencyData);

        // Set up escalation timer if enabled
        if (notificationSettings.escalationEnabled) {
            const timerId = setTimeout(() => {
                escalateAlert(emergencyData);
            }, notificationSettings.escalationDelay);

            setEscalationTimers(prev => new Map(prev.set(alert.id, timerId)));
        }

        // Add to history
        setAlertHistory(prev => [emergencyData, ...prev.slice(0, 49)]); // Keep last 50
    };

    // Send immediate notifications
    const sendImmediateNotifications = async (emergencyData) => {
        // Browser notification
        if (notificationSettings.browserNotifications) {
            sendBrowserNotification(emergencyData);
        }

        // Sound alert
        if (notificationSettings.soundAlerts) {
            playEmergencySound();
        }

        // Visual alert overlay
        showEmergencyOverlay(emergencyData);

        // Save to Firebase for backend processing
        await saveEmergencyToFirebase(emergencyData);

        // Call parent component callback
        if (onManagerAlert) {
            onManagerAlert(emergencyData);
        }
    };

    // Enhanced browser notification
    const sendBrowserNotification = (emergencyData) => {
        if ('Notification' in window && Notification.permission === 'granted') {
            const notification = new Notification('🔥 FIRE EMERGENCY ALERT 🔥', {
                body: `Fire detected in ${emergencyData.segment}!\nImmediate action required.\nCause: ${emergencyData.cause}`,
                icon: '/fire-emergency-icon.png',
                badge: '/fire-badge.png',
                requireInteraction: true,
                persistent: true,
                tag: `fire-emergency-${emergencyData.id}`,
                data: emergencyData,
                actions: [
                    { action: 'acknowledge', title: 'Acknowledge' },
                    { action: 'dispatch', title: 'Dispatch Emergency Services' }
                ],
                vibrate: [200, 100, 200, 100, 200, 100, 200],
                silent: false
            });

            notification.onclick = () => {
                window.focus();
                notification.close();
                acknowledgeAlert(emergencyData.id);
            };

            // Auto-close after 30 seconds if not interacted with
            setTimeout(() => {
                notification.close();
            }, 30000);
        }
    };

    // Play emergency sound
    const playEmergencySound = () => {
        if (emergencyAudioRef.current) {
            emergencyAudioRef.current.currentTime = 0;
            emergencyAudioRef.current.play().catch(console.error);
        }
    };

    // Show emergency overlay
    const showEmergencyOverlay = (emergencyData) => {
        // This would trigger a full-screen emergency modal
        // Implementation depends on your modal system
        console.log('Emergency overlay triggered for:', emergencyData);
    };

    // Save emergency data to Firebase
    const saveEmergencyToFirebase = async (emergencyData) => {
        try {
            const managerAlertsRef = ref(database, 'smartBuilding/managerAlerts');
            await push(managerAlertsRef, {
                ...emergencyData,
                timestamp: serverTimestamp(),
                notificationChannels: {
                    browser: notificationSettings.browserNotifications,
                    sound: notificationSettings.soundAlerts,
                    email: notificationSettings.emailAlerts,
                    sms: notificationSettings.smsAlerts
                },
                managerContacts: notificationSettings.managerContacts
            });
        } catch (error) {
            console.error('Failed to save emergency to Firebase:', error);
        }
    };

    // Escalate alert to secondary contacts
    const escalateAlert = async (emergencyData) => {
        const escalatedData = {
            ...emergencyData,
            escalated: true,
            escalationTime: new Date().toISOString(),
            severity: 'CRITICAL_ESCALATED'
        };

        // Send escalation notifications
        sendBrowserNotification({
            ...escalatedData,
            segment: `${escalatedData.segment} - ESCALATED`
        });

        // Update active alerts
        setActiveAlerts(prev =>
            prev.map(alert =>
                alert.id === emergencyData.id ? escalatedData : alert
            )
        );

        // Save escalation to Firebase
        await saveEmergencyToFirebase(escalatedData);
    };

    // Acknowledge alert
    const acknowledgeAlert = async (alertId) => {
        // Play acknowledgment sound
        if (ackAudioRef.current) {
            ackAudioRef.current.play().catch(console.error);
        }

        // Clear escalation timer
        const timer = escalationTimers.get(alertId);
        if (timer) {
            clearTimeout(timer);
            setEscalationTimers(prev => {
                const newMap = new Map(prev);
                newMap.delete(alertId);
                return newMap;
            });
        }

        // Update alert status
        setActiveAlerts(prev =>
            prev.map(alert =>
                alert.id === alertId
                    ? { ...alert, status: 'ACKNOWLEDGED', acknowledgedAt: new Date().toISOString() }
                    : alert
            )
        );

        // Save acknowledgment to Firebase
        try {
            const ackRef = ref(database, `smartBuilding/managerAlerts/${alertId}/acknowledgment`);
            await push(ackRef, {
                acknowledgedAt: serverTimestamp(),
                acknowledgedBy: 'Manager', // In real app, this would be the logged-in user
                status: 'ACKNOWLEDGED'
            });
        } catch (error) {
            console.error('Failed to save acknowledgment:', error);
        }
    };

    // Dismiss alert
    const dismissAlert = (alertId) => {
        setActiveAlerts(prev => prev.filter(alert => alert.id !== alertId));

        // Clear any timers
        const escalationTimer = escalationTimers.get(alertId);
        const dismissTimer = dismissTimers.get(alertId);

        if (escalationTimer) clearTimeout(escalationTimer);
        if (dismissTimer) clearTimeout(dismissTimer);
    };

    // Request notification permission
    const requestNotificationPermission = async () => {
        if ('Notification' in window) {
            const permission = await Notification.requestPermission();
            setNotificationSettings(prev => ({
                ...prev,
                browserNotifications: permission === 'granted'
            }));
            return permission === 'granted';
        }
        return false;
    };

    // Fire Emergency Action Buttons Component
    const FireEmergencyActions = ({ alert }) => (
        <div className="flex flex-wrap gap-2 mt-4">
            <button
                onClick={() => acknowledgeAlert(alert.id)}
                className="bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded-lg text-sm font-medium transition-colors"
            >
                ✓ Acknowledge
            </button>
            <button
                onClick={() => {
                    // Simulate dispatching emergency services
                    alert('Emergency services dispatched! Fire department notified.');
                    acknowledgeAlert(alert.id);
                }}
                className="bg-red-600 hover:bg-red-700 text-white px-4 py-2 rounded-lg text-sm font-medium transition-colors"
            >
                🚒 Dispatch Fire Department
            </button>
            <button
                onClick={() => {
                    // Simulate evacuation alert
                    alert('Building evacuation alert sent to all residents!');
                }}
                className="bg-orange-600 hover:bg-orange-700 text-white px-4 py-2 rounded-lg text-sm font-medium transition-colors"
            >
                📢 Evacuation Alert
            </button>
            <button
                onClick={() => dismissAlert(alert.id)}
                className="bg-gray-600 hover:bg-gray-700 text-white px-4 py-2 rounded-lg text-sm font-medium transition-colors"
            >
                ✕ Dismiss
            </button>
        </div>
    );

    return (
        <div className="notification-manager">
            {/* Active Fire Emergency Alerts */}
            {activeAlerts.filter(alert => alert.status === 'ACTIVE').length > 0 && (
                <div className="fixed top-4 right-4 z-50 space-y-4 max-w-md">
                    {activeAlerts
                        .filter(alert => alert.status === 'ACTIVE')
                        .map(alert => (
                            <div
                                key={alert.id}
                                className="bg-red-900 border-2 border-red-500 rounded-xl p-6 shadow-2xl animate-pulse"
                            >
                                <div className="flex items-center space-x-3 mb-3">
                                    <span className="text-3xl">🔥</span>
                                    <div>
                                        <h3 className="text-white font-bold text-lg">FIRE EMERGENCY</h3>
                                        <p className="text-red-200 text-sm">{alert.segment}</p>
                                    </div>
                                </div>

                                <div className="text-white text-sm space-y-1 mb-4">
                                    <p><strong>Location:</strong> {alert.segment}</p>
                                    <p><strong>Cause:</strong> {alert.cause}</p>
                                    <p><strong>Time:</strong> {new Date(alert.timestamp).toLocaleString()}</p>
                                    {alert.escalated && (
                                        <p className="text-yellow-300 font-bold">⚠️ ESCALATED ALERT</p>
                                    )}
                                </div>

                                <FireEmergencyActions alert={alert} />
                            </div>
                        ))
                    }
                </div>
            )}

            {/* Notification Settings Panel */}
            <div className="bg-white/10 backdrop-blur-xl rounded-xl p-6 border border-white/20 mb-6">
                <h3 className="text-xl font-bold text-white mb-4 flex items-center space-x-2">
                    <span>🔔</span>
                    <span>Manager Notification Settings</span>
                </h3>

                <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <div className="space-y-3">
                        <label className="flex items-center space-x-3">
                            <input
                                type="checkbox"
                                checked={notificationSettings.browserNotifications}
                                onChange={(e) => setNotificationSettings(prev => ({
                                    ...prev,
                                    browserNotifications: e.target.checked
                                }))}
                                className="w-4 h-4 text-blue-600"
                            />
                            <span className="text-white">Browser Notifications</span>
                        </label>

                        <label className="flex items-center space-x-3">
                            <input
                                type="checkbox"
                                checked={notificationSettings.soundAlerts}
                                onChange={(e) => setNotificationSettings(prev => ({
                                    ...prev,
                                    soundAlerts: e.target.checked
                                }))}
                                className="w-4 h-4 text-blue-600"
                            />
                            <span className="text-white">Sound Alerts</span>
                        </label>

                        <label className="flex items-center space-x-3">
                            <input
                                type="checkbox"
                                checked={notificationSettings.escalationEnabled}
                                onChange={(e) => setNotificationSettings(prev => ({
                                    ...prev,
                                    escalationEnabled: e.target.checked
                                }))}
                                className="w-4 h-4 text-blue-600"
                            />
                            <span className="text-white">Auto-Escalation</span>
                        </label>
                    </div>

                    <div className="space-y-3">
                        <div>
                            <label className="block text-white text-sm mb-1">Escalation Delay (minutes)</label>
                            <input
                                type="number"
                                min="1"
                                max="30"
                                value={notificationSettings.escalationDelay / 60000}
                                onChange={(e) => setNotificationSettings(prev => ({
                                    ...prev,
                                    escalationDelay: parseInt(e.target.value) * 60000
                                }))}
                                className="w-full px-3 py-2 bg-white/10 border border-white/20 rounded-lg text-white"
                            />
                        </div>

                        <button
                            onClick={requestNotificationPermission}
                            className="w-full bg-blue-600 hover:bg-blue-700 text-white px-4 py-2 rounded-lg text-sm font-medium transition-colors"
                        >
                            Enable Browser Notifications
                        </button>
                    </div>
                </div>
            </div>

            {/* Alert History */}
            {alertHistory.length > 0 && (
                <div className="bg-white/10 backdrop-blur-xl rounded-xl p-6 border border-white/20">
                    <h3 className="text-xl font-bold text-white mb-4 flex items-center space-x-2">
                        <span>📋</span>
                        <span>Manager Alert History</span>
                    </h3>

                    <div className="space-y-3 max-h-60 overflow-y-auto">
                        {alertHistory.map(alert => (
                            <div
                                key={alert.id}
                                className={`p-4 rounded-lg border ${alert.status === 'ACKNOWLEDGED'
                                        ? 'bg-green-900/20 border-green-500/50'
                                        : alert.escalated
                                            ? 'bg-red-900/30 border-red-500/50'
                                            : 'bg-orange-900/20 border-orange-500/50'
                                    }`}
                            >
                                <div className="flex justify-between items-start">
                                    <div>
                                        <p className="text-white font-medium">{alert.segment} - {alert.type}</p>
                                        <p className="text-gray-300 text-sm">{alert.cause}</p>
                                        <p className="text-gray-400 text-xs">{new Date(alert.timestamp).toLocaleString()}</p>
                                    </div>
                                    <span className={`px-2 py-1 rounded text-xs font-bold ${alert.status === 'ACKNOWLEDGED' ? 'bg-green-600 text-white' :
                                            alert.escalated ? 'bg-red-600 text-white' : 'bg-orange-600 text-white'
                                        }`}>
                                        {alert.status}
                                    </span>
                                </div>
                            </div>
                        ))}
                    </div>
                </div>
            )}
        </div>
    );
}

export default NotificationManager;