-- Database schema for Pet Feeder IoT System

-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Table: devices
CREATE TABLE devices (
    device_id VARCHAR(32) PRIMARY KEY,
    device_name VARCHAR(100) NOT NULL DEFAULT 'Pet Feeder',
    mac_address CHAR(17) UNIQUE,
    firmware_version VARCHAR(20) DEFAULT '1.0.0',
    hardware_version VARCHAR(20) DEFAULT '1.0',
    location VARCHAR(200),
    timezone VARCHAR(50) DEFAULT 'Europe/Bucharest',
    registered_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    last_seen TIMESTAMP WITH TIME ZONE,
    is_active BOOLEAN DEFAULT TRUE,
    notes TEXT
);

-- Table: device_status (time-series data)
CREATE TABLE device_status (
    status_id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    device_id VARCHAR(32) REFERENCES devices(device_id) ON DELETE CASCADE,
    state_code SMALLINT NOT NULL,
    state_description VARCHAR(50),
    battery_level SMALLINT CHECK (battery_level >= 0 AND battery_level <= 100),
    wifi_signal_strength SMALLINT,
    wifi_connected BOOLEAN DEFAULT FALSE,
    mqtt_connected BOOLEAN DEFAULT FALSE,
    free_heap_memory INTEGER,
    uptime_seconds BIGINT,
    recorded_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_device FOREIGN KEY (device_id) REFERENCES devices(device_id)
);

-- Create hypertable for time-series data (if using TimescaleDB)
-- SELECT create_hypertable('device_status', 'recorded_at');

-- Table: sensor_data (time-series data)
CREATE TABLE sensor_data (
    data_id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    device_id VARCHAR(32) REFERENCES devices(device_id) ON DELETE CASCADE,
    temperature_celsius DECIMAL(4,2),
    humidity_percent DECIMAL(4,2),
    food_level_cm DECIMAL(5,2) CHECK (food_level_cm >= 0 AND food_level_cm <= 50),
    ambient_light_lux INTEGER,
    sound_level_db SMALLINT,
    recorded_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_device FOREIGN KEY (device_id) REFERENCES devices(device_id)
);

-- Table: feeding_schedule
CREATE TABLE feeding_schedule (
    schedule_id SERIAL PRIMARY KEY,
    device_id VARCHAR(32) REFERENCES devices(device_id) ON DELETE CASCADE,
    schedule_name VARCHAR(100) DEFAULT 'Default Schedule',
    hour SMALLINT NOT NULL CHECK (hour >= 0 AND hour <= 23),
    minute SMALLINT NOT NULL CHECK (minute >= 0 AND minute <= 59),
    food_amount_grams DECIMAL(5,2) NOT NULL CHECK (food_amount_grams > 0),
    days_of_week CHAR(7) DEFAULT '1111111', -- Monday to Sunday (1=enabled, 0=disabled)
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(device_id, hour, minute)
);

-- Table: feeding_events
CREATE TABLE feeding_events (
    event_id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    device_id VARCHAR(32) REFERENCES devices(device_id) ON DELETE CASCADE,
    schedule_id INTEGER REFERENCES feeding_schedule(schedule_id) ON DELETE SET NULL,
    trigger_type VARCHAR(20) NOT NULL CHECK (trigger_type IN ('SCHEDULED', 'MANUAL', 'REMOTE', 'BUTTON')),
    food_amount_grams DECIMAL(5,2) NOT NULL,
    expected_amount_grams DECIMAL(5,2),
    success BOOLEAN DEFAULT TRUE,
    error_message TEXT,
    start_time TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    end_time TIMESTAMP WITH TIME ZONE,
    duration_ms INTEGER,
    food_level_before DECIMAL(5,2),
    food_level_after DECIMAL(5,2)
);

-- Table: system_logs
CREATE TABLE system_logs (
    log_id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    device_id VARCHAR(32) REFERENCES devices(device_id) ON DELETE CASCADE,
    log_level VARCHAR(10) CHECK (log_level IN ('DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL')),
    log_source VARCHAR(50),
    log_message TEXT NOT NULL,
    additional_data JSONB,
    recorded_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Table: commands_history
CREATE TABLE commands_history (
    command_id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    device_id VARCHAR(32) REFERENCES devices(device_id) ON DELETE CASCADE,
    command_type VARCHAR(50) NOT NULL,
    command_payload JSONB NOT NULL,
    source VARCHAR(50) CHECK (source IN ('WEB', 'MOBILE', 'API', 'LOCAL')),
    executed_by VARCHAR(100),
    execution_result VARCHAR(20) CHECK (execution_result IN ('SUCCESS', 'FAILED', 'PENDING', 'TIMEOUT')),
    result_message TEXT,
    sent_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    executed_at TIMESTAMP WITH TIME ZONE,
    response_received_at TIMESTAMP WITH TIME ZONE
);

-- Table: users
CREATE TABLE users (
    user_id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash CHAR(60) NOT NULL,
    full_name VARCHAR(100),
    phone_number VARCHAR(20),
    profile_picture_url TEXT,
    is_active BOOLEAN DEFAULT TRUE,
    is_admin BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    last_login_at TIMESTAMP WITH TIME ZONE
);

-- Table: user_devices (many-to-many relationship)
CREATE TABLE user_devices (
    user_id UUID REFERENCES users(user_id) ON DELETE CASCADE,
    device_id VARCHAR(32) REFERENCES devices(device_id) ON DELETE CASCADE,
    access_level VARCHAR(20) DEFAULT 'VIEWER' CHECK (access_level IN ('OWNER', 'ADMIN', 'EDITOR', 'VIEWER')),
    added_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, device_id)
);

