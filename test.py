#!/bin/env python3

# 因为dbus-broker要求使用systemd方式启动，所以建议系统在dbus-broker方式下测试
# 默认以系统环境进行测试，可使用以下方法对dbus-daemon进行测试：
# dbus-daemon --session --print-address
# ./test.py unix:path=/tmp/dbus-pgzTfCGOr7,guid=8061d47acfcdedf415fdc56265d6f700

import subprocess
import os
import sys


def test_normal():
    server_cmd = ["./build/dbus-test", "-s"]
    client_cmd = ["./build/dbus-test", "-c", "-t", "1"]
    p_server = subprocess.Popen(
        server_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    p_client = subprocess.Popen(
        client_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    server_output = p_server.communicate()[0].decode("utf-8")
    client_output = p_client.communicate()[0].decode("utf-8")

    print("start analyze method:")
    result = []
    a = 0
    b = 0
    for line in client_output.splitlines():
        if (line.startswith("Method:")):
            tuple_temp = line.split(":")
            if (tuple_temp[1] == "Time1"):
                a = tuple_temp[2]
            if (tuple_temp[1] == "Time2"):
                b = tuple_temp[2]
                result.append(int(b) - int(a))
    result.remove(max(result))
    result.remove(min(result))
    average = sum(result) / len(result)
    print("method call average: " + str(average) + "us")

    server_result = []
    client_result = []
    for line in server_output.splitlines():
        server_result.append(int(line))
    for line in client_output.splitlines():
        if (line.startswith("Signal:")):
            client_result.append(int(line.split(":")[3]))
    result.clear()
    for i in range(len(server_result)):
        result.append(client_result[i] - server_result[i])

    result.remove(max(result))
    result.remove(min(result))
    average = sum(result) / len(result)
    print("signal call average: " + str(average) + "us")


def test_ltp():
    print("start ltp test, wait a moment...")
    server_cmd = ["./build/dbus-test", "-s"]
    client_cmd = ["./build/dbus-test", "-c", "-t", "0"]
    subprocess.Popen(
        server_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    p_client = subprocess.Popen(
        client_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    client_output = p_client.communicate()[0].decode("utf-8")
    print("start analyze ltp:")
    result = []
    a = 0
    b = 0
    for line in client_output.splitlines():
        if (line.startswith("Method:")):
            tuple_temp = line.split(":")
            if (tuple_temp[1] == "Time1"):
                a = tuple_temp[2]
            if (tuple_temp[1] == "Time2"):
                b = tuple_temp[2]
                result.append(int(b) - int(a))
    result.remove(max(result))
    result.remove(min(result))
    average = sum(result) / len(result)
    print("ltp average: " + str(average) + "us")


if not os.access("./build/", os.F_OK):
    subprocess.run(
        ["cmake", "-Bbuild", "-DCMAKE_BUILD_TYPE=Release"])
subprocess.run(["cmake", "--build", "build"])

if (len(sys.argv) == 2):
    os.environ["DBUS_SESSION_BUS_ADDRESS"] = sys.argv[1]
test_normal()
test_ltp()
