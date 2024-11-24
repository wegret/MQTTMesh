from flask import Flask, jsonify, request
import sqlite3
import os
import yaml

# with open("config.yaml", 'r', encoding='utf-8') as file:
#     config = yaml.safe_load(file)
port = 8889

db_path = "database/mqtt_db"

app = Flask(__name__)

def get_db_connection():
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    return conn

@app.route('/devices', methods=['GET'])
def get_devices():
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute('SELECT * FROM DeviceInfo')
    devices = cur.fetchall()
    conn.close()
    
    devices_list = [dict(ix) for ix in devices]  
    return jsonify(devices_list)

@app.route('/sensors', methods=['GET'])
def get_sensors():
    client_id = request.args.get('ClientID', type=str)
    if not client_id:
        return "Error: No ClientID provided. Please specify a ClientID.", 400

    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute('''
        SELECT s.SensorType, s.Value, s.UpdateTime
        FROM SensorData s
        WHERE s.ClientID = ?
        ORDER BY s.UpdateTime DESC
    ''', (client_id,))
    sensors = cur.fetchall()
    conn.close()
    sensor_dict = {}
    for sensor in sensors:
        if sensor['SensorType'] not in sensor_dict:
            sensor_dict[sensor['SensorType']] = {
                'Value': sensor['Value'],
                'UpdateTime': sensor['UpdateTime']
            }

    return jsonify(sensor_dict)

if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True, port=port)
