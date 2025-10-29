import { useState, useEffect } from 'react';

/**
 * Emergency Overlay Modal for Critical Fire Alerts
 * Full-screen overlay that demands immediate attention
 */
function EmergencyOverlay({
    isVisible,
    emergencyData,
    onAcknowledge,
    onDismiss,
    onDispatchEmergencyServices
}) {
    const [countdown, setCountdown] = useState(30);
    const [isBlinking, setIsBlinking] = useState(true);

    useEffect(() => {
        if (!isVisible) return;

        // Countdown timer for auto-escalation
        const countdownInterval = setInterval(() => {
            setCountdown(prev => {
                if (prev <= 1) {
                    clearInterval(countdownInterval);
                    // Auto-escalate when countdown reaches 0
                    onDispatchEmergencyServices?.();
                    return 0;
                }
                return prev - 1;
            });
        }, 1000);

        // Blinking effect
        const blinkInterval = setInterval(() => {
            setIsBlinking(prev => !prev);
        }, 500);

        return () => {
            clearInterval(countdownInterval);
            clearInterval(blinkInterval);
        };
    }, [isVisible, onDispatchEmergencyServices]);

    useEffect(() => {
        if (isVisible) {
            setCountdown(30);
            setIsBlinking(true);
        }
    }, [isVisible]);

    if (!isVisible || !emergencyData) return null;

    const formatTime = (seconds) => {
        const mins = Math.floor(seconds / 60);
        const secs = seconds % 60;
        return `${mins}:${secs.toString().padStart(2, '0')}`;
    };

    return (
        <div className="fixed inset-0 z-[9999] flex items-center justify-center">
            {/* Animated background */}
            <div
                className={`absolute inset-0 bg-red-900 transition-opacity duration-500 ${isBlinking ? 'opacity-90' : 'opacity-70'
                    }`}
                style={{
                    backgroundImage: `
                        radial-gradient(circle at 25% 25%, rgba(255, 0, 0, 0.8) 0%, transparent 50%),
                        radial-gradient(circle at 75% 75%, rgba(255, 100, 0, 0.8) 0%, transparent 50%),
                        radial-gradient(circle at 50% 50%, rgba(255, 0, 0, 0.6) 0%, transparent 70%)
                    `,
                    animation: 'pulse 2s infinite'
                }}
            />

            {/* Emergency siren pattern overlay */}
            <div
                className="absolute inset-0 opacity-20"
                style={{
                    background: `repeating-linear-gradient(
                        45deg,
                        transparent,
                        transparent 10px,
                        rgba(255, 255, 0, 0.3) 10px,
                        rgba(255, 255, 0, 0.3) 20px
                    )`,
                    animation: 'slide 1s linear infinite'
                }}
            />

            {/* Main alert modal */}
            <div className="relative bg-gray-900 border-4 border-red-500 rounded-2xl p-8 max-w-2xl mx-4 shadow-2xl">
                {/* Pulsing border effect */}
                <div
                    className="absolute -inset-1 bg-gradient-to-r from-red-500 via-yellow-500 to-red-500 rounded-2xl opacity-60 animate-pulse"
                    style={{ zIndex: -1 }}
                />

                {/* Header */}
                <div className="text-center mb-6">
                    <div className="flex justify-center items-center space-x-4 mb-4">
                        <span className="text-6xl animate-bounce">🔥</span>
                        <div>
                            <h1 className="text-4xl font-bold text-red-400 tracking-wide">
                                FIRE EMERGENCY
                            </h1>
                            <p className="text-red-300 text-lg">IMMEDIATE ACTION REQUIRED</p>
                        </div>
                        <span className="text-6xl animate-bounce" style={{ animationDelay: '0.5s' }}>🚨</span>
                    </div>

                    {/* Countdown timer */}
                    <div className="bg-red-800 border-2 border-red-400 rounded-lg p-4 mb-6">
                        <div className="text-white text-sm mb-2">AUTO-DISPATCH IN:</div>
                        <div className="text-4xl font-mono font-bold text-yellow-300">
                            {formatTime(countdown)}
                        </div>
                        <div className="text-red-200 text-xs mt-2">
                            Emergency services will be automatically contacted
                        </div>
                    </div>
                </div>

                {/* Emergency details */}
                <div className="bg-black/50 rounded-lg p-6 mb-6 border border-red-500/50">
                    <div className="grid grid-cols-1 md:grid-cols-2 gap-4 text-white">
                        <div>
                            <h3 className="text-red-400 font-bold text-lg mb-3">🏢 Location Details</h3>
                            <div className="space-y-2 text-sm">
                                <p><span className="text-red-300">Building:</span> KUET Smart Apartment Complex</p>
                                <p><span className="text-red-300">Segment:</span> {emergencyData.segment}</p>
                                <p><span className="text-red-300">Floor/Area:</span> {getLocationDetails(emergencyData.segment)}</p>
                                <p><span className="text-red-300">Fire Cause:</span> {emergencyData.cause}</p>
                            </div>
                        </div>

                        <div>
                            <h3 className="text-red-400 font-bold text-lg mb-3">⏰ Emergency Timeline</h3>
                            <div className="space-y-2 text-sm">
                                <p><span className="text-red-300">Detected:</span> {new Date(emergencyData.timestamp).toLocaleTimeString()}</p>
                                <p><span className="text-red-300">Alert Level:</span> {emergencyData.severity}</p>
                                <p><span className="text-red-300">Status:</span> {emergencyData.status}</p>
                                {emergencyData.escalated && (
                                    <p className="text-yellow-300 font-bold">⚠️ ESCALATED ALERT</p>
                                )}
                            </div>
                        </div>
                    </div>
                </div>

                {/* Critical Actions */}
                <div className="space-y-4 mb-6">
                    <h3 className="text-yellow-400 font-bold text-xl text-center">CRITICAL ACTIONS REQUIRED</h3>

                    <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                        {/* Immediate Response */}
                        <button
                            onClick={onDispatchEmergencyServices}
                            className="bg-red-600 hover:bg-red-700 text-white p-4 rounded-lg font-bold text-lg transition-all transform hover:scale-105 border-2 border-red-400"
                        >
                            🚒 DISPATCH FIRE DEPARTMENT
                            <div className="text-sm font-normal mt-1">Call Emergency Services Now</div>
                        </button>

                        {/* Building Evacuation */}
                        <button
                            onClick={() => {
                                // Trigger building evacuation alert
                                alert('🚨 Building evacuation alert activated!\n\nAll residents will be notified immediately.');
                                // In a real system, this would trigger the building's PA system
                            }}
                            className="bg-orange-600 hover:bg-orange-700 text-white p-4 rounded-lg font-bold text-lg transition-all transform hover:scale-105 border-2 border-orange-400"
                        >
                            📢 BUILDING EVACUATION
                            <div className="text-sm font-normal mt-1">Alert All Residents</div>
                        </button>
                    </div>
                </div>

                {/* Secondary Actions */}
                <div className="grid grid-cols-2 md:grid-cols-4 gap-3 mb-6">
                    <button
                        onClick={() => {
                            alert('🚨 Security team alerted!\n\nBuilding security will respond immediately.');
                        }}
                        className="bg-blue-600 hover:bg-blue-700 text-white p-3 rounded text-sm font-medium transition-all"
                    >
                        🛡️ Alert Security
                    </button>

                    <button
                        onClick={() => {
                            alert('⛽ Gas supply shutoff activated!\n\nMain gas valves will be closed as a safety precaution.');
                        }}
                        className="bg-purple-600 hover:bg-purple-700 text-white p-3 rounded text-sm font-medium transition-all"
                    >
                        ⛽ Shut Off Gas
                    </button>

                    <button
                        onClick={() => {
                            alert('🔌 Emergency power protocols activated!\n\nNon-essential power will be cut to affected areas.');
                        }}
                        className="bg-yellow-600 hover:bg-yellow-700 text-white p-3 rounded text-sm font-medium transition-all"
                    >
                        🔌 Power Control
                    </button>

                    <button
                        onClick={() => {
                            alert('📞 Management team notified!\n\nAll emergency contacts have been alerted.');
                        }}
                        className="bg-green-600 hover:bg-green-700 text-white p-3 rounded text-sm font-medium transition-all"
                    >
                        📞 Notify Management
                    </button>
                </div>

                {/* Acknowledgment and Dismiss */}
                <div className="flex justify-center space-x-4">
                    <button
                        onClick={onAcknowledge}
                        className="bg-green-600 hover:bg-green-700 text-white px-8 py-3 rounded-lg font-bold transition-all transform hover:scale-105"
                    >
                        ✓ ACKNOWLEDGE EMERGENCY
                    </button>

                    <button
                        onClick={onDismiss}
                        className="bg-gray-600 hover:bg-gray-700 text-white px-6 py-3 rounded-lg font-medium transition-all"
                    >
                        False Alarm
                    </button>
                </div>

                {/* Emergency contacts footer */}
                <div className="mt-6 pt-4 border-t border-red-500/50">
                    <div className="text-center text-red-200 text-sm">
                        <p className="font-bold">Emergency Contacts:</p>
                        <p>Fire Department: 01303488507 | Admin: +8801XXXXXXXXX</p>
                        <p>Building Address: Fulbarigate, Khulna, KUET Area</p>
                    </div>
                </div>
            </div>

            {/* CSS Animations */}
            <style jsx>{`
                @keyframes slide {
                    0% { transform: translateX(-20px); }
                    100% { transform: translateX(20px); }
                }
                
                @keyframes pulse {
                    0%, 100% { opacity: 0.7; }
                    50% { opacity: 0.9; }
                }
            `}</style>
        </div>
    );
}

// Helper function to get location details based on segment
function getLocationDetails(segment) {
    const locationMap = {
        'Kitchen': 'Ground Floor - Cooking Area',
        'Bedroom': '1st Floor - Sleeping Quarters',
        'Parking': 'Ground Floor - Vehicle Storage',
        'Central_Gas': 'Basement - Gas Distribution Hub',
        'Central Gas': 'Basement - Gas Distribution Hub'
    };

    return locationMap[segment] || 'Location Details Unavailable';
}

export default EmergencyOverlay;