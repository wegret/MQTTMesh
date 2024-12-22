#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import time
import os
from colorama import init, Fore, Style
from db_query import fetch_latest_sensor_data

import colorama

colorama.init()

init(autoreset=True)

SENSOR_COLORS = {
    "temperature": Fore.RED,
    "pressure": Fore.BLUE,
    "altitude": Fore.GREEN,
    "sealevel pressure": Fore.CYAN,
    "tvoc": Fore.MAGENTA,
    "eco2": Fore.YELLOW,
    "raw h2": Fore.RED,
    "raw ethanol": Fore.BLUE,
    "lux": Fore.GREEN,
    "humidity": Fore.CYAN,
    "sound": Fore.MAGENTA,
}


def print_sensor_data(client_id, sensor_data):
    """
    在控制台打印传感器数据。

    参数:
        client_id: 当前设备的ID
        sensor_data: 包含 (SensorType, Value, UpdateTime) 的列表
    """
    print(f"设备: {client_id}")
    print("=" * 30)
    for s_type, value, update_time in sensor_data:
        color = SENSOR_COLORS.get(s_type.lower(), Fore.WHITE)
        print(f"{color}{s_type:<25}{value:<25} {update_time}{Style.RESET_ALL}")

def clear_screen():
    """
    清空控制台屏幕。
    在 Linux/macOS 使用 'clear'，在 Windows 使用 'cls'。
    """
    if os.name == 'nt':
        os.system('cls')
    else:
        os.system('clear')

def main():
    parser = argparse.ArgumentParser(description='显示指定设备传感器的最新信息，每秒刷新。')
    parser.add_argument('clientid', help='设备的 ClientID')
    parser.add_argument('--db', default='mqtt_db', help='数据库文件路径，默认为 mqtt_db')
    args = parser.parse_args()

    client_id = args.clientid
    db_path = args.db

    while True:
        clear_screen()
        data = fetch_latest_sensor_data(db_path, client_id)
        print_sensor_data(client_id, data)
        time.sleep(0.5)

if __name__ == "__main__":
    main()
