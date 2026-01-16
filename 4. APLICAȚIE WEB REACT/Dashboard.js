import React, { useState, useEffect } from 'react';
import { Grid, Paper, Typography, Box, Button, Card, CardContent } from '@mui/material';
import { 
  LineChart, Line, BarChart, Bar, PieChart, Pie, Cell,
  XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer 
} from 'recharts';
import FeedIcon from '@mui/icons-material/Feed';
import DeviceThermostatIcon from '@mui/icons-material/DeviceThermostat';
import BatteryChargingFullIcon from '@mui/icons-material/BatteryChargingFull';
import SignalWifi4BarIcon from '@mui/icons-material/SignalWifi4Bar';
import CircularProgress from '@mui/material/CircularProgress';

const Dashboard = ({ devices, currentDevice, onSendCommand }) => {
  const [selectedDevice, setSelectedDevice] = useState(currentDevice || (devices.length > 0 ? devices[0].id : null));
  const [sensorHistory, setSensorHistory] = useState([]);
  const [feedingHistory, setFeedingHistory] = useState([]);

  const device = devices.find(d => d.id === selectedDevice);

  useEffect(() => {
    if (device) {
      // Simulare date istorice (în realitate ar veni de la API)
      const mockSensorData = Array.from({ length: 24 }, (_, i) => ({
        time: `${i}:00`,
        temperature: 20 + Math.sin(i) * 5,
        foodLevel: 30 - (i * 0.5),
        humidity: 40 + Math.cos(i) * 10
      }));
      
      const mockFeedingData = [
        { time: '08:00', amount: 25, type: 'Scheduled' },
        { time: '14:00', amount: 15, type: 'Manual' },
        { time: '20:00', amount: 25, type: 'Scheduled' }
      ];
      
      setSensorHistory(mockSensorData);
      setFeedingHistory(mockFeedingData);
    }
  }, [device]);

  const handleFeedNow = () => {
    if (selectedDevice) {
      onSendCommand(selectedDevice, 'feed', { amount: 25 });
    }
  };

  if (!device) {
    return (
      <Box sx={{ display: 'flex', justifyContent: 'center', alignItems: 'center', height: '50vh' }}>
        <CircularProgress />
        <Typography variant="h6" sx={{ ml: 2 }}>
          No devices connected. Please set up a device.
        </Typography>
      </Box>
    );
  }

  const statusColors = {
    IDLE: '#4caf50',
    FEEDING: '#ff9800',
    ERROR: '#f44336',
    CONNECTING: '#2196f3'
  };

  const statusText = {
    0: 'Idle',
    1: 'Feeding',
    2: 'Error',
    3: 'Connecting'
  };

  return (
    <Box>
      <Typography variant="h4" gutterBottom>
        Dashboard - {device.name}
      </Typography>
      
      {/* Status Cards */}
      <Grid container spacing={3} sx={{ mb: 3 }}>
        <Grid item xs={12} sm={6} md={3}>
          <Paper sx={{ p: 2, display: 'flex', alignItems: 'center' }}>
            <DeviceThermostatIcon sx={{ mr: 2, color: '#f44336' }} />
            <Box>
              <Typography variant="h6">
                {device.sensors?.temp?.toFixed(1) || '--'}°C
              </Typography>
              <Typography variant="body2" color="textSecondary">
                Temperature
              </Typography>
            </Box>
          </Paper>
        </Grid>
        
        <Grid item xs={12} sm={6} md={3}>
          <Paper sx={{ p: 2, display: 'flex', alignItems: 'center' }}>
            <FeedIcon sx={{ mr: 2, color: '#4caf50' }} />
            <Box>
              <Typography variant="h6">
                {device.sensors?.food?.toFixed(1) || '--'} cm
              </Typography>
              <Typography variant="body2" color="textSecondary">
                Food Level
              </Typography>
            </Box>
          </Paper>
        </Grid>
        
        <Grid item xs={12} sm={6} md={3}>
          <Paper sx={{ p: 2, display: 'flex', alignItems: 'center' }}>
            <BatteryChargingFullIcon sx={{ mr: 2, color: '#ff9800' }} />
            <Box>
              <Typography variant="h6">
                {device.status?.battery || '--'}%
              </Typography>
              <Typography variant="body2" color="textSecondary">
                Battery
              </Typography>
            </Box>
          </Paper>
        </Grid>
        
        <Grid item xs={12} sm={6} md={3}>
          <Paper sx={{ p: 2, display: 'flex', alignItems: 'center' }}>
            <Box sx={{ 
              width: 12, 
              height: 12, 
              borderRadius: '50%', 
              bgcolor: statusColors[statusText[device.status?.state] || 'IDLE'],
              mr: 2 
            }} />
            <Box>
              <Typography variant="h6">
                {statusText[device.status?.state] || 'Unknown'}
              </Typography>
              <Typography variant="body2" color="textSecondary">
                Status
              </Typography>
            </Box>
          </Paper>
        </Grid>
      </Grid>
      
      {/* Charts and Control */}
      <Grid container spacing={3}>
        <Grid item xs={12} md={8}>
          <Paper sx={{ p: 2 }}>
            <Typography variant="h6" gutterBottom>
              Sensor History (Last 24 Hours)
            </Typography>
            <ResponsiveContainer width="100%" height={300}>
              <LineChart data={sensorHistory}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis dataKey="time" />
                <YAxis />
                <Tooltip />
                <Legend />
                <Line type="monotone" dataKey="temperature" stroke="#f44336" name="Temperature (°C)" />
                <Line type="monotone" dataKey="foodLevel" stroke="#4caf50" name="Food Level (cm)" />
                <Line type="monotone" dataKey="humidity" stroke="#2196f3" name="Humidity (%)" />
              </LineChart>
            </ResponsiveContainer>
          </Paper>
        </Grid>
        
        <Grid item xs={12} md={4}>
          <Card sx={{ height: '100%' }}>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Quick Control
              </Typography>
              <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
                <Button 
                  variant="contained" 
                  color="primary" 
                  fullWidth
                  onClick={handleFeedNow}
                  disabled={device.status?.state === 1} // Disable during feeding
                >
                  Feed Now (25g)
                </Button>
                
                <Button variant="outlined" fullWidth>
                  Check Status
                </Button>
                
                <Button variant="outlined" fullWidth>
                  View Logs
                </Button>
              </Box>
              
              <Typography variant="h6" sx={{ mt: 4, mb: 2 }}>
                Connection Status
              </Typography>
              <Grid container spacing={2}>
                <Grid item xs={6}>
                  <Paper sx={{ p: 1, textAlign: 'center' }}>
                    <SignalWifi4BarIcon 
                      sx={{ color: device.status?.wifi ? '#4caf50' : '#f44336' }} 
                    />
                    <Typography variant="body2">
                      WiFi: {device.status?.wifi ? 'Connected' : 'Disconnected'}
                    </Typography>
                  </Paper>
                </Grid>
                <Grid item xs={6}>
                  <Paper sx={{ p: 1, textAlign: 'center' }}>
                    <Box sx={{ 
                      width: 12, 
                      height: 12, 
                      borderRadius: '50%', 
                      bgcolor: device.status?.mqtt ? '#4caf50' : '#f44336',
                      mx: 'auto',
                      mb: 1
                    }} />
                    <Typography variant="body2">
                      MQTT: {device.status?.mqtt ? 'Connected' : 'Disconnected'}
                    </Typography>
                  </Paper>
                </Grid>
              </Grid>
            </CardContent>
          </Card>
        </Grid>
        
        {/* Feeding History */}
        <Grid item xs={12}>
          <Paper sx={{ p: 2 }}>
            <Typography variant="h6" gutterBottom>
              Today's Feeding History
            </Typography>
            <ResponsiveContainer width="100%" height={200}>
              <BarChart data={feedingHistory}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis dataKey="time" />
                <YAxis />
                <Tooltip />
                <Legend />
                <Bar dataKey="amount" fill="#8884d8" name="Food Amount (g)" />
              </BarChart>
            </ResponsiveContainer>
          </Paper>
        </Grid>
      </Grid>
    </Box>
  );
};

export default Dashboard;