// Import the functions you need from the SDKs you need
import { initializeApp } from "firebase/app";
import { getAnalytics } from "firebase/analytics";
import { getDatabase } from "firebase/database";

// Your web app's Firebase configuration
// For Firebase JS SDK v7.20.0 and later, measurementId is optional
const firebaseConfig = {
  apiKey: "AIzaSyDC9lCy5fUDA_zuUAnxVy0pZSqI3F5NuDM",
  authDomain: "smart-building-monitoring-iot.firebaseapp.com",
  databaseURL:
    "https://smart-building-monitoring-iot-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "smart-building-monitoring-iot",
  storageBucket: "smart-building-monitoring-iot.firebasestorage.app",
  messagingSenderId: "687656787065",
  appId: "1:687656787065:web:7ab3037187bff712fc88a8",
  measurementId: "G-66Z66LLE08",
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const analytics = getAnalytics(app);
const database = getDatabase(app);

export { app, analytics, database };
