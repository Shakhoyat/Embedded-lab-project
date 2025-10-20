# Smart Building Monitoring - Frontend

A React-based frontend application for monitoring IoT sensors in a smart building system. This application connects to Firebase Realtime Database to display real-time sensor data from Arduino Mega and ESP32 devices.

## Features

- 🔥 **Firebase Integration**: Real-time data synchronization with Firebase Realtime Database
- 🎨 **Tailwind CSS 3**: Modern, responsive UI design
- ⚡ **Vite**: Fast development and build tool
- 📊 **Real-time Monitoring**: Live updates from multiple sensors:
  - DHT11 (Temperature & Humidity)
  - DS18B20 (Temperature)
  - MQ2 (Gas Sensor)
  - MQ135 (Air Quality)
  - Flame Sensor

## Tech Stack

- React 19.1.1
- Vite 7.1.7
- Tailwind CSS 3.4.18
- Firebase 11.x
- PostCSS & Autoprefixer

## Prerequisites

- Node.js (v16 or higher)
- npm or yarn

## Installation

1. Install dependencies:
```bash
npm install
```

## Development

Start the development server:
```bash
npm run dev
```

The application will be available at `http://localhost:5173`

## Build

Build for production:
```bash
npm run build
```

Preview production build:
```bash
npm preview
```

## Firebase Configuration

The Firebase configuration is located in `src/firebase/config.js`. The current setup includes:

- **Project**: smart-building-monitoring-iot
- **Database**: Realtime Database (Asia Southeast 1)
- **Analytics**: Enabled

### Firebase Database Structure (Example)

```json
{
  "sensors": {
    "room1": {
      "temperature": 25.5,
      "humidity": 60,
      "gasLevel": 120,
      "airQuality": 45,
      "flameDetected": false
    },
    "room2": { ... },
    "room3": { ... }
  }
}
```

## Project Structure

```
frontend/
├── src/
│   ├── components/
│   │   └── SmartBuildingMonitor.jsx  # Main monitoring component
│   ├── firebase/
│   │   └── config.js                  # Firebase configuration
│   ├── App.jsx                        # Root component
│   ├── App.css                        # App styles
│   ├── index.css                      # Global styles + Tailwind
│   └── main.jsx                       # Entry point
├── public/                            # Static assets
├── index.html                         # HTML template
├── package.json                       # Dependencies
├── vite.config.js                     # Vite configuration
├── tailwind.config.js                 # Tailwind configuration
└── postcss.config.js                  # PostCSS configuration
```

## Customization

### Updating Sensor Data Display

Edit `src/components/SmartBuildingMonitor.jsx` to match your Firebase database structure:

```javascript
const sensorRef = ref(database, 'your/database/path');
```

### Styling

- Tailwind classes can be used throughout the components
- Global styles in `src/index.css`
- Component-specific styles in `src/App.css`

### Adding New Components

Create new components in `src/components/` and import them in your pages.

## Troubleshooting

### Tailwind styles not applying
Make sure your `tailwind.config.js` includes all content paths:
```javascript
content: [
  "./index.html",
  "./src/**/*.{js,ts,jsx,tsx}",
]
```

### Firebase connection issues
- Check your Firebase credentials in `src/firebase/config.js`
- Verify database rules in Firebase Console
- Ensure your database URL matches your Firebase region

## License

MIT


## React Compiler

The React Compiler is not enabled on this template because of its impact on dev & build performances. To add it, see [this documentation](https://react.dev/learn/react-compiler/installation).

## Expanding the ESLint configuration

If you are developing a production application, we recommend using TypeScript with type-aware lint rules enabled. Check out the [TS template](https://github.com/vitejs/vite/tree/main/packages/create-vite/template-react-ts) for information on how to integrate TypeScript and [`typescript-eslint`](https://typescript-eslint.io) in your project.
