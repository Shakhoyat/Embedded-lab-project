import { useState, useEffect } from 'react';
import SmartBuildingMonitor from './SmartBuildingMonitor';
import NotificationManager from './NotificationManager';
import EmergencyOverlay from './EmergencyOverlay';

/**
 * Enhanced Smart Building Monitor with Advanced Manager Notifications
 * Integrates comprehensive fire emergency management system
 */
function EnhancedSmartBuildingMonitor() {
    const [showEmergencyOverlay, setShowEmergencyOverlay] = useState(false);
    const [currentEmergency, setCurrentEmergency] = useState(null);
    const [managerMode, setManagerMode] = useState(false);
    const [alerts, setAlerts] = useState([]);
    const [emergencyNotifications, setEmergencyNotifications] = useState([]);
    const [segments, setSegments] = useState({});
    const [systemEmergency, setSystemEmergency] = useState(false);

    // Manager authentication (simplified for demo)
    const [isManagerAuthenticated, setIsManagerAuthenticated] = useState(false);
    const [managerCredentials, setManagerCredentials] = useState({ username: '', password: '' });

    useEffect(() => {
        // Check for critical fire emergencies
        const fireEmergencies = alerts.filter(alert =>
            (alert.type === 'EMERGENCY' || alert.type === 'ARDUINO_EMERGENCY') &&
            (alert.cause?.toLowerCase().includes('fire') ||
                alert.cause?.toLowerCase().includes('flame') ||
                alert.segment) &&
            !alert.acknowledged
        );

        if (fireEmergencies.length > 0 && !showEmergencyOverlay) {
            const mostRecentEmergency = fireEmergencies[0];
            setCurrentEmergency(mostRecentEmergency);
            setShowEmergencyOverlay(true);
        }
    }, [alerts, showEmergencyOverlay]);

    const handleManagerAlert = (emergencyData) => {
        console.log('Manager alert triggered:', emergencyData);

        // If this is a fire emergency, show the overlay
        if (emergencyData.type === 'FIRE_EMERGENCY') {
            setCurrentEmergency(emergencyData);
            setShowEmergencyOverlay(true);
        }
    };

    const handleEmergencyAcknowledge = () => {
        setShowEmergencyOverlay(false);
        setCurrentEmergency(null);

        // In a real system, this would update the alert status in Firebase
        console.log('Emergency acknowledged by manager');
    };

    const handleEmergencyDismiss = () => {
        setShowEmergencyOverlay(false);
        setCurrentEmergency(null);

        // Log false alarm
        console.log('Emergency dismissed as false alarm');
    };

    const handleDispatchEmergencyServices = () => {
        // Simulate emergency services dispatch
        const confirmDispatch = window.confirm(
            '🚨 DISPATCH EMERGENCY SERVICES? 🚨\n\n' +
            'This will immediately contact:\n' +
            '• Fire Department (01303488507)\n' +
            '• Building Management\n' +
            '• Security Services\n\n' +
            'Continue with emergency dispatch?'
        );

        if (confirmDispatch) {
            alert(
                '🚒 EMERGENCY SERVICES DISPATCHED! 🚒\n\n' +
                '✓ Fire Department contacted\n' +
                '✓ Emergency crews en route\n' +
                '✓ Building management notified\n' +
                '✓ Security team alerted\n\n' +
                'Estimated arrival time: 5-8 minutes'
            );

            setShowEmergencyOverlay(false);
            setCurrentEmergency(null);
        }
    };

    const authenticateManager = () => {
        // Simple demo authentication
        const validCredentials = {
            username: 'manager',
            password: 'emergency123'
        };

        if (managerCredentials.username === validCredentials.username &&
            managerCredentials.password === validCredentials.password) {
            setIsManagerAuthenticated(true);
            setManagerMode(true);
        } else {
            alert('Invalid credentials. Demo credentials:\nUsername: manager\nPassword: emergency123');
        }
    };

    const ManagerLoginModal = () => (
        <div className="fixed inset-0 bg-black/80 backdrop-blur-sm z-50 flex items-center justify-center">
            <div className="bg-gray-900 border border-gray-600 rounded-2xl p-8 max-w-md mx-4">
                <div className="text-center mb-6">
                    <div className="w-16 h-16 bg-blue-600 rounded-full flex items-center justify-center mx-auto mb-4">
                        <span className="text-2xl">👨‍💼</span>
                    </div>
                    <h2 className="text-2xl font-bold text-white">Manager Access</h2>
                    <p className="text-gray-400 mt-2">Enhanced emergency management controls</p>
                </div>

                <div className="space-y-4">
                    <div>
                        <label className="block text-white text-sm font-medium mb-2">Username</label>
                        <input
                            type="text"
                            value={managerCredentials.username}
                            onChange={(e) => setManagerCredentials(prev => ({
                                ...prev,
                                username: e.target.value
                            }))}
                            className="w-full px-4 py-3 bg-gray-800 border border-gray-600 rounded-lg text-white focus:border-blue-500 focus:outline-none"
                            placeholder="Enter username"
                        />
                    </div>

                    <div>
                        <label className="block text-white text-sm font-medium mb-2">Password</label>
                        <input
                            type="password"
                            value={managerCredentials.password}
                            onChange={(e) => setManagerCredentials(prev => ({
                                ...prev,
                                password: e.target.value
                            }))}
                            className="w-full px-4 py-3 bg-gray-800 border border-gray-600 rounded-lg text-white focus:border-blue-500 focus:outline-none"
                            placeholder="Enter password"
                            onKeyPress={(e) => e.key === 'Enter' && authenticateManager()}
                        />
                    </div>

                    <div className="flex space-x-3 pt-4">
                        <button
                            onClick={authenticateManager}
                            className="flex-1 bg-blue-600 hover:bg-blue-700 text-white py-3 rounded-lg font-medium transition-colors"
                        >
                            Login
                        </button>
                        <button
                            onClick={() => setManagerMode(false)}
                            className="flex-1 bg-gray-600 hover:bg-gray-700 text-white py-3 rounded-lg font-medium transition-colors"
                        >
                            Cancel
                        </button>
                    </div>
                </div>

                <div className="mt-6 p-4 bg-blue-900/20 border border-blue-500/30 rounded-lg">
                    <p className="text-blue-200 text-sm text-center">
                        <strong>Demo Credentials:</strong><br />
                        Username: manager<br />
                        Password: emergency123
                    </p>
                </div>
            </div>
        </div>
    );

    const ManagerModeToggle = () => (
        <button
            onClick={() => {
                if (isManagerAuthenticated) {
                    setManagerMode(!managerMode);
                } else {
                    setManagerMode(true);
                }
            }}
            className={`fixed bottom-6 right-6 z-40 px-6 py-3 rounded-full font-bold text-white shadow-lg transition-all transform hover:scale-105 ${managerMode
                    ? 'bg-red-600 hover:bg-red-700'
                    : 'bg-blue-600 hover:bg-blue-700'
                }`}
        >
            {managerMode ? '👨‍💼 Manager Mode: ON' : '👨‍💼 Enable Manager Mode'}
        </button>
    );

    return (
        <div className="enhanced-smart-building-monitor">
            {/* Emergency Overlay */}
            <EmergencyOverlay
                isVisible={showEmergencyOverlay}
                emergencyData={currentEmergency}
                onAcknowledge={handleEmergencyAcknowledge}
                onDismiss={handleEmergencyDismiss}
                onDispatchEmergencyServices={handleDispatchEmergencyServices}
            />

            {/* Manager Login Modal */}
            {managerMode && !isManagerAuthenticated && <ManagerLoginModal />}

            {/* Main Building Monitor */}
            <SmartBuildingMonitor
                onAlertsChange={setAlerts}
                onEmergencyNotificationsChange={setEmergencyNotifications}
                onSegmentsChange={setSegments}
                onSystemEmergencyChange={setSystemEmergency}
            />

            {/* Enhanced Manager Notifications (only visible in manager mode) */}
            {managerMode && isManagerAuthenticated && (
                <div className="fixed top-20 left-4 right-4 z-30 pointer-events-none">
                    <div className="pointer-events-auto">
                        <NotificationManager
                            alerts={alerts}
                            emergencyNotifications={emergencyNotifications}
                            systemEmergency={systemEmergency}
                            segments={segments}
                            onManagerAlert={handleManagerAlert}
                        />
                    </div>
                </div>
            )}

            {/* Manager Mode Toggle */}
            <ManagerModeToggle />

            {/* Emergency Status Banner */}
            {systemEmergency && (
                <div className="fixed top-0 left-0 right-0 z-50 bg-red-600 text-white p-3 text-center font-bold animate-pulse">
                    🚨 SYSTEM EMERGENCY ACTIVE - IMMEDIATE ATTENTION REQUIRED 🚨
                </div>
            )}

            {/* Manager Mode Indicator */}
            {managerMode && isManagerAuthenticated && (
                <div className="fixed top-4 left-4 z-40 bg-green-600 text-white px-4 py-2 rounded-lg shadow-lg">
                    <div className="flex items-center space-x-2">
                        <div className="w-2 h-2 bg-white rounded-full animate-pulse"></div>
                        <span className="font-bold">Manager Mode Active</span>
                    </div>
                </div>
            )}
        </div>
    );
}

export default EnhancedSmartBuildingMonitor;