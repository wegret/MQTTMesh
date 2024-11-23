import os
import sqlite3
from flask import Flask, jsonify, request
from datetime import datetime

base_dir = os.path.dirname(os.path.abspath(__file__))
db_path = os.path.join(base_dir, "mqtt_db")

conn = sqlite3.connect(db_path)
c = conn.cursor()

# c.execute('''
# CREATE TABLE IF NOT EXISTS DeviceInfo (
#     ClientID TEXT PRIMARY KEY,
#     Username TEXT,
#     IPAddress TEXT,
#     ConnectionTime DATETIME,
#     SessionActive BOOLEAN
# )''')

# c.execute('''
# CREATE TABLE IF NOT EXISTS SensorData (
#     DataID INTEGER PRIMARY KEY AUTOINCREMENT,
#     ClientID TEXT,
#     SensorType TEXT,
#     Value REAL,
#     UpdateTime DATETIME,
#     FOREIGN KEY (ClientID) REFERENCES DeviceInfo (ClientID)
# )''')

# conn.commit()
# conn.close()

devices = [
    ('MzE4NTczOTQ1MjQ1ODEyMDc3MDY2NjQ5NDU3MDI0Njk2MzC', 'tester1', '113.45.173.169:33597', datetime.now().isoformat(), True),
    ('MzE4MDkyMDkzMjE3MDExOTkyMDczMzc3OTkwNTgyNTk5NjI', 'tester2', '113.45.173.169:58357', datetime.now().isoformat(), False)
]
c.executemany('''
    INSERT INTO DeviceInfo (ClientID, Username, IPAddress, ConnectionTime, SessionActive) 
    VALUES (?, ?, ?, ?, ?)
''', devices)

conn.commit()

sensor_data = [
    ('MzE4NTczOTQ1MjQ1ODEyMDc3MDY2NjQ5NDU3MDI0Njk2MzC', 'Temperature', 22.5, datetime.now().isoformat()),
    ('MzE4NTczOTQ1MjQ1ODEyMDc3MDY2NjQ5NDU3MDI0Njk2MzC', 'Pressure', 101.3, datetime.now().isoformat()),
    ('MzE4NTczOTQ1MjQ1ODEyMDc3MDY2NjQ5NDU3MDI0Njk2MzC', 'Altitude', 45.0, datetime.now().isoformat()),
    ('MzE4MDkyMDkzMjE3MDExOTkyMDczMzc3OTkwNTgyNTk5NjI', 'Temperature', 21.0, datetime.now().isoformat()),
    ('MzE4MDkyMDkzMjE3MDExOTkyMDczMzc3OTkwNTgyNTk5NjI', 'Pressure', 100.8, datetime.now().isoformat()),
    ('MzE4MDkyMDkzMjE3MDExOTkyMDczMzc3OTkwNTgyNTk5NjI', 'Altitude', 50.0, datetime.now().isoformat())
]
c.executemany('''
    INSERT INTO SensorData (ClientID, SensorType, Value, UpdateTime) 
    VALUES (?, ?, ?, ?)
''', sensor_data)

conn.commit()
conn.close()

print("数据插入完成")