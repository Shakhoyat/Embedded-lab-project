import { useEffect, useState } from 'react';
import { database } from '../firebase/config';
import { ref, onValue } from 'firebase/database';

/**
 * Example component demonstrating Firebase Realtime Database integration
 * for Smart Building Monitoring IoT system
 */
function SmartBuildingMonitor() {
    const [sensorData, setSensorData] = useState({});
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        // Example: Listen to sensor data from Firebase Realtime Database
        // Adjust the path according to your database structure
        const sensorRef = ref(database, 'sensors');

        const unsubscribe = onValue(sensorRef, (snapshot) => {
            const data = snapshot.val();
            setSensorData(data || {});
            setLoading(false);
        }, (error) => {
            console.error('Error fetching data:', error);
            setLoading(false);
        });

        // Cleanup subscription on unmount
        return () => unsubscribe();
    }, []);

    if (loading) {
        return (
            <div className="flex items-center justify-center min-h-screen bg-gray-100">
                <div className="text-xl font-semibold text-gray-700">Loading sensor data...</div>
            </div>
        );
    }

    return (
        <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100 p-8">
            <div className="max-w-7xl mx-auto">
                <h1 className="text-4xl font-bold text-gray-800 mb-8 text-center">
                    Smart Building Monitoring System
                </h1>

                <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
                    {/* Example sensor cards - customize based on your data structure */}
                    <div className="bg-white rounded-lg shadow-lg p-6 hover:shadow-xl transition-shadow">
                        <h2 className="text-xl font-semibold text-gray-700 mb-4">Temperature</h2>
                        <p className="text-3xl font-bold text-blue-600">
                            {sensorData.temperature || '--'}°C
                        </p>
                    </div>

                    <div className="bg-white rounded-lg shadow-lg p-6 hover:shadow-xl transition-shadow">
                        <h2 className="text-xl font-semibold text-gray-700 mb-4">Humidity</h2>
                        <p className="text-3xl font-bold text-green-600">
                            {sensorData.humidity || '--'}%
                        </p>
                    </div>

                    <div className="bg-white rounded-lg shadow-lg p-6 hover:shadow-xl transition-shadow">
                        <h2 className="text-xl font-semibold text-gray-700 mb-4">Air Quality</h2>
                        <p className="text-3xl font-bold text-purple-600">
                            {sensorData.airQuality || '--'} PPM
                        </p>
                    </div>

                    <div className="bg-white rounded-lg shadow-lg p-6 hover:shadow-xl transition-shadow">
                        <h2 className="text-xl font-semibold text-gray-700 mb-4">Gas Level (MQ2)</h2>
                        <p className="text-3xl font-bold text-orange-600">
                            {sensorData.gasLevel || '--'} PPM
                        </p>
                    </div>

                    <div className="bg-white rounded-lg shadow-lg p-6 hover:shadow-xl transition-shadow">
                        <h2 className="text-xl font-semibold text-gray-700 mb-4">Flame Detected</h2>
                        <p className="text-3xl font-bold text-red-600">
                            {sensorData.flameDetected ? 'YES' : 'NO'}
                        </p>
                    </div>

                    <div className="bg-white rounded-lg shadow-lg p-6 hover:shadow-xl transition-shadow">
                        <h2 className="text-xl font-semibold text-gray-700 mb-4">Status</h2>
                        <p className="text-lg font-semibold text-gray-600">
                            {Object.keys(sensorData).length > 0 ? 'Connected' : 'Waiting for data...'}
                        </p>
                    </div>
                </div>

                {/* Raw data display for debugging */}
                <div className="mt-8 bg-white rounded-lg shadow-lg p-6">
                    <h2 className="text-xl font-semibold text-gray-700 mb-4">Raw Sensor Data</h2>
                    <pre className="bg-gray-50 p-4 rounded overflow-auto text-sm">
                        {JSON.stringify(sensorData, null, 2)}
                    </pre>
                </div>
            </div>
        </div>
    );
}

export default SmartBuildingMonitor;
