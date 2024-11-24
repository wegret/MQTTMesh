import os
import sqlite3
from flask import Flask, jsonify, request
from datetime import datetime

base_dir = os.path.dirname(os.path.abspath(__file__))
db_path = os.path.join(base_dir, "mqtt_db")

conn = sqlite3.connect(db_path)
c = conn.cursor()

c.execute('''
CREATE TABLE IF NOT EXISTS DeviceInfo (
    ClientID TEXT PRIMARY KEY,
    Username TEXT,
    IPAddress TEXT,
    ConnectionTime DATETIME,
    SessionActive BOOLEAN
)''')

c.execute('''
CREATE TABLE IF NOT EXISTS SensorData (
    DataID INTEGER PRIMARY KEY AUTOINCREMENT,
    ClientID TEXT,
    SensorType TEXT,
    Value REAL,
    UpdateTime DATETIME,
    FOREIGN KEY (ClientID) REFERENCES DeviceInfo (ClientID)
)''')

# conn.commit()
# conn.close()

sensor_id = ['MzE4NTczOTQ1MjQ1ODEyMDc3MDY2NjQ5NDU3MDI0Njk2MzC', 'MzE4MDkyMDkzMjE3MDExOTkyMDczMzc3OTkwNTgyNTk5NjI']

devices = [
    (sensor_id[0], 'tester1', '113.45.173.169:33597', datetime.now().isoformat(), True),
    (sensor_id[1], 'tester2', '113.45.173.169:58357', datetime.now().isoformat(), False)
]
c.executemany('''
    INSERT INTO DeviceInfo (ClientID, Username, IPAddress, ConnectionTime, SessionActive) 
    VALUES (?, ?, ?, ?, ?)
''', devices)

conn.commit()

sensor_data = [
    (sensor_id[0], 'Temperature', 22.5, datetime.now().isoformat()),
    (sensor_id[0], 'Pressure', 101434, datetime.now().isoformat()),
    (sensor_id[0], 'Altitude', 45.0, datetime.now().isoformat()),
    (sensor_id[0], 'TVOC', 4, datetime.now().isoformat()),
    (sensor_id[0], 'eCO2', 400, datetime.now().isoformat()),
    (sensor_id[0], 'Raw H2', 12949, datetime.now().isoformat()),
    (sensor_id[0], 'Raw Ethanol', 18444, datetime.now().isoformat()),
    (sensor_id[0], 'Lux', 146.70, datetime.now().isoformat()),
    (sensor_id[0], 'Humidity', 0.5498, datetime.now().isoformat()),
    (sensor_id[0], 'Sound', 138, datetime.now().isoformat()),
    
    (sensor_id[1], 'Temperature', 21.0, datetime.now().isoformat()),
    (sensor_id[1], 'Pressure', 100.8, datetime.now().isoformat()),
    (sensor_id[1], 'Altitude', 50.0, datetime.now().isoformat()),
    (sensor_id[1], 'TVOC', 3, datetime.now().isoformat()),
    (sensor_id[1], 'eCO2', 350, datetime.now().isoformat()),
    (sensor_id[1], 'Raw H2', 12000, datetime.now().isoformat()),
    (sensor_id[1], 'Raw Ethanol', 18000, datetime.now().isoformat()),
    (sensor_id[1], 'Lux', 150.70, datetime.now().isoformat()),
    (sensor_id[1], 'Humidity', 0.5498, datetime.now().isoformat()),
    (sensor_id[1], 'Sound', 140, datetime.now().isoformat())
]
c.executemany('''
    INSERT INTO SensorData (ClientID, SensorType, Value, UpdateTime) 
    VALUES (?, ?, ?, ?)
''', sensor_data)

conn.commit()
conn.close()

print("数据插入完成")