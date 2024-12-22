#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sqlite3

def fetch_latest_sensor_data(db_path, client_id):
    """
    获取指定 ClientID 下每种传感器的最新数据。
    
    参数:
        db_path: 数据库文件路径
        client_id: 目标设备的 ClientID
    返回:
        列表, 每个元素为 (SensorType, Value, UpdateTime)
    """
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    sql = """
    SELECT SensorType, Value, UpdateTime
    FROM (
        SELECT SensorType, Value, UpdateTime,
               ROW_NUMBER() OVER (PARTITION BY SensorType ORDER BY UpdateTime DESC) as rn
        FROM SensorData
        WHERE ClientID = ?
    )
    WHERE rn = 1;
    """
    cursor.execute(sql, (client_id,))
    rows = cursor.fetchall()
    
    conn.close()
    return rows
