import { useEffect, useState, useRef } from 'react';
import { database } from '../firebase/config';
import { ref, onValue } from 'firebase/database';

/**
 * Professional Smart Building Monitoring Dashboard
 * Enterprise-grade monitoring for 4 segments: Kitchen, Bedroom, Parking, Central Gas Chamber
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
    const [selectedSegment, setSelectedSegment] = useState(null);
    const audioRef = useRef(null);

    // Request notification permission
    useEffect(() => {
        if ('Notification' in window && Notification.permission === 'default') {
            Notification.requestPermission().then(setNotificationPermission);
        } else if ('Notification' in window) {
            setNotificationPermission(Notification.permission);
        }
    }, []);

    // Firebase listeners
    useEffect(() => {
        const systemRef = ref(database, 'smartBuilding/system');
        const segmentsRef = ref(database, 'smartBuilding/segments');
        const alertsRef = ref(database, 'smartBuilding/alerts');
        const emergencyRef = ref(database, 'smartBuilding/emergency_notifications');

        const unsubscribeSystem = onValue(systemRef, (snapshot) => {
            setSystemData(snapshot.val() || null);
            setLastUpdate(new Date());
            setConnectionStatus('connected');
            setLoading(false);
        }, () => {
            setConnectionStatus('error');
            setLoading(false);
        });

        const unsubscribeSegments = onValue(segmentsRef, (snapshot) => {
            setSegments(snapshot.val() || {});
        });

        const unsubscribeAlerts = onValue(alertsRef, (snapshot) => {
            const data = snapshot.val();
            if (data) {
                const alertsArray = Object.entries(data)
                    .map(([id, alert]) => ({ id, ...alert }))
                    .sort((a, b) => b.id - a.id)
                    .slice(0, 15);
                setAlerts(alertsArray);

                alertsArray.forEach(alert => {
                    if (alert.type === 'EMERGENCY' && !alert.acknowledged) {
                        showBrowserNotification(alert);
                        playAlertSound();
                    }
                });
            }
        });

        const unsubscribeEmergency = onValue(emergencyRef, (snapshot) => {
            const data = snapshot.val();
            if (data) {
                const emergencyArray = Object.entries(data)
                    .map(([id, notification]) => ({ id, ...notification }))
                    .sort((a, b) => b.id - a.id)
                    .slice(0, 5);
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

    const showBrowserNotification = (alert) => {
        if (notificationPermission === 'granted' && alert.notification) {
            new Notification(alert.notification.title || 'Emergency Alert', {
                body: alert.notification.body,
                icon: '/fire-icon.png',
                requireInteraction: true,
                tag: alert.id
            });
        }
    };

    const playAlertSound = () => {
        audioRef.current?.play().catch(() => { });
    };

    const getStatusColor = (segment) => {
        if (!segment) return { bg: 'from-gray-500 to-gray-600', text: 'text-gray-100', border: 'border-gray-400' };
        if (segment.isEmergency || segment.flameDetected) return { bg: 'from-red-600 to-red-700', text: 'text-white', border: 'border-red-400' };
        if (segment.isDangerous) return { bg: 'from-orange-500 to-orange-600', text: 'text-white', border: 'border-orange-400' };
        return { bg: 'from-emerald-500 to-emerald-600', text: 'text-white', border: 'border-emerald-400' };
    };

    const getStatusText = (segment) => {
        if (!segment) return 'NO DATA';
        if (segment.flameDetected) return '🔥 FIRE DETECTED';
        if (segment.isEmergency) return '⚠️ EMERGENCY';
        if (segment.isDangerous) return '⚡ WARNING';
        return '✓ SAFE';
    };

    const getMetricColor = (value, thresholds) => {
        if (value >= thresholds.critical) return 'text-red-600 bg-red-50 border-red-200';
        if (value >= thresholds.warning) return 'text-orange-600 bg-orange-50 border-orange-200';
        return 'text-emerald-600 bg-emerald-50 border-emerald-200';
    };

    const MetricCard = ({ label, value, unit, icon, thresholds }) => {
        const colorClass = thresholds ? getMetricColor(value, thresholds) : 'text-blue-600 bg-blue-50 border-blue-200';

        return (
            <div className={`${colorClass} border-2 rounded-lg p-4 transition-all duration-300 hover:scale-105 hover:shadow-lg`}>
                <div className="flex items-center justify-between mb-2">
                    <span className="text-sm font-medium opacity-80">{label}</span>
                    {icon && <span className="text-xl">{icon}</span>}
                </div>
                <div className="text-3xl font-bold tracking-tight">
                    {typeof value === 'number' ? value.toFixed(1) : value}
                    <span className="text-lg ml-1 font-normal">{unit}</span>
                </div>
            </div>
        );
    };

    const SegmentCard = ({ name, segment, icon, gradient }) => {
        const statusColors = getStatusColor(segment);
        const status = getStatusText(segment);
        const isEmergency = segment?.isEmergency || segment?.flameDetected;

        return (
            <div
                onClick={() => setSelectedSegment(selectedSegment === name ? null : name)}
                className={`bg-white rounded-2xl shadow-xl overflow-hidden cursor-pointer transition-all duration-500 hover:shadow-2xl transform hover:-translate-y-1 ${isEmergency ? 'ring-4 ring-red-500 animate-pulse' : ''
                    } ${selectedSegment === name ? 'ring-4 ring-blue-500' : ''}`}
            >
                {/* Header with gradient */}
                <div className={`bg-gradient-to-br ${gradient} p-6 relative overflow-hidden`}>
                    <div className="absolute top-0 right-0 opacity-10 text-9xl -mt-8 -mr-8">{icon}</div>
                    <div className="relative z-10">
                        <div className="flex items-center justify-between mb-3">
                            <div className="flex items-center space-x-3">
                                <span className="text-4xl drop-shadow-lg">{icon}</span>
                                <div>
                                    <h3 className="text-2xl font-bold text-white tracking-tight">{name}</h3>
                                    <p className="text-sm text-white/80 font-medium">{segment?.sensorTypes || 'No sensors'}</p>
                                </div>
                            </div>
                        </div>

                        <div className={`inline-flex items-center space-x-2 px-4 py-2 rounded-full ${statusColors.bg} ${statusColors.text} shadow-lg`}>
                            <div className={`w-3 h-3 rounded-full ${isEmergency ? 'bg-white animate-ping' : 'bg-white/50'}`}></div>
                            <span className="font-bold text-lg">{status}</span>
                        </div>
                    </div>
                </div>

                {/* Content */}
                <div className="p-6 bg-gradient-to-b from-gray-50 to-white">
                    <div className="grid grid-cols-2 gap-4">
                        {segment?.temperature > 0 && (
                            <MetricCard
                                label="Temperature"
                                value={segment.temperature}
                                unit="°C"
                                icon="🌡️"
                                thresholds={{ warning: 35, critical: 45 }}
                            />
                        )}

                        {segment?.humidity > 0 && (
                            <MetricCard
                                label="Humidity"
                                value={segment.humidity}
                                unit="%"
                                icon="💧"
                                thresholds={{ warning: 70, critical: 85 }}
                            />
                        )}

                        {segment?.gasLevel > 0 && (
                            <MetricCard
                                label="Gas Level"
                                value={segment.gasLevel}
                                unit="PPM"
                                icon="💨"
                                thresholds={{ warning: 300, critical: 500 }}
                            />
                        )}

                        {segment?.airQuality > 0 && (
                            <MetricCard
                                label="Air Quality"
                                value={segment.airQuality}
                                unit="PPM"
                                icon="🍃"
                                thresholds={{ warning: 400, critical: 600 }}
                            />
                        )}
                    </div>

                    {/* Additional Info */}
                    <div className="mt-4 pt-4 border-t border-gray-200 flex items-center justify-between text-xs">
                        <span className="text-gray-500 flex items-center space-x-1">
                            <span>⚙️</span>
                            <span>{segment?.components || 'N/A'}</span>
                        </span>
                        <span className="text-gray-400">
                            {segment?.lastUpdated ? new Date(segment.lastUpdated).toLocaleTimeString() : 'No update'}
                        </span>
                    </div>
                </div>
            </div>
        );
    };

    if (loading) {
        return (
            <div className="min-h-screen bg-gradient-to-br from-slate-900 via-blue-900 to-indigo-900 flex items-center justify-center">
                <div className="text-center">
                    <div className="relative">
                        <div className="w-20 h-20 border-4 border-blue-200 border-t-blue-500 rounded-full animate-spin mx-auto"></div>
                        <div className="w-20 h-20 border-4 border-blue-300 border-t-transparent rounded-full animate-spin mx-auto absolute top-0 left-1/2 -ml-10" style={{ animationDirection: 'reverse', animationDuration: '1.5s' }}></div>
                    </div>
                    <p className="mt-6 text-xl font-semibold text-white">Initializing Smart Building Monitor</p>
                    <p className="text-blue-200 mt-2">Connecting to Firebase...</p>
                </div>
            </div>
        );
    }

    return (
        <div className="min-h-screen bg-gradient-to-br from-slate-900 via-blue-900 to-indigo-900">
            <audio ref={audioRef} src="/alert-sound.mp3" preload="auto" />

            {/* Top Bar */}
            <div className="bg-white/10 backdrop-blur-xl border-b border-white/20 sticky top-0 z-50 shadow-2xl">
                <div className="max-w-7xl mx-auto px-6 py-4">
                    <div className="flex items-center justify-between">
                        <div className="flex items-center space-x-4">
                            <div className="w-12 h-12 bg-gradient-to-br from-blue-500 to-indigo-600 rounded-xl flex items-center justify-center shadow-lg">
                                <span className="text-2xl">🏢</span>
                            </div>
                            <div>
                                <h1 className="text-2xl font-bold text-white tracking-tight">Smart Building Monitor</h1>
                                <p className="text-sm text-blue-200">{systemData?.info?.buildingName || 'KUET Smart Apartment Complex'}</p>
                            </div>
                        </div>

                        <div className="flex items-center space-x-4">
                            {/* Connection Status */}
                            <div className={`flex items-center space-x-2 px-4 py-2 rounded-full ${connectionStatus === 'connected' ? 'bg-emerald-500/20 border border-emerald-400/50' :
                                    connectionStatus === 'error' ? 'bg-red-500/20 border border-red-400/50' :
                                        'bg-yellow-500/20 border border-yellow-400/50'
                                }`}>
                                <div className={`w-2 h-2 rounded-full ${connectionStatus === 'connected' ? 'bg-emerald-400 animate-pulse' :
                                        connectionStatus === 'error' ? 'bg-red-400' :
                                            'bg-yellow-400 animate-pulse'
                                    }`}></div>
                                <span className="text-white text-sm font-medium">
                                    {connectionStatus === 'connected' ? 'Live' : connectionStatus === 'error' ? 'Error' : 'Connecting'}
                                </span>
                            </div>

                            {/* System Status Badge */}
                            {systemData?.systemEmergency ? (
                                <div className="bg-red-600 px-6 py-2 rounded-full shadow-lg animate-pulse border-2 border-white/30">
                                    <div className="text-white font-bold text-sm flex items-center space-x-2">
                                        <span className="text-xl">⚠️</span>
                                        <span>EMERGENCY ACTIVE</span>
                                    </div>
                                </div>
                            ) : (
                                <div className="bg-emerald-600 px-6 py-2 rounded-full shadow-lg border-2 border-white/30">
                                    <div className="text-white font-bold text-sm flex items-center space-x-2">
                                        <span className="text-xl">✓</span>
                                        <span>ALL SYSTEMS OPERATIONAL</span>
                                    </div>
                                </div>
                            )}
                        </div>
                    </div>
                </div>
            </div>

            <div className="max-w-7xl mx-auto px-6 py-8">
                {/* System Overview Cards */}
                <div className="grid grid-cols-1 md:grid-cols-4 gap-6 mb-8">
                    <div className="bg-gradient-to-br from-blue-500 to-blue-600 rounded-xl p-6 shadow-xl text-white">
                        <div className="flex items-center justify-between mb-2">
                            <span className="text-sm font-semibold uppercase tracking-wide opacity-90">Temperature</span>
                            <span className="text-3xl">🌡️</span>
                        </div>
                        <div className="text-4xl font-bold">{systemData?.globalTemperature?.toFixed(1) || '--'}°C</div>
                        <div className="text-xs mt-2 opacity-75">Global Average</div>
                    </div>

                    <div className="bg-gradient-to-br from-cyan-500 to-cyan-600 rounded-xl p-6 shadow-xl text-white">
                        <div className="flex items-center justify-between mb-2">
                            <span className="text-sm font-semibold uppercase tracking-wide opacity-90">Humidity</span>
                            <span className="text-3xl">💧</span>
                        </div>
                        <div className="text-4xl font-bold">{systemData?.globalHumidity?.toFixed(0) || '--'}%</div>
                        <div className="text-xs mt-2 opacity-75">Kitchen Reference</div>
                    </div>

                    <div className="bg-gradient-to-br from-purple-500 to-purple-600 rounded-xl p-6 shadow-xl text-white">
                        <div className="flex items-center justify-between mb-2">
                            <span className="text-sm font-semibold uppercase tracking-wide opacity-90">Uptime</span>
                            <span className="text-3xl">⏱️</span>
                        </div>
                        <div className="text-4xl font-bold">{Math.floor((systemData?.uptime || 0) / 60)}</div>
                        <div className="text-xs mt-2 opacity-75">Minutes Online</div>
                    </div>

                    <div className="bg-gradient-to-br from-emerald-500 to-emerald-600 rounded-xl p-6 shadow-xl text-white">
                        <div className="flex items-center justify-between mb-2">
                            <span className="text-sm font-semibold uppercase tracking-wide opacity-90">Status</span>
                            <span className="text-3xl">📡</span>
                        </div>
                        <div className="text-2xl font-bold">{systemData?.esp32Status?.toUpperCase() || 'OFFLINE'}</div>
                        <div className="text-xs mt-2 opacity-75">
                            {lastUpdate ? lastUpdate.toLocaleTimeString() : 'No updates'}
                        </div>
                    </div>
                </div>

                {/* Segment Monitoring */}
                <div className="mb-8">
                    <h2 className="text-3xl font-bold text-white mb-6 flex items-center space-x-3">
                        <span className="w-10 h-10 bg-white/10 rounded-lg flex items-center justify-center">📊</span>
                        <span>Segment Monitoring</span>
                    </h2>

                    <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                        <SegmentCard name="Kitchen" segment={segments.kitchen} icon="👨‍🍳" gradient="from-orange-500 to-red-500" />
                        <SegmentCard name="Bedroom" segment={segments.bedroom} icon="🛏️" gradient="from-indigo-500 to-purple-500" />
                        <SegmentCard name="Parking" segment={segments.parking} icon="🚗" gradient="from-gray-600 to-gray-700" />
                        <SegmentCard name="Central Gas" segment={segments.centralGas} icon="⚠️" gradient="from-yellow-500 to-orange-500" />
                    </div>
                </div>

                {/* Emergency Notifications */}
                {emergencyNotifications.length > 0 && (
                    <div className="mb-8 bg-red-900/20 backdrop-blur-xl border-2 border-red-500/50 rounded-2xl p-6 shadow-2xl">
                        <h2 className="text-2xl font-bold text-white mb-4 flex items-center space-x-3">
                            <span className="w-10 h-10 bg-red-500 rounded-lg flex items-center justify-center animate-pulse">🚨</span>
                            <span>Emergency Dispatch Notifications</span>
                        </h2>

                        <div className="space-y-3">
                            {emergencyNotifications.map(notification => (
                                <div key={notification.id} className="bg-white/10 backdrop-blur-sm rounded-xl p-5 border border-red-500/30 hover:bg-white/20 transition-all">
                                    <div className="flex items-start justify-between">
                                        <div className="flex-1">
                                            <div className="flex items-center space-x-3 mb-2">
                                                <span className="text-2xl">🚒</span>
                                                <span className="text-white font-bold text-lg">{notification.recipient}</span>
                                            </div>
                                            <div className="text-red-200 text-sm space-y-1">
                                                <p><strong>Location:</strong> {notification.details?.location}</p>
                                                <p><strong>Cause:</strong> {notification.details?.cause}</p>
                                                <p className="text-xs text-red-300">{notification.timestamp}</p>
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            ))}
                        </div>
                    </div>
                )}

                {/* Alerts Timeline */}
                <div className="bg-white/10 backdrop-blur-xl rounded-2xl p-6 shadow-2xl border border-white/20">
                    <h2 className="text-2xl font-bold text-white mb-6 flex items-center space-x-3">
                        <span className="w-10 h-10 bg-white/10 rounded-lg flex items-center justify-center">📋</span>
                        <span>Activity Timeline</span>
                        <span className="ml-auto text-sm font-normal text-blue-200">Last {alerts.length} events</span>
                    </h2>

                    {alerts.length > 0 ? (
                        <div className="space-y-3 max-h-96 overflow-y-auto pr-2 custom-scrollbar">
                            {alerts.map((alert, index) => (
                                <div
                                    key={alert.id}
                                    className={`rounded-xl p-5 border-2 backdrop-blur-sm transition-all hover:scale-[1.02] ${alert.type === 'EMERGENCY'
                                            ? 'bg-red-900/30 border-red-500/50 hover:bg-red-900/40'
                                            : alert.type === 'ALL_CLEAR'
                                                ? 'bg-emerald-900/30 border-emerald-500/50 hover:bg-emerald-900/40'
                                                : 'bg-orange-900/30 border-orange-500/50 hover:bg-orange-900/40'
                                        }`}
                                    style={{ animationDelay: `${index * 50}ms` }}
                                >
                                    <div className="flex items-start justify-between">
                                        <div className="flex-1">
                                            <div className="flex items-center space-x-3 mb-2">
                                                <span className={`px-4 py-1.5 rounded-full text-xs font-bold shadow-lg ${alert.type === 'EMERGENCY'
                                                        ? 'bg-red-600 text-white'
                                                        : alert.type === 'ALL_CLEAR'
                                                            ? 'bg-emerald-600 text-white'
                                                            : 'bg-orange-600 text-white'
                                                    }`}>
                                                    {alert.type}
                                                </span>
                                                {alert.segment && (
                                                    <span className="text-white font-semibold text-lg">{alert.segment}</span>
                                                )}
                                            </div>

                                            {alert.cause && (
                                                <p className="text-white text-sm mt-2 font-medium">
                                                    <span className="opacity-70">Cause:</span> {alert.cause}
                                                </p>
                                            )}

                                            {alert.message && (
                                                <p className="text-emerald-200 text-sm mt-2">{alert.message}</p>
                                            )}

                                            {alert.timestamp && (
                                                <p className="text-xs text-gray-400 mt-3">{alert.timestamp}</p>
                                            )}
                                        </div>

                                        <div className="text-4xl ml-4">
                                            {alert.type === 'EMERGENCY' ? '🔥' :
                                                alert.type === 'ALL_CLEAR' ? '✅' : '⚠️'}
                                        </div>
                                    </div>
                                </div>
                            ))}
                        </div>
                    ) : (
                        <div className="text-center py-12">
                            <div className="text-6xl mb-4">✨</div>
                            <p className="text-white text-lg font-medium">No alerts recorded</p>
                            <p className="text-blue-200 text-sm mt-2">All systems operating normally</p>
                        </div>
                    )}
                </div>
            </div>

            {/* Footer */}
            <div className="bg-white/5 backdrop-blur-xl border-t border-white/10 mt-12 py-6">
                <div className="max-w-7xl mx-auto px-6 text-center">
                    <p className="text-blue-200 text-sm">
                        Smart Building Monitoring System © 2025 | Real-time IoT Dashboard
                    </p>
                    {lastUpdate && (
                        <p className="text-blue-300 text-xs mt-1">
                            Last synchronized: {lastUpdate.toLocaleString()}
                        </p>
                    )}
                </div>
            </div>

            {/* Custom Scrollbar Styles */}
            <style>{`
                .custom-scrollbar::-webkit-scrollbar {
                    width: 8px;
                }
                .custom-scrollbar::-webkit-scrollbar-track {
                    background: rgba(255, 255, 255, 0.1);
                    border-radius: 10px;
                }
                .custom-scrollbar::-webkit-scrollbar-thumb {
                    background: rgba(255, 255, 255, 0.3);
                    border-radius: 10px;
                }
                .custom-scrollbar::-webkit-scrollbar-thumb:hover {
                    background: rgba(255, 255, 255, 0.5);
                }
            `}</style>
        </div>
    );
}

export default SmartBuildingMonitor;
