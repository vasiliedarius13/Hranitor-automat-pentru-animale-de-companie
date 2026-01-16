import React, { useState, useEffect } from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import { ThemeProvider, createTheme } from '@mui/material/styles';
import CssBaseline from '@mui/material/CssBaseline';
import Box from '@mui/material/Box';
import Container from '@mui/material/Container';
import { mqttClient } from './services/mqtt';
import Header from './components/Header';
import Sidebar from './components/Sidebar';
import Dashboard from './pages/Dashboard';
import Devices from './pages/Devices';
import Schedule from './pages/Schedule';
import History from './pages/History';
import Settings from './pages/Settings';
import './App.css';

const theme = createTheme({
  palette: {
    mode: 'light',
    primary: {
      main: '#1976d2',
    },
    secondary: {
      main: '#dc004e',
    },
  },
});

function App() {
  const [devices, setDevices] = useState([]);
  const [currentDevice, setCurrentDevice] = useState(null);
  const [sidebarOpen, setSidebarOpen] = useState(true);
  const [notificationCount, setNotificationCount] = useState(0);

  useEffect(() => {
    // Conectare la broker MQTT
    mqttClient.connect();

    // Abonare la topicuri
    mqttClient.subscribe('petfeeder/+/status', (topic, message) => {
      const deviceId = topic.split('/')[1];
      const status = JSON.parse(message.toString());
      
      setDevices(prev => {
        const existing = prev.find(d => d.id === deviceId);
        if (existing) {
          return prev.map(d => 
            d.id === deviceId ? { ...d, status } : d
          );
        } else {
          return [...prev, { id: deviceId, name: `Device ${deviceId}`, status }];
        }
      });
    });

    mqttClient.subscribe('petfeeder/+/sensors', (topic, message) => {
      const deviceId = topic.split('/')[1];
      const sensors = JSON.parse(message.toString());
      
      setDevices(prev => prev.map(d => 
        d.id === deviceId ? { ...d, sensors } : d
      ));
    });

    return () => {
      mqttClient.disconnect();
    };
  }, []);

  const toggleSidebar = () => {
    setSidebarOpen(!sidebarOpen);
  };

  const sendCommand = (deviceId, command, params = {}) => {
    const payload = JSON.stringify({
      cmd: command,
      ...params,
      timestamp: Date.now()
    });
    
    mqttClient.publish(`petfeeder/${deviceId}/commands`, payload);
  };

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <Router>
        <Box sx={{ display: 'flex' }}>
          <Header 
            toggleSidebar={toggleSidebar} 
            notificationCount={notificationCount}
            currentDevice={currentDevice}
          />
          <Sidebar 
            open={sidebarOpen}
            devices={devices}
            currentDevice={currentDevice}
            onDeviceSelect={setCurrentDevice}
          />
          <Box
            component="main"
            sx={{
              flexGrow: 1,
              height: '100vh',
              overflow: 'auto',
              mt: 8,
              ml: sidebarOpen ? '240px' : 0,
              transition: 'margin-left 0.3s',
            }}
          >
            <Container maxWidth="xl" sx={{ mt: 4, mb: 4 }}>
              <Routes>
                <Route path="/" element={
                  <Dashboard 
                    devices={devices}
                    currentDevice={currentDevice}
                    onSendCommand={sendCommand}
                  />
                } />
                <Route path="/devices" element={
                  <Devices 
                    devices={devices}
                    onDeviceSelect={setCurrentDevice}
                  />
                } />
                <Route path="/schedule" element={
                  <Schedule 
                    currentDevice={currentDevice}
                    onSendCommand={sendCommand}
                  />
                } />
                <Route path="/history" element={
                  <History 
                    currentDevice={currentDevice}
                  />
                } />
                <Route path="/settings" element={
                  <Settings 
                    currentDevice={currentDevice}
                  />
                } />
              </Routes>
            </Container>
          </Box>
        </Box>
      </Router>
    </ThemeProvider>
  );
}

export default App;