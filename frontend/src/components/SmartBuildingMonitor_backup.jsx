import { useEffect, useState, useRef } from 'react';
import { database } from '../firebase/config';
import { ref, onValue } from 'firebase/database';

/**
 * Smart Building Monitoring System - Professional Dashboard
 * Monitors 4 segments: Kitchen, Bedroom, Parking, Central Gas Chamber
 * Features: Real-time updates, emergency notifications, advanced analytics
 */
function SmartBuildingMonitor() {
    const [systemData, setSystemData] = useState(null);
    const [segments, setSegments] = useState({});
    const [alerts, setAlerts] = useState([]);
    const [emergencyNotifications, setEmergencyNotifications] = useState([]);
    const [loading, setLoading] = useState(true);
    const [lastUpdate, setLastUpdate] = useState(null);
    const [notificationPermission, setNotificationPermission] = useState('default');
    const [connectionStatus, setConnectionStatus] = useState('connecting');
    const audioRef = useRef(null);

    // Request notification permission on component mount
    useEffect(() => {
        if ('Notification' in window) {
            if (Notification.permission === 'default') {
                Notification.requestPermission().then(permission => {
                    setNotificationPermission(permission);
                });
            } else {
                setNotificationPermission(Notification.permission);
            }
        }
    }, []);

    // Listen to Firebase data
    useEffect(() => {
        const systemRef = ref(database, 'smartBuilding/system');
        const segmentsRef = ref(database, 'smartBuilding/segments');
        const alertsRef = ref(database, 'smartBuilding/alerts');
        const emergencyRef = ref(database, 'smartBuilding/emergency_notifications');

        // System data listener
        const unsubscribeSystem = onValue(systemRef, (snapshot) => {
            const data = snapshot.val();
            setSystemData(data || null);
            setLastUpdate(new Date());
            setConnectionStatus('connected');
            setLoading(false);
        }, (error) => {
            console.error('Error fetching system data:', error);
            setConnectionStatus('error');
            setLoading(false);
        });

        // Segments data listener
        const unsubscribeSegments = onValue(segmentsRef, (snapshot) => {
            const data = snapshot.val();
            setSegments(data || {});
        });

        // Alerts listener
        const unsubscribeAlerts = onValue(alertsRef, (snapshot) => {
            const data = snapshot.val();
            if (data) {
                const alertsArray = Object.entries(data).map(([id, alert]) => ({
                    id,
                    ...alert
                })).sort((a, b) => b.id - a.id).slice(0, 10); // Latest 10 alerts

                setAlerts(alertsArray);

                // Show browser notification for new emergency alerts
                alertsArray.forEach(alert => {
                    if (alert.type === 'EMERGENCY' && !alert.acknowledged) {
                        showBrowserNotification(alert);
                        playAlertSound();
                    }
                });
            }
        });

        // Emergency notifications listener
        const unsubscribeEmergency = onValue(emergencyRef, (snapshot) => {
            const data = snapshot.val();
            if (data) {
                const emergencyArray = Object.entries(data).map(([id, notification]) => ({
                    id,
                    ...notification
                })).sort((a, b) => b.id - a.id).slice(0, 5);

                setEmergencyNotifications(emergencyArray);
            }
        });

        return () => {
            unsubscribeSystem();
            unsubscribeSegments();
            unsubscribeAlerts();
            unsubscribeEmergency();
        };
    }, []);

    // Show browser notification
    const showBrowserNotification = (alert) => {
        if (notificationPermission === 'granted' && alert.notification) {
            new Notification(alert.notification.title || 'Emergency Alert', {
                body: alert.notification.body || 'Check the dashboard immediately',
                icon: '/fire-icon.png',
                badge: '/badge-icon.png',
                requireInteraction: true,
                tag: alert.id
            });
        }
    };

    // Play alert sound
    const playAlertSound = () => {
        if (audioRef.current) {
            audioRef.current.play().catch(err => console.log('Audio play failed:', err));
        }
    };

    // Get segment status color
    const getStatusColor = (segment) => {
        if (!segment) return 'bg-gray-400';
        if (segment.isEmergency || segment.flameDetected) return 'bg-red-600';
        if (segment.isDangerous) return 'bg-orange-500';
        return 'bg-green-500';
    };

    // Get segment status text
    const getStatusText = (segment) => {
        if (!segment) return 'NO DATA';
        if (segment.flameDetected) return 'FIRE DETECTED';
        if (segment.isEmergency) return 'EMERGENCY';
        if (segment.isDangerous) return 'WARNING';
        return 'SAFE';
    };

    // Render segment card
    const SegmentCard = ({ name, segment, icon }) => {
        const status = getStatusText(segment);
        const colorClass = getStatusColor(segment);

        return (
            <div className={`rounded-xl shadow-lg overflow-hidden transition-all duration-300 hover:shadow-2xl ${segment?.isEmergency ? 'ring-4 ring-red-500 animate-pulse' : ''
                }`}>
                {/* Header */}
                <div className={`${colorClass} p-4 text-white`}>
                    <div className="flex items-center justify-between">
                        <div className="flex items-center space-x-3">
                            <span className="text-3xl">{icon}</span>
                            <div>
                                <h3 className="text-xl font-bold">{name}</h3>
                                <p className="text-sm opacity-90">{segment?.sensorTypes || 'No sensors'}</p>
                            </div>
                        </div>
                        <div className="text-right">
                            <div className="text-2xl font-bold">{status}</div>
                            {segment?.flameDetected && (
                                <div className="flex items-center justify-end space-x-1 mt-1">
                                    <span className="animate-pulse">🔥</span>
                                    <span className="text-sm font-semibold">FLAME</span>
                                </div>
                            )}
                        </div>
                    </div>
                </div>

                {/* Body */}
                <div className="bg-white p-4">
                    <div className="grid grid-cols-2 gap-3">
                        {segment?.temperature > 0 && (
                            <div className="bg-blue-50 rounded-lg p-3">
                                <div className="text-xs text-gray-600 mb-1">Temperature</div>
                                <div className="text-2xl font-bold text-blue-600">
                                    {segment.temperature.toFixed(1)}°C
                                </div>
                            </div>
                        )}

                        {segment?.humidity > 0 && (
                            <div className="bg-cyan-50 rounded-lg p-3">
                                <div className="text-xs text-gray-600 mb-1">Humidity</div>
                                <div className="text-2xl font-bold text-cyan-600">
                                    {segment.humidity.toFixed(0)}%
                                </div>
                            </div>
                        )}

                        {segment?.gasLevel > 0 && (
                            <div className={`rounded-lg p-3 ${segment.gasLevel > 500 ? 'bg-red-50' :
                                    segment.gasLevel > 300 ? 'bg-orange-50' : 'bg-green-50'
                                }`}>
                                <div className="text-xs text-gray-600 mb-1">Gas Level</div>
                                <div className={`text-2xl font-bold ${segment.gasLevel > 500 ? 'text-red-600' :
                                        segment.gasLevel > 300 ? 'text-orange-600' : 'text-green-600'
                                    }`}>
                                    {segment.gasLevel}
                                </div>
                            </div>
                        )}

                        {segment?.airQuality > 0 && (
                            <div className={`rounded-lg p-3 ${segment.airQuality > 600 ? 'bg-red-50' :
                                    segment.airQuality > 400 ? 'bg-orange-50' : 'bg-green-50'
                                }`}>
                                <div className="text-xs text-gray-600 mb-1">Air Quality</div>
                                <div className={`text-2xl font-bold ${segment.airQuality > 600 ? 'text-red-600' :
                                        segment.airQuality > 400 ? 'text-orange-600' : 'text-green-600'
                                    }`}>
                                    {segment.airQuality}
                                </div>
                            </div>
                        )}
                    </div>

                    {segment?.components && (
                        <div className="mt-3 text-xs text-gray-500">
                            Components: {segment.components}
                        </div>
                    )}

                    {segment?.lastUpdated && (
                        <div className="mt-2 text-xs text-gray-400">
                            Updated: {segment.lastUpdated}
                        </div>
                    )}
                </div>
            </div>
        );
    };

    if (loading) {
        return (
            <div className="flex items-center justify-center min-h-screen bg-gradient-to-br from-slate-900 via-blue-900 to-slate-900">
                <div className="text-center">
                    <div className="animate-spin rounded-full h-16 w-16 border-t-4 border-b-4 border-blue-500 mx-auto mb-4"></div>
                    <div className="text-xl font-semibold text-white">Loading Smart Building Data...</div>
                </div>
            </div>
        );
    }

    return (
        <div className="min-h-screen bg-gradient-to-br from-slate-900 via-blue-900 to-slate-900 p-4 md:p-8">
            {/* Hidden audio element for alert sound */}
            <audio ref={audioRef} src="/alert-sound.mp3" preload="auto" />

            <div className="max-w-7xl mx-auto">
                {/* Header */}
                <div className="bg-white/10 backdrop-blur-md rounded-2xl shadow-2xl p-6 mb-6 border border-white/20">
                    <div className="flex flex-col md:flex-row items-center justify-between">
                        <div>
                            <h1 className="text-3xl md:text-4xl font-bold text-white mb-2 flex items-center space-x-3">
                                <span>🏢</span>
                                <span>Smart Building Monitor</span>
                            </h1>
                            <p className="text-blue-200">{systemData?.info?.buildingName || 'KUET Smart Apartment Complex'}</p>
                        </div>

                        <div className="mt-4 md:mt-0 text-right">
                            {systemData?.systemEmergency ? (
                                <div className="bg-red-600 text-white px-6 py-3 rounded-lg animate-pulse">
                                    <div className="text-2xl font-bold">⚠️ SYSTEM EMERGENCY</div>
                                    <div className="text-sm">Duration: {Math.floor((systemData.emergencyDuration || 0) / 1000)}s</div>
                                </div>
                            ) : (
                                <div className="bg-green-600 text-white px-6 py-3 rounded-lg">
                                    <div className="text-2xl font-bold">✅ ALL SYSTEMS SAFE</div>
                                </div>
                            )}

                            {lastUpdate && (
                                <div className="text-xs text-blue-200 mt-2">
                                    Last Update: {lastUpdate.toLocaleTimeString()}
                                </div>
                            )}
                        </div>
                    </div>
                </div>

                {/* Global Environmental Data */}
                <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-6">
                    <div className="bg-white/10 backdrop-blur-md rounded-xl p-6 border border-white/20">
                        <div className="text-blue-200 text-sm mb-2">Global Temperature</div>
                        <div className="text-4xl font-bold text-white">
                            {systemData?.globalTemperature?.toFixed(1) || '--'}°C
                        </div>
                    </div>

                    <div className="bg-white/10 backdrop-blur-md rounded-xl p-6 border border-white/20">
                        <div className="text-blue-200 text-sm mb-2">Global Humidity</div>
                        <div className="text-4xl font-bold text-white">
                            {systemData?.globalHumidity?.toFixed(0) || '--'}%
                        </div>
                    </div>

                    <div className="bg-white/10 backdrop-blur-md rounded-xl p-6 border border-white/20">
                        <div className="text-blue-200 text-sm mb-2">System Status</div>
                        <div className="text-2xl font-bold text-white">
                            {systemData?.esp32Status || 'Offline'}
                        </div>
                        <div className="text-xs text-blue-200 mt-1">
                            Uptime: {Math.floor((systemData?.uptime || 0) / 60)} min
                        </div>
                    </div>
                </div>

                {/* Segment Cards */}
                <div className="grid grid-cols-1 md:grid-cols-2 gap-6 mb-6">
                    <SegmentCard
                        name="Kitchen"
                        segment={segments.kitchen}
                        icon="👨‍🍳"
                    />

                    <SegmentCard
                        name="Bedroom"
                        segment={segments.bedroom}
                        icon="🛏️"
                    />

                    <SegmentCard
                        name="Parking"
                        segment={segments.parking}
                        icon="🚗"
                    />

                    <SegmentCard
                        name="Central Gas Chamber"
                        segment={segments.centralGas}
                        icon="⚠️"
                    />
                </div>

                {/* Emergency Notifications */}
                {emergencyNotifications.length > 0 && (
                    <div className="bg-red-900/30 backdrop-blur-md border border-red-500/50 rounded-xl p-6 mb-6">
                        <h2 className="text-2xl font-bold text-white mb-4 flex items-center space-x-2">
                            <span>🚨</span>
                            <span>Emergency Notifications</span>
                        </h2>

                        <div className="space-y-3">
                            {emergencyNotifications.map(notification => (
                                <div key={notification.id} className="bg-white/10 rounded-lg p-4 border border-red-500/30">
                                    <div className="flex items-start justify-between">
                                        <div className="flex-1">
                                            <div className="text-white font-semibold">
                                                {notification.recipient}
                                            </div>
                                            <div className="text-red-200 text-sm mt-1">
                                                Location: {notification.details?.location} - {notification.details?.cause}
                                            </div>
                                            <div className="text-xs text-red-300 mt-1">
                                                {notification.timestamp}
                                            </div>
                                        </div>
                                        <div className="text-red-400 text-2xl">🚒</div>
                                    </div>
                                </div>
                            ))}
                        </div>
                    </div>
                )}

                {/* Alerts Timeline */}
                <div className="bg-white/10 backdrop-blur-md rounded-xl p-6 border border-white/20">
                    <h2 className="text-2xl font-bold text-white mb-4 flex items-center space-x-2">
                        <span>📋</span>
                        <span>Recent Alerts</span>
                    </h2>

                    {alerts.length > 0 ? (
                        <div className="space-y-2 max-h-96 overflow-y-auto">
                            {alerts.map(alert => (
                                <div
                                    key={alert.id}
                                    className={`rounded-lg p-4 border ${alert.type === 'EMERGENCY'
                                            ? 'bg-red-900/30 border-red-500/50'
                                            : alert.type === 'ALL_CLEAR'
                                                ? 'bg-green-900/30 border-green-500/50'
                                                : 'bg-orange-900/30 border-orange-500/50'
                                        }`}
                                >
                                    <div className="flex items-start justify-between">
                                        <div className="flex-1">
                                            <div className="flex items-center space-x-2">
                                                <span className={`px-3 py-1 rounded text-xs font-bold ${alert.type === 'EMERGENCY'
                                                        ? 'bg-red-600 text-white'
                                                        : alert.type === 'ALL_CLEAR'
                                                            ? 'bg-green-600 text-white'
                                                            : 'bg-orange-600 text-white'
                                                    }`}>
                                                    {alert.type}
                                                </span>
                                                {alert.segment && (
                                                    <span className="text-white font-semibold">
                                                        {alert.segment}
                                                    </span>
                                                )}
                                            </div>

                                            {alert.cause && (
                                                <div className="text-white text-sm mt-2">
                                                    Cause: {alert.cause}
                                                </div>
                                            )}

                                            {alert.message && (
                                                <div className="text-green-200 text-sm mt-2">
                                                    {alert.message}
                                                </div>
                                            )}

                                            {alert.timestamp && (
                                                <div className="text-xs text-gray-400 mt-2">
                                                    {alert.timestamp}
                                                </div>
                                            )}
                                        </div>

                                        <div className="text-2xl ml-4">
                                            {alert.type === 'EMERGENCY' ? '🔥' :
                                                alert.type === 'ALL_CLEAR' ? '✅' : '⚠️'}
                                        </div>
                                    </div>
                                </div>
                            ))}
                        </div>
                    ) : (
                        <div className="text-center text-gray-400 py-8">
                            No alerts at this time
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
}

export default SmartBuildingMonitor;