-- Table: notifications
CREATE TABLE notifications (
    notification_id UUID DEFAULT uuid_generate_v4() PRIMARY KEY,
    user_id UUID REFERENCES users(user_id) ON DELETE CASCADE,
    device_id VARCHAR(32) REFERENCES devices(device_id) ON DELETE CASCADE,
    notification_type VARCHAR(50) NOT NULL,
    title VARCHAR(200) NOT NULL,
    message TEXT NOT NULL,
    priority VARCHAR(10) DEFAULT 'MEDIUM' CHECK (priority IN ('LOW', 'MEDIUM', 'HIGH', 'CRITICAL')),
    is_read BOOLEAN DEFAULT FALSE,
    sent_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    read_at TIMESTAMP WITH TIME ZONE
);

-- Table: notification_rules
CREATE TABLE notification_rules (
    rule_id SERIAL PRIMARY KEY,
    user_id UUID REFERENCES users(user_id) ON DELETE CASCADE,
    device_id VARCHAR(32) REFERENCES devices(device_id) ON DELETE CASCADE,
    rule_type VARCHAR(50) NOT NULL,
    condition JSONB NOT NULL,
    action JSONB NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Indexes for performance
CREATE INDEX idx_device_status_device_time ON device_status(device_id, recorded_at DESC);
CREATE INDEX idx_sensor_data_device_time ON sensor_data(device_id, recorded_at DESC);
CREATE INDEX idx_feeding_events_device_time ON feeding_events(device_id, start_time DESC);
CREATE INDEX idx_system_logs_device_time ON system_logs(device_id, recorded_at DESC);
CREATE INDEX idx_commands_history_device_time ON commands_history(device_id, sent_at DESC);
CREATE INDEX idx_notifications_user_unread ON notifications(user_id, is_read) WHERE is_read = FALSE;

-- Views for common queries
CREATE VIEW device_current_status AS
SELECT 
    d.device_id,
    d.device_name,
    d.last_seen,
    ds.state_code,
    ds.state_description,
    ds.battery_level,
    ds.wifi_connected,
    ds.mqtt_connected,
    ds.recorded_at as last_status_update,
    EXTRACT(EPOCH FROM (NOW() - ds.recorded_at)) as seconds_since_update
FROM devices d
LEFT JOIN device_status ds ON d.device_id = ds.device_id
WHERE ds.recorded_at = (
    SELECT MAX(recorded_at) 
    FROM device_status 
    WHERE device_id = d.device_id
);

CREATE VIEW feeding_statistics_daily AS
SELECT 
    device_id,
    DATE(start_time) as feed_date,
    COUNT(*) as total_feeds,
    SUM(food_amount_grams) as total_food_grams,
    AVG(duration_ms) as avg_duration_ms,
    SUM(CASE WHEN success THEN 1 ELSE 0 END) as successful_feeds,
    SUM(CASE WHEN NOT success THEN 1 ELSE 0 END) as failed_feeds
FROM feeding_events
GROUP BY device_id, DATE(start_time);

CREATE VIEW sensor_trends_daily AS
SELECT 
    device_id,
    DATE(recorded_at) as reading_date,
    ROUND(AVG(temperature_celsius), 2) as avg_temperature,
    ROUND(MIN(temperature_celsius), 2) as min_temperature,
    ROUND(MAX(temperature_celsius), 2) as max_temperature,
    ROUND(AVG(food_level_cm), 2) as avg_food_level,
    ROUND(MIN(food_level_cm), 2) as min_food_level
FROM sensor_data
GROUP BY device_id, DATE(recorded_at);

-- Functions and Triggers
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE TRIGGER update_devices_updated_at 
    BEFORE UPDATE ON devices 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_users_updated_at 
    BEFORE UPDATE ON users 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_feeding_schedule_updated_at 
    BEFORE UPDATE ON feeding_schedule 
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Function to check low food level
CREATE OR REPLACE FUNCTION check_low_food_level()
RETURNS TRIGGER AS $$
BEGIN
    IF NEW.food_level_cm < 5.0 THEN
        INSERT INTO notifications (
            device_id,
            notification_type,
            title,
            message,
            priority
        ) VALUES (
            NEW.device_id,
            'LOW_FOOD',
            'Low Food Level Alert',
            'Food level is critically low (' || NEW.food_level_cm || ' cm). Please refill.',
            'HIGH'
        );
    END IF;
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE TRIGGER trigger_low_food_alert 
    AFTER INSERT ON sensor_data 
    FOR EACH ROW EXECUTE FUNCTION check_low_food_level();

-- Insert sample data
INSERT INTO devices (device_id, device_name, mac_address) 
VALUES ('petfeeder_01', 'Kitchen Pet Feeder', 'AA:BB:CC:DD:EE:FF');

INSERT INTO feeding_schedule (device_id, hour, minute, food_amount_grams, days_of_week)
VALUES 
    ('petfeeder_01', 8, 0, 25.0, '1111111'),
    ('petfeeder_01', 18, 0, 25.0, '1111111');