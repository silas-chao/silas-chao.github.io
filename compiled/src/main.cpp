#include <Wire.h>
#include <WebServer.h>
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include <iostream>
using namespace std;
// Setup variables
int picked = 0;

struct vehicle
{
    String brand;
    String model;
    float weight;
    int LPlate; // 车牌号（整数形式）
    int maxCapacity;
    String status; // 添加状态字段："available" 或 "in_use"
};

struct person
{
    String name; // 添加姓名字段以支持API
    float weight;
    float height;
    int age;
    String id; // 添加ID字段以支持唯一标识
};

struct worldCoords
{
    double y; // latitude
    double x; // longitude
    double z; // altitude

    void update(double newY, double newX, double newZ)
    {
        y = newY;
        x = newX;
        z = newZ;
    }
};
worldCoords locus;
// All vehicles and people possible
vehicle vehicles[10];
person people[25];
int vehicleCount = 0;
int peopleCount = 0;
// Current vehicle
vehicle currentCar = vehicles[picked];

// Function prototypes
void something();

// Define GPIOs
#define YELLOW_LED_PIN 12
#define RGB_LED_PIN 16
#define BUZZER_PIN 23
#define LEFT_BUTTON_PIN 26
#define RIGHT_BUTTON_PIN 27

// I2C Configuration
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000

// LCD Configuration
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// Wi-Fi Configuration
const char *ssid = "GitCommited";
const char *password = "ABCDEFGH123";

WebServer server(80);

// State variables
String lcdContent = "";
bool yellowLedState = false;

// Function to initialize I2C
void i2c_master_init()
{
    Wire.begin(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    Wire.setClock(I2C_MASTER_FREQ_HZ);
    Serial.println("I2C initialized successfully");
}

// Function to initialize LCD
void lcd_init()
{
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcdContent = "Hello, World!";
    lcd.print(lcdContent);
}

// Function to handle button press events
void handle_button_press()
{
    bool leftButtonPressed = digitalRead(14) == LOW;
    bool rightButtonPressed = digitalRead(RIGHT_BUTTON_PIN) == LOW;
    if (leftButtonPressed || rightButtonPressed)
    {
        yellowLedState = true;
        digitalWrite(YELLOW_LED_PIN, HIGH); // Turn on the yellow LED
        lcd.clear();
        if (leftButtonPressed)
        {
            lcdContent = "Left Button";
            lcd.setCursor(0, 0);
            lcd.print(lcdContent);
            Serial.println("Left button pressed");
        }
        else if (rightButtonPressed)
        {
            lcdContent = "Right Button";
            lcd.setCursor(0, 0);
            lcd.print(lcdContent);
            Serial.println("Right button pressed");
        }
    }
    else
    {
        yellowLedState = false;
        digitalWrite(YELLOW_LED_PIN, LOW); // Turn off the yellow LED
        Serial.println("No button pressed");
    }
}

// I2C Scanner
void i2c_scanner()
{
    Serial.println("Scanning I2C devices...");
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("Found I2C device at address: 0x");
            Serial.println(addr, HEX);
        }
    }
    Serial.println("I2C scan complete.");
}

String replaceIP(String input)
{
    IPAddress currentIP = WiFi.softAPIP(); // 获取当前IP
    String ipStr = currentIP.toString();   // 转换为字符串，如 "192.168.4.1"
    String oldIP = "192.168.0.1";
    input.replace(oldIP, ipStr); // 替换所有 "192.168.0.1"
    return input;
}

// HTML handler function
void handleOfIndex()
{
    const string html = R"(
        <!DOCTYPE html>
        <html lang="zh-CN">
        
        <head>
            <meta charset="UTF-8">
            <title>Home Page</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    margin: 0;
                    padding: 0;
                    background-color: #f4f4f4;
                }
        
                .nav {
                    background-color: #333;
                    overflow: hidden;
                }
        
                .nav a {
                    float: left;
                    display: block;
                    color: white;
                    text-align: center;
                    padding: 14px 16px;
                    text-decoration: none;
                }
        
                .nav a:hover {
                    background-color: #575757;
                }
        
                .page {
                    padding: 20px;
                    max-width: 800px;
                    margin: 20px auto;
                    background: white;
                    border-radius: 8px;
                    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
                }
        
                h1 {
                    text-align: center;
                    color: #333;
                }
        
                .status-card {
                    background-color: #e7f3fe;
                    border-left: 6px solid #2196F3;
                    padding: 15px;
                    margin: 15px 0;
                    border-radius: 4px;
                }
        
                .status-card h3 {
                    margin: 0;
                    color: #2196F3;
                }
        
                .status-card p {
                    margin: 5px 0;
                    color: #555;
                }
        
                @media (max-width: 600px) {
                    .nav a {
                        float: none;
                        width: 100%;
                    }
                }
        
                .input-group {
                    margin: 20px 0;
                    display: flex;
                    gap: 10px;
                    max-width: 500px;
                }
        

                .input-field {
                    flex: 1;
                    padding: 8px 12px;
                    border: 1px solid #ddd;
                    border-radius: 4px;
                    font-size: 14px;
                }
        

                .submit-btn {
                    background-color: #2196F3;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    cursor: pointer;
                    transition: background-color 0.3s;
                }
        
                .submit-btn:hover {
                    background-color: #1976D2;
                }
        
                /* 结果展示样式 */
                .result-box {
                    margin-top: 15px;
                    padding: 10px;
                    background-color: #f8f9fa;
                    border-radius: 4px;
                    border: 1px solid #eee;
                }
            </style>
        </head>
        
        <body>
            <nav class="nav">
                <a href="index.html">Home Page</a>
                <a href="people.html">User Info</a>
                <a href="vehicles.html">Vehicle Control</a>
                <a href="drive.html">Drive</a>
            </nav>

            <div class="page active">
                <h1>DashBoard</h1>
                <div class="status-card">
                    <h3>Status:</h3>
                    <p id="OLLable">Online Vehicle: Still Pulling Data...</p>
                    <p id="UName">User Name: Still Pulling Data...</p>
                </div>
            </div>

            <script>
                async function GetOnlineVehicleNumber() {
                    const jData = await InvokeRequest({
                        cmd: 'GetOnlineVehicleNumber'
                    });
                    if (jData.statuses === "Timeout") {
                        document.getElementById('OLLable').textContent = "Online Vehicle: Timeout";
                        throw new Error('Network response was not ok');
                    } else if (jData.statuses !== "Fail") {
                        document.getElementById('OLLable').textContent = `Online Vehicle: ${jData.data}`; // 修复为小写data
                    } else {
                        document.getElementById('OLLable').textContent = "Online Vehicle: Err";
                    }
                }

                async function GetUserName() {
                    const jData = await InvokeRequest({
                        cmd: 'GetUserName'
                    });
                    if (jData.statuses === "Timeout") {
                        document.getElementById('UName').textContent = "User Name: Timeout";
                        throw new Error('Network response was not ok');
                    } else if (jData.statuses !== "Fail") {
                        document.getElementById('UName').textContent = `User Name: ${jData.data}`; // 修复为小写data
                    } else {
                        document.getElementById('UName').textContent = "User Name: Err";
                    }
                }

                async function InvokeRequest(data, timeout = 5000) {
                    const apiUrl = 'http://192.168.0.1/api';
                    const controller = new AbortController();
                    const timeoutId = setTimeout(() => controller.abort(), timeout);
                    try {
                        const response = await fetch(apiUrl, {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify(data),
                            signal: controller.signal
                        });
                        const result = await response.json();
                        clearTimeout(timeoutId);
                        return result;
                    } catch (error) {
                        var rJson = { statuses: "" };
                        if (error.name === "AbortError") {
                            rJson.statuses = "Timeout";
                        } else {
                            rJson.statuses = "Fail";
                        }
                        return rJson;
                    }
                }

                document.addEventListener('DOMContentLoaded', function () {
                    GetOnlineVehicleNumber();
                    GetUserName();
                });
            </script>
        </body>
        
        </html>
        )";
    server.send(200, "text/html", replaceIP(html.c_str()));
    cout << "User accessed index.html" << endl;
}

void handleOfPeople()
{
    string html = R"html_delimiter(
        <!DOCTYPE html>
        <html lang="zh-CN">
        
        <head>
            <meta charset="UTF-8">
            <title>People Page</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    margin: 0;
                    padding: 0;
                    background-color: #f4f4f4;
                }
        
                .nav {
                    background-color: #333;
                    overflow: hidden;
                }
        
                .nav a {
                    float: left;
                    display: block;
                    color: white;
                    text-align: center;
                    padding: 14px 16px;
                    text-decoration: none;
                }
        
                .nav a:hover {
                    background-color: #575757;
                }
        
                .page {
                    padding: 20px;
                    max-width: 800px;
                    margin: 20px auto;
                    background: white;
                    border-radius: 8px;
                    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
                }
        
                h1 {
                    text-align: center;
                    color: #333;
                }
        
                .status-card {
                    background-color: #e7f3fe;
                    border-left: 6px solid #2196F3;
                    padding: 15px;
                    margin: 15px 0;
                    border-radius: 4px;
                }
        
                .status-card h3 {
                    margin: 0;
                    color: #2196F3;
                }
        
                .status-card p {
                    margin: 5px 0;
                    color: #555;
                }
        
                @media (max-width: 600px) {
                    .nav a {
                        float: none;
                        width: 100%;
                    }
                }
        
                .input-group {
                    margin: 20px 0;
                    display: flex;
                    gap: 10px;
                    max-width: 500px;
                }
        

                .input-field {
                    flex: 1;
                    padding: 8px 12px;
                    border: 1px solid #ddd;
                    border-radius: 4px;
                    font-size: 14px;
                }
        

                .submit-btn {
                    background-color: #2196F3;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    cursor: pointer;
                    transition: background-color 0.3s;
                }
        
                .submit-btn:hover {
                    background-color: #1976D2;
                }
        
                .result-box {
                    margin-top: 15px;
                    padding: 10px;
                    background-color: #f8f9fa;
                    border-radius: 4px;
                    border: 1px solid #eee;
                }
        
                .list-container {
                    background: white;
                    border-radius: 8px;
                    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
                    max-width: 800px;
                    margin: 20px auto;
                    padding: 20px;
                }
        
                /* 列表项样式 */
                .list-item {
                    display: flex;
                    align-items: center;
                    padding: 12px;
                    border-bottom: 1px solid #eee;
                    transition: background 0.3s;
                }
        
                .list-item:hover {
                    background-color: #f8f9fa;
                }
        
                /* 输入组样式 */
                .input-group {
                    display: flex;
                    gap: 10px;
                    margin-bottom: 20px;
                }
        
                .input-field {
                    flex: 1;
                    padding: 8px 12px;
                    border: 1px solid #ddd;
                    border-radius: 4px;
                }
        
                /* 操作按钮样式 */
                .action-btn {
                    padding: 6px 12px;
                    border: none;
                    border-radius: 4px;
                    cursor: pointer;
                    transition: opacity 0.3s;
                }
        
                .add-btn {
                    background-color: #4CAF50;
                    color: white;
                }
        
                .delete-btn {
                    background-color: #f44336;
                    color: white;
                    margin-left: auto;
                }
        
                /* 空状态提示 */
                .empty-state {
                    color: #666;
                    text-align: center;
                    padding: 20px;
                }
            </style>
        </head>
        
        <body>
            <nav class="nav">
                <a href="index.html">Home Page</a>
                <a href="people.html">User Info</a>
                <a href="vehicles.html">Vehicle Control</a>
                <a href="drive.html">Drive</a>
            </nav>

            <div class="list-container">
                <div class="input-group">
                    <input type="text" class="input-field" id="itemInput" placeholder="input">
                    <button class="action-btn add-btn" onclick="addListItem(-1)">add people</button>
                </div>

                <div id="listContent">
                    <div class="empty-state">No item yet</div>
                </div>
            </div>

            <script>
                // 添加列表项函数
                async function addListItem(text = -1) {
                    const input = document.getElementById('itemInput');
                    const listContainer = document.getElementById('listContent');
                    let content = text === -1 ? input.value.trim() : text;

                    if (content === "") {
                        alert('Please input valid data');
                        input.focus();
                        return false;
                    }

                    try {
                        const addSuccess = await AddPeople(content);
                        if (!addSuccess) return false;

                        if (listContainer.querySelector('.empty-state')) {
                            listContainer.innerHTML = '';
                        }

                        const newItem = document.createElement('div');
                        newItem.className = 'list-item';
                        newItem.innerHTML = `
                            <span>${content}</span>
                            <button class="action-btn delete-btn" onclick="deleteListItem(this)">Remove</button>
                        `;

                        listContainer.appendChild(newItem);
                        input.value = '';
                        return true;
                    } catch (error) {
                        console.error('Add item failed:', error);
                        return false;
                    }
                }

                // 删除列表项函数
                async function deleteListItem(button) {
                    const item = button.closest('.list-item');
                    const content = item.querySelector('span').textContent;

                    try {
                        const deleteSuccess = await DelPeople(content);
                        if (!deleteSuccess) return;

                        item.remove();
                        const listContainer = document.getElementById('listContent');
                        if (listContainer.children.length === 0) {
                            listContainer.innerHTML = '<div class="empty-state">No item yet</div>';
                        }
                    } catch (error) {
                        console.error('Delete item failed:', error);
                    }
                }

                // 获取人员列表
                async function GetListOfPeople() {
                    try {
                        const rJson = await InvokeRequest({ cmd: "GetListOfPeople" });

                        // 后端返回格式：{"data": "name1|name2|name3"}
                        if (rJson.data) {
                            const listContainer = document.getElementById('listContent');
                            listContainer.innerHTML = '';

                            if (rJson.data === "") {
                                listContainer.innerHTML = '<div class="empty-state">No item yet</div>';
                                return;
                            }

                            const TArray = rJson.data.split("|");
                            for (const x of TArray) {
                                if (x.trim()) {
                                    await addListItem(x.trim()); // 使用addListItem添加，避免重复逻辑
                                }
                            }
                        }
                    } catch (error) {
                        console.error('Get list failed:', error);
                    }
                }

                // 添加人员
                async function AddPeople(content) {
                    try {
                        const rJson = await InvokeRequest({
                            cmd: "AddPeople",
                            Name: content
                        });

                        // 后端返回格式：{"statuses": "Success"} 或 {"statuses": "Fail"}
                        if (rJson.statuses === "Success") {
                            return true;
                        }
                        handleError(rJson.statuses || 'Fail', 'add people');
                        return false;
                    } catch (error) {
                        console.error('Add failed:', error);
                        return false;
                    }
                }

                // 删除人员
                async function DelPeople(content) {
                    try {
                        const rJson = await InvokeRequest({
                            cmd: "DelPeople",
                            Name: content
                        });

                        // 后端返回格式：{"statuses": "Success"} 或 {"statuses": "Fail"}
                        if (rJson.statuses === "Success") {
                            return true;
                        }
                        handleError(rJson.statuses || 'Fail', 'delete people');
                        return false;
                    } catch (error) {
                        console.error('Delete failed:', error);
                        return false;
                    }
                }

                // 错误处理
                function handleError(status, operation) {
                    const messageMap = {
                        Timeout: `Operation timeout: ${operation}`,
                        Fail: `Operation failed: ${operation}`,
                        default: `Unknown error: ${operation}`
                    };
                    alert(messageMap[status] || messageMap.default);
                }

                // 发送请求
                async function InvokeRequest(data, timeout = 5000) {
                    const apiUrl = 'http://192.168.0.1/api';
                    const controller = new AbortController();
                    const timeoutId = setTimeout(() => controller.abort(), timeout);

                    try {
                        const response = await fetch(apiUrl, {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify(data),
                            signal: controller.signal
                        });

                        const result = await response.json();
                        clearTimeout(timeoutId);
                        return result;
                    } catch (error) {
                        return {
                            statuses: error.name === "AbortError" ? "Timeout" : "Fail"
                        };
                    }
                }

                document.addEventListener('DOMContentLoaded', function () {
                    GetListOfPeople();
                });
            </script>
        </body>
        
        </html>
        )html_delimiter";
    server.send(200, "text/html", replaceIP(html.c_str()));
    cout << "User accessed people.html" << endl;
}

void handleOfVehicles()
{
    const string html = R"html_delimiter(
        <!DOCTYPE html>
        <html lang="zh-CN">
        
        <head>
            <meta charset="UTF-8">
            <title>Vehicle Page</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    margin: 0;
                    padding: 0;
                    background-color: #f4f4f4;
                }
        
                .nav {
                    background-color: #333;
                    overflow: hidden;
                }
        
                .nav a {
                    float: left;
                    display: block;
                    color: white;
                    text-align: center;
                    padding: 14px 16px;
                    text-decoration: none;
                }
        
                .nav a:hover {
                    background-color: #575757;
                }
        
                .page {
                    padding: 20px;
                    max-width: 800px;
                    margin: 20px auto;
                    background: white;
                    border-radius: 8px;
                    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
                }
        
                h1 {
                    text-align: center;
                    color: #333;
                }
        
                .status-card {
                    background-color: #e7f3fe;
                    border-left: 6px solid #2196F3;
                    padding: 15px;
                    margin: 15px 0;
                    border-radius: 4px;
                }
        
                .status-card h3 {
                    margin: 0;
                    color: #2196F3;
                }
        
                .status-card p {
                    margin: 5px 0;
                    color: #555;
                }
        
                @media (max-width: 600px) {
                    .nav a {
                        float: none;
                        width: 100%;
                    }
                }
        
                .input-group {
                    margin: 20px 0;
                    display: flex;
                    gap: 10px;
                    max-width: 500px;
                }
        
                .input-field {
                    flex: 1;
                    padding: 8px 12px;
                    border: 1px solid #ddd;
                    border-radius: 4px;
                    font-size: 14px;
                }
                .submit-btn {
                    background-color: #2196F3;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    cursor: pointer;
                    transition: background-color 0.3s;
                }
        
                .submit-btn:hover {
                    background-color: #1976D2;
                }
        
                /* 结果展示样式 */
                .result-box {
                    margin-top: 15px;
                    padding: 10px;
                    background-color: #f8f9fa;
                    border-radius: 4px;
                    border: 1px solid #eee;
                }
            </style>
        </head>
        
        <body>
            <nav class="nav">
                <a href="index.html">Home Page</a>
                <a href="people.html">User Info</a>
                <a href="vehicles.html">Vehicle Control</a>
                <a href="drive.html">Drive</a>
            </nav>

            <div class="page active">
                <h1>DashBoard Of Vehicles</h1>

                <div id="vehicleList" class="vehicle-list">
                    <div class="status-card example-card">
                        <h3>Vehicle Name: DemoCar</h3>
                        <p>Type: Sedan</p>
                        <p>Location: Parking Lot A</p>
                        <p>Brand: Toyota</p>
                        <p>Model: Camry</p>
                        <p>Weight: 1500 kg</p>
                        <p>License Plate: ABC123</p>
                        <p>Max Capacity: 5 persons</p>
                    </div>
                </div>

                <h2>Vehicle Operations</h2>

                <div class="input-group">
                    <input type="text" class="input-field" id="changeBrandId" placeholder="Vehicle ID">
                    <input type="text" class="input-field" id="changeBrandValue" placeholder="New Brand">
                    <button class="submit-btn" onclick="handleChangeBrand()">Change Brand</button>
                </div>

                <div class="input-group">
                    <input type="text" class="input-field" id="changeModelId" placeholder="Vehicle ID">
                    <input type="text" class="input-field" id="changeModelValue" placeholder="New Model">
                    <button class="submit-btn" onclick="handleChangeModel()">Change Model</button>
                </div>

                <div class="input-group">
                    <input type="text" class="input-field" id="changeWeightId" placeholder="Vehicle ID">
                    <input type="number" class="input-field" id="changeWeightValue" placeholder="New Weight (kg)">
                    <button class="submit-btn" onclick="handleChangeWeight()">Change Weight</button>
                </div>

                <div class="input-group">
                    <input type="text" class="input-field" id="changeLicenseId" placeholder="Vehicle ID">
                    <input type="text" class="input-field" id="changeLicenseValue" placeholder="New License Plate">
                    <button class="submit-btn" onclick="handleChangeLicense()">Change License</button>
                </div>

                <div class="input-group">
                    <input type="text" class="input-field" id="changeCapacityId" placeholder="Vehicle ID">
                    <input type="number" class="input-field" id="changeCapacityValue" placeholder="New Max Capacity">
                    <button class="submit-btn" onclick="handleChangeCapacity()">Change Capacity</button>
                </div>

                </br>
                <div class="input-group" style="flex-wrap: wrap; gap: 8px;">
                    <input type="text" class="input-field" id="addVehicleBrand" placeholder="Brand">
                    <input type="text" class="input-field" id="addVehicleModel" placeholder="Model">
                    <input type="number" class="input-field" id="addVehicleWeight" placeholder="Weight (kg)">
                    <input type="text" class="input-field" id="addVehicleLicense" placeholder="License Plate">
                    <input type="number" class="input-field" id="addVehicleCapacity" placeholder="Max Capacity">
                    <button class="submit-btn" onclick="handleAddVehicle()">Add Vehicle</button>
                </div>
                </br>
                <div class="input-group">
                    <input type="text" class="input-field" id="deleteVehicleId" placeholder="Vehicle ID">
                    <button class="submit-btn" onclick="handleDeleteVehicle()">Delete Vehicle</button>
                </div>

                <div class="result-box" id="resultBox"></div>
            </div>

            <script>

                function validateInput(id, value, type = 'text') {
                    if (!value.trim()) {
                        alert(`Please enter a valid ${id.replace(/([A-Z])/g, ' $1').toLowerCase()}`);
                        document.getElementById(id).focus();
                        return false;
                    }
                    if (type === 'number' && (isNaN(value) || value <= 0)) {
                        alert(`Please enter a valid positive number for ${id.replace(/([A-Z])/g, ' $1').toLowerCase()}`);
                        document.getElementById(id).focus();
                        return false;
                    }
                    return true;
                }


                async function handleChangeBrand() {
                    const vehicleId = document.getElementById('changeBrandId').value;
                    const newValue = document.getElementById('changeBrandValue').value;
                    if (!validateInput('changeBrandId', vehicleId) || !validateInput('changeBrandValue', newValue)) return;
                    const jData = await InvokeRequest({
                        cmd: 'ChangeBrand',
                        VehicleID: vehicleId,
                        NewBrand: newValue
                    });
                    showResult(jData);
                }

                async function handleChangeModel() {
                    const vehicleId = document.getElementById('changeModelId').value;
                    const newValue = document.getElementById('changeModelValue').value;
                    if (!validateInput('changeModelId', vehicleId) || !validateInput('changeModelValue', newValue)) return;
                    const jData = await InvokeRequest({
                        cmd: 'ChangeModel',
                        VehicleID: vehicleId,
                        NewModel: newValue
                    });
                    showResult(jData);
                }

                async function handleChangeWeight() {
                    const vehicleId = document.getElementById('changeWeightId').value;
                    const newValue = document.getElementById('changeWeightValue').value;
                    if (!validateInput('changeWeightId', vehicleId) || !validateInput('changeWeightValue', newValue, 'number')) return;
                    const jData = await InvokeRequest({
                        cmd: 'ChangeWeight',
                        VehicleID: vehicleId,
                        NewWeight: parseFloat(newValue)
                    });
                    showResult(jData);
                }

                async function handleChangeLicense() {
                    const vehicleId = document.getElementById('changeLicenseId').value;
                    const newValue = document.getElementById('changeLicenseValue').value;
                    if (!validateInput('changeLicenseId', vehicleId) || !validateInput('changeLicenseValue', newValue)) return;
                    const jData = await InvokeRequest({
                        cmd: 'ChangeLicensePlate',
                        VehicleID: vehicleId,
                        NewLicense: newValue
                    });
                    showResult(jData);
                }

                async function handleChangeCapacity() {
                    const vehicleId = document.getElementById('changeCapacityId').value;
                    const newValue = document.getElementById('changeCapacityValue').value;
                    if (!validateInput('changeCapacityId', vehicleId) || !validateInput('changeCapacityValue', newValue, 'number')) return;
                    const jData = await InvokeRequest({
                        cmd: 'ChangeMaxCapacity',
                        VehicleID: vehicleId,
                        NewCapacity: parseInt(newValue)
                    });
                    showResult(jData);
                }

                async function handleAddVehicle() {
                    const newVehicle = {
                        Brand: document.getElementById('addVehicleBrand').value,
                        Model: document.getElementById('addVehicleModel').value,
                        Weight: document.getElementById('addVehicleWeight').value,
                        License: document.getElementById('addVehicleLicense').value,
                        Capacity: document.getElementById('addVehicleCapacity').value
                    };
                    if (!validateInput('addVehicleBrand', newVehicle.Brand) ||
                        !validateInput('addVehicleModel', newVehicle.Model) ||
                        !validateInput('addVehicleWeight', newVehicle.Weight, 'number') ||
                        !validateInput('addVehicleLicense', newVehicle.License) ||
                        !validateInput('addVehicleCapacity', newVehicle.Capacity, 'number')) return;
                    newVehicle.Weight = parseFloat(newVehicle.Weight);
                    newVehicle.Capacity = parseInt(newVehicle.Capacity);
                    const jData = await InvokeRequest({
                        cmd: 'AddVehicle',
                        Data: newVehicle
                    });
                    showResult(jData);
                }

                async function handleDeleteVehicle() {
                    const vehicleId = document.getElementById('deleteVehicleId').value;
                    if (!validateInput('deleteVehicleId', vehicleId)) return;
                    const jData = await InvokeRequest({
                        cmd: 'DeleteVehicle',
                        VehicleID: vehicleId
                    });
                    showResult(jData);
                }


                function showResult(jData) {
                    const resultBox = document.getElementById('resultBox');
                    if (jData.statuses === "Success") {
                        resultBox.textContent = `success: ${jData.message || ''}`;
                        resultBox.style.color = 'green';
                        GetVehicles();
                    } else {
                        resultBox.textContent = `fail: ${jData.message || jData.error || 'unexpected error'}`;
                        resultBox.style.color = 'red';
                    }
                }


                async function GetVehicles() {
                    const jData = await InvokeRequest({ cmd: 'GetVehicles' });
                    const container = document.getElementById('vehicleList');
                    container.innerHTML = '';

                    if (jData.statuses === "Success" && jData.Data && jData.Data.length > 0) {
                        jData.Data.forEach(vehicle => {
                            container.innerHTML += `
                                <div class="status-card">
                                    <h3>${vehicle.Name} (ID: ${vehicle.VehicleID})</h3>
                                    <p>Type: ${vehicle.Type}</p>
                                    <p>Location: ${vehicle.Location}</p>
                                    <p>Brand: ${vehicle.Brand}</p>
                                    <p>Model: ${vehicle.Model}</p>
                                    <p>Weight: ${vehicle.Weight} kg</p>
                                    <p>License Plate: ${vehicle.LicensePlate}</p>
                                    <p>Max Capacity: ${vehicle.MaxCapacity} persons</p>
                                </div>
                            `;
                        });
                    } else {
                        container.innerHTML = '<div class="status-card">No vehicles available</div>';
                    }
                }

                async function InvokeRequest(data, timeout = 5000) {
                    const apiUrl = 'http://192.168.0.1/api';
                    const controller = new AbortController();
                    const timeoutId = setTimeout(() => controller.abort(), timeout);

                    try {
                        const response = await fetch(apiUrl, {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify(data),
                            signal: controller.signal
                        });
                        const result = await response.json();
                        clearTimeout(timeoutId);
                        return result;
                    } catch (error) {
                        return {
                            statuses: error.name === "AbortError" ? "Timeout" : "Fail",
                            message: error.name === "AbortError" ? "Request timed out" : "Network error"
                        };
                    }
                }

                document.addEventListener('DOMContentLoaded', function () {
                    GetVehicles();
                });
            </script>
        </body>
        
        </html>
        )html_delimiter";
    server.send(200, "text/html", replaceIP(html.c_str()));
    cout << "User accessed vehicles.html" << endl;
}

void handleOfDrive()
{
    const string html = R"html_delimiter(
        <!DOCTYPE html>
        <html lang="zh-CN">
        
        <head>
            <meta charset="UTF-8">
            <title>Drive Controller</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    margin: 0;
                    padding: 0;
                    background-color: #f4f4f4;
                }
        
                .nav {
                    background-color: #333;
                    overflow: hidden;
                }
        
                .nav a {
                    float: left;
                    display: block;
                    color: white;
                    text-align: center;
                    padding: 14px 16px;
                    text-decoration: none;
                }
        
                .nav a:hover {
                    background-color: #575757;
                }
        
                .page {
                    padding: 20px;
                    max-width: 800px;
                    margin: 20px auto;
                    background: white;
                    border-radius: 8px;
                    box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
                }
        
                .status-card {
                    background-color: #e7f3fe;
                    border-left: 6px solid #2196F3;
                    padding: 15px;
                    margin: 15px 0;
                    border-radius: 4px;
                    cursor: pointer;
                }
        
                .input-group {
                    margin: 20px 0;
                    display: flex;
                    gap: 10px;
                    flex-wrap: wrap;
                }
        
                .input-field {
                    flex: 1;
                    padding: 8px 12px;
                    border: 1px solid #ddd;
                    border-radius: 4px;
                    min-width: 200px;
                }
        
                .submit-btn {
                    background-color: #2196F3;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    cursor: pointer;
                }
        
                .step-container {
                    display: none;
                }
        
                .step-active {
                    display: block;
                }
        
                .passenger-item {
                    display: flex;
                    align-items: center;
                    gap: 10px;
                    margin: 8px 0;
                    padding: 8px;
                    background: #f8f9fa;
                    border-radius: 4px;
                }
        
                .capacity-warning {
                    color: #dc3545;
                    font-weight: bold;
                }
        
                #selectedVehicleInfo {
                    background: #e2f0ff;
                    padding: 15px;
                    border-radius: 4px;
                    margin: 15px 0;
                }
        
                .loader {
                    border: 4px solid #f3f3f3;
                    border-top: 4px solid #3498db;
                    border-radius: 50%;
                    width: 30px;
                    height: 30px;
                    animation: spin 1s linear infinite;
                    margin: 20px auto;
                }
        
                @keyframes spin {
                    0% {
                        transform: rotate(0deg);
                    }
        
                    100% {
                        transform: rotate(360deg);
                    }
                }
            </style>
        </head>
        
        <body>
            <nav class="nav">
                <a href="index.html">Home Page</a>
                <a href="people.html">User Info</a>
                <a href="vehicles.html">Vehicle Control</a>
                <a href="drive.html">Drive</a>
            </nav>

            <div class="page">
                <div id="step1" class="step-active step-container">
                    <h2>Choose Vehicle (Remaining Seats: <span id="remainingSeats">-</span>)</h2>
                    <div id="vehicleList">
                        <div class="loader">Loading vehicles...</div>
                    </div>
                </div>

                <div id="step2" class="step-container">
                    <div id="selectedVehicleInfo">
                        <h3>Chosen Vehicle: <span id="selectedVehicleName">-</span></h3>
                        <p>License Plate: <span id="selectedVehicleLicense">-</span></p>
                        <p>Max People Count: <span id="selectedVehicleCapacity">-</span></p>
                    </div>

                    <h2>Add People</h2>
                    <div class="input-group">
                        <select class="input-field" id="existingUsers">
                            <option value="">Loading...</option>
                        </select>
                        <button class="submit-btn" onclick="addExistingUser()">Add People</button>
                    </div>

                    <div class="input-group">
                        <input type="text" class="input-field" id="customUserName" placeholder="Custom User Name">
                        <button class="submit-btn" onclick="addCustomUser()">Add Custom People</button>
                    </div>

                    <div class="input-group">
                        <button class="submit-btn" onclick="addEmptySeat()">Add Free Seat</button>
                    </div>

                    <h2>Current People List</h2>
                    <div id="passengerList"></div>

                    <div class="input-group">
                        <button class="submit-btn" onclick="startDrive()" style="width:100%;">Start</button>
                        <button class="submit-btn" onclick="resetSelection()" style="background-color:#6c757d;">Re-Choose Vehicle</button>
                    </div>
                </div>

                <div id="resultBox" class="result-box"></div>
            </div>

            <script>
                const state = {
                    selectedVehicle: null,
                    passengers: [],
                    vehicles: [],
                    users: []
                };

                const API_ENDPOINTS = {
                    VEHICLES: 'http://192.168.0.1/api/vehicle',  // GET
                    USERS: 'http://192.168.0.1/api/users',      // GET
                    DRIVES: 'http://192.168.0.1/api/drives'     // POST
                };

                document.addEventListener('DOMContentLoaded', async () => {
                    try {
                        await Promise.all([fetchVehicles(), fetchUsers()]);
                        renderVehicles();
                        renderUserOptions();
                    } catch (error) {
                        showMessage('Failed to load data, please try again', 'red');
                    }
                });

                async function apiRequest(url, method = 'GET', body = null) {
                    const headers = { 'Content-Type': 'application/json' };
                    const config = { method, headers };
                    if (body) config.body = JSON.stringify(body);

                    try {
                        const response = await fetch(url, config);
                        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
                        return await response.json();
                    } catch (error) {
                        console.error('Failed to fetch API:', error);
                        throw error;
                    }
                }

                async function fetchVehicles() {
                    const data = await apiRequest(API_ENDPOINTS.VEHICLES);
                    state.vehicles = data.data.map(v => ({
                        id: v.id,
                        name: v.name,
                        license: v.license_plate,
                        capacity: v.max_capacity,
                        status: v.status
                    }));
                }

                async function fetchUsers() {
                    const data = await apiRequest(API_ENDPOINTS.USERS);
                    state.users = data.data.map(u => ({
                        id: u.id,
                        name: u.full_name
                    }));
                }

                function renderVehicles() {
                    const container = document.getElementById('vehicleList');
                    if (state.vehicles.length === 0) {
                        container.innerHTML = '<div class="status-card">No vehicles available</div>';
                        return;
                    }
                    container.innerHTML = state.vehicles.map(vehicle => `
                        <div class="status-card" onclick="${vehicle.status === 'available' ? `selectVehicle('${vehicle.id}')` : ''}" 
                            style="cursor: ${vehicle.status === 'available' ? 'pointer' : 'not-allowed'}; opacity: ${vehicle.status === 'available' ? 1 : 0.5};">
                            <h3>${vehicle.name}</h3>
                            <p>License: ${vehicle.license}</p>
                            <p>Max People: ${vehicle.capacity} persons</p>
                            <p>Status: ${vehicle.status}</p>
                        </div>
                    `).join('');
                }

                function selectVehicle(vehicleId) {
                    state.selectedVehicle = state.vehicles.find(v => v.id === vehicleId);
                    if (!state.selectedVehicle || state.selectedVehicle.status !== 'available') {
                        showMessage('Vehicle not available', 'red');
                        return;
                    }
                    state.passengers = [];

                    document.getElementById('selectedVehicleName').textContent = state.selectedVehicle.name;
                    document.getElementById('selectedVehicleLicense').textContent = state.selectedVehicle.license;
                    document.getElementById('selectedVehicleCapacity').textContent = state.selectedVehicle.capacity;

                    document.getElementById('step1').classList.remove('step-active');
                    document.getElementById('step2').classList.add('step-active');
                    updateUI();
                }

                function renderUserOptions() {
                    const select = document.getElementById('existingUsers');
                    select.innerHTML = '<option value="">Choose People</option>' +
                        state.users.map(user => `
                            <option value="${user.id}">${user.name}</option>
                        `).join('');
                }

                function addExistingUser() {
                    const userId = document.getElementById('existingUsers').value;
                    if (!userId) {
                        showMessage('Please select a user', 'red');
                        return;
                    }
                    const user = state.users.find(u => u.id === userId);
                    if (state.passengers.length >= state.selectedVehicle.capacity) {
                        showMessage('Vehicle capacity exceeded', 'red');
                        return;
                    }
                    if (state.passengers.some(p => p.type === 'user' && p.id === user.id)) {
                        showMessage('User already added', 'red');
                        return;
                    }
                    addPassenger({ type: 'user', id: user.id, name: user.name });
                }

                function addCustomUser() {
                    const name = document.getElementById('customUserName').value.trim();
                    if (!name) {
                        showMessage('Please enter a custom user name', 'red');
                        document.getElementById('customUserName').focus();
                        return;
                    }
                    if (state.passengers.length >= state.selectedVehicle.capacity) {
                        showMessage('Vehicle capacity exceeded', 'red');
                        return;
                    }
                    addPassenger({ type: 'custom', name });
                    document.getElementById('customUserName').value = '';
                }

                function addEmptySeat() {
                    if (state.passengers.length >= state.selectedVehicle.capacity) {
                        showMessage('Vehicle capacity exceeded', 'red');
                        return;
                    }
                    addPassenger({ type: 'empty', name: 'Free Seat' });
                }

                function addPassenger(passenger) {
                    state.passengers.push(passenger);
                    updateUI();
                }

                function removePassenger(index) {
                    state.passengers.splice(index, 1);
                    updateUI();
                }

                function updateUI() {
                    const remaining = state.selectedVehicle ? state.selectedVehicle.capacity - state.passengers.length : '-';
                    document.getElementById('remainingSeats').textContent = remaining;

                    const passengerList = document.getElementById('passengerList');
                    passengerList.innerHTML = state.passengers.map((p, index) => `
                        <div class="status-card">
                            <p>${p.name}</p>
                            <button class="submit-btn" onclick="removePassenger(${index})" style="background-color:#dc3545;">Remove</button>
                        </div>
                    `).join('') || '<p>No passengers added</p>';
                }

                async function startDrive() {
                    if (state.passengers.length === 0) {
                        showMessage('Error: Please add at least one passenger or free seat', 'red');
                        return;
                    }

                    try {
                        const payload = {
                            vehicle_id: state.selectedVehicle.id,
                            passengers: state.passengers.map(p => ({
                                type: p.type,
                                user_id: p.type === 'user' ? p.id : null,
                                custom_name: p.type === 'custom' ? p.name : null
                            }))
                        };

                        const response = await apiRequest(API_ENDPOINTS.DRIVES, 'POST', payload);

                        if (response.success) {
                            showMessage(`Trip started successfully! Drive ID: ${response.drive_id}, Start Time: ${response.start_time}`, 'green');
                            resetSelection();
                        } else {
                            showMessage(`Failed to start: ${response.error || 'Unexpected error'} - ${response.message || ''}`, 'red');
                        }
                    } catch (error) {
                        showMessage('Failed to connect to server', 'red');
                    }
                }

                function resetSelection() {
                    state.selectedVehicle = null;
                    state.passengers = [];
                    document.getElementById('selectedVehicleName').textContent = '-';
                    document.getElementById('selectedVehicleLicense').textContent = '-';
                    document.getElementById('selectedVehicleCapacity').textContent = '-';
                    document.getElementById('step2').classList.remove('step-active');
                    document.getElementById('step1').classList.add('step-active');
                    updateUI();
                    renderVehicles();
                }

                function showMessage(message, color) {
                    const resultBox = document.getElementById('resultBox');
                    resultBox.textContent = message;
                    resultBox.style.color = color;
                }

                updateUI();
            </script>
        </body>
        
        </html>
        )html_delimiter";
    server.send(200, "text/html", replaceIP(html.c_str()));
    cout << "User accessed drive.html" << endl;
}

void handleNotFound()
{
    server.send(404, "text/plain", "404: Not Found - The requested URL was not found on this server.");
}

void handleApiPost()
{
    if (!server.hasArg("plain"))
    {
        server.send(400, "application/json", "{\"error\":\"No body provided\"}");
        return;
    }
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    String cmd = doc["cmd"].as<String>();

    // Main
    if (cmd == "GetOnlineVehicleNumber")
    {
        int onlineCount = 0;
        for (int i = 0; i < vehicleCount; i++)
        {
            if (vehicles[i].status == "in_use")
                onlineCount++;
        }
        doc.clear();
        doc["data"] = String(onlineCount);
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
    else if (cmd == "GetUserName")
    {
        doc.clear();
        doc["data"] = "admin";
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
    // People
    else if (cmd == "GetListOfPeople")
    {
        String nameList;
        for (int i = 0; i < peopleCount; i++)
        {
            nameList += people[i].name;
            if (i < peopleCount - 1)
                nameList += "|";
        }
        doc.clear();
        doc["data"] = nameList;
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
    else if (cmd == "AddPeople")
    {
        String name = doc["Name"].as<String>();
        if (peopleCount < 25)
        {
            people[peopleCount++] = {name, 70.0, 1.70, 30, "U" + String(peopleCount + 1)};
            doc.clear();
            doc["statuses"] = "Success";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
        else
        {
            doc.clear();
            doc["statuses"] = "Fail";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
    }
    else if (cmd == "DelPeople")
    {
        String name = doc["Name"].as<String>();
        for (int i = 0; i < peopleCount; i++)
        {
            if (people[i].name == name)
            {
                for (int j = i; j < peopleCount - 1; j++)
                {
                    people[j] = people[j + 1];
                }
                peopleCount--;
                doc.clear();
                doc["statuses"] = "Success";
                String response;
                serializeJson(doc, response);
                server.send(200, "application/json", response);
                return;
            }
        }
        doc.clear();
        doc["statuses"] = "Fail";
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
    // Vehicle Control
    else if (cmd == "GetVehicles")
    {
        doc.clear();
        doc["statuses"] = "Success";
        JsonArray data = doc.createNestedArray("Data");
        for (int i = 0; i < vehicleCount; i++)
        {
            JsonObject v = data.createNestedObject();
            v["VehicleID"] = "V" + String(i + 1);
            v["Name"] = vehicles[i].brand + " " + vehicles[i].model;
            v["Type"] = "Unknown";           // 未定义类型，固定值
            v["Location"] = "Parking Lot A"; // 示例值
            v["Brand"] = vehicles[i].brand;
            v["Model"] = vehicles[i].model;
            v["Weight"] = vehicles[i].weight;
            v["LicensePlate"] = String(vehicles[i].LPlate);
            v["MaxCapacity"] = vehicles[i].maxCapacity;
        }
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
    else if (cmd == "ChangeBrand" || cmd == "ChangeModel" || cmd == "ChangeWeight" ||
             cmd == "ChangeLicensePlate" || cmd == "ChangeMaxCapacity")
    {
        String vehicleID = doc["VehicleID"].as<String>();
        int index = vehicleID.substring(1).toInt() - 1;
        if (index >= 0 && index < vehicleCount)
        {
            if (cmd == "ChangeBrand")
                vehicles[index].brand = doc["NewBrand"].as<String>();
            else if (cmd == "ChangeModel")
                vehicles[index].model = doc["NewModel"].as<String>();
            else if (cmd == "ChangeWeight")
                vehicles[index].weight = doc["NewWeight"].as<float>();
            else if (cmd == "ChangeLicensePlate")
                vehicles[index].LPlate = doc["NewLicense"].as<int>();
            else if (cmd == "ChangeMaxCapacity")
                vehicles[index].maxCapacity = doc["NewCapacity"].as<int>();
            doc.clear();
            doc["statuses"] = "Success";
            doc["message"] = "success";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
        else
        {
            doc.clear();
            doc["statuses"] = "Fail";
            doc["message"] = "Vehicle not found";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
    }
    else if (cmd == "AddVehicle")
    {
        if (vehicleCount < 10)
        {
            JsonObject data = doc["Data"].as<JsonObject>();
            vehicles[vehicleCount++] = {
                data["Brand"].as<String>(),
                data["Model"].as<String>(),
                data["Weight"].as<float>(),
                data["License"].as<int>(),
                data["Capacity"].as<int>(),
                "available"};
            doc.clear();
            doc["statuses"] = "Success";
            doc["message"] = "Vehicle ID: V" + String(vehicleCount);
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
        else
        {
            doc.clear();
            doc["statuses"] = "Fail";
            doc["message"] = "Max vehicles reached";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
    }
    else if (cmd == "DeleteVehicle")
    {
        String vehicleID = doc["VehicleID"].as<String>();
        int index = vehicleID.substring(1).toInt() - 1;
        if (index >= 0 && index < vehicleCount)
        {
            for (int i = index; i < vehicleCount - 1; i++)
            {
                vehicles[i] = vehicles[i + 1];
            }
            vehicleCount--;
            doc.clear();
            doc["statuses"] = "Success";
            doc["message"] = "";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
        else
        {
            doc.clear();
            doc["statuses"] = "Fail";
            doc["message"] = "Vehicle not found";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
    }
    else
    {
        server.send(400, "application/json", "{\"error\":\"Unknown command\"}");
    }
}

// 处理 GET /api/vehicle
void handleGetVehicleList()
{
    JsonDocument doc;
    JsonArray data = doc.createNestedArray("data");
    for (int i = 0; i < vehicleCount; i++)
    {
        JsonObject v = data.createNestedObject();
        v["id"] = "V" + String(i + 1);
        v["name"] = vehicles[i].brand + " " + vehicles[i].model;
        v["license_plate"] = String(vehicles[i].LPlate);
        v["max_capacity"] = vehicles[i].maxCapacity;
        v["status"] = vehicles[i].status;
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// 处理 GET /api/users
void handleGetPeopleList()
{
    JsonDocument doc;
    JsonArray data = doc.createNestedArray("data");
    for (int i = 0; i < peopleCount; i++)
    {
        JsonObject p = data.createNestedObject();
        p["id"] = people[i].id;
        p["full_name"] = people[i].name;
        p["email"] = "";
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// 处理 POST /api/drives
void handleDrive()
{
    if (!server.hasArg("plain"))
    {
        server.send(400, "application/json", "{\"error\":\"No body provided\"}");
        return;
    }
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    String vehicleID = doc["vehicle_id"].as<String>();
    JsonArray passengers = doc["passengers"];
    int index = vehicleID.substring(1).toInt() - 1;

    if (index >= 0 && index < vehicleCount)
    {
        if (vehicles[index].status == "in_use")
        {
            doc.clear();
            doc["success"] = false;
            doc["error"] = "VEHICLE_NOT_AVAILABLE";
            doc["message"] = "VehicleNotavailable";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
        else if (passengers.size() > vehicles[index].maxCapacity)
        {
            doc.clear();
            doc["success"] = false;
            doc["error"] = "CAPACITY_EXCEEDED";
            doc["message"] = "Too many passengers";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
        else
        {
            vehicles[index].status = "in_use";
            doc.clear();
            doc["success"] = true;
            doc["drive_id"] = "D" + String(millis());
            doc["start_time"] = "2023-07-20T14:30:00Z";
            String response;
            serializeJson(doc, response);
            server.send(200, "application/json", response);
        }
    }
    else
    {
        doc.clear();
        doc["success"] = false;
        doc["error"] = "VEHICLE_NOT_FOUND";
        doc["message"] = "Vehicle not found";
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    }
}

void setup()
{
    // Initialize Serial for debugging
    Serial.begin(115200);

    // Initialize GPIOs for LEDs, Buzzer, and Buttons
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RGB_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

    // Ensure Yellow LED is off by default
    // digitalWrite(YELLOW_LED_PIN, LOW);

    // Turn on RGB LED
    // digitalWrite(RGB_LED_PIN, HIGH);

    // Beep the buzzer twice
    for (int i = 0; i < 2; i++)
    {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
    }

    // Initialize I2C
    i2c_master_init();

    // Initialize LCD
    lcd_init();

    // Run I2C Scanner
    i2c_scanner();

    // Initialize Wi-Fi
    Serial.print("Setting up Wi-Fi...");
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Wi-Fi IP Address: ");
    Serial.println(IP);

    // Set up web server
    server.on("/", handleOfIndex);
    server.on("/index.html", handleOfIndex);
    server.on("/people.html", handleOfPeople);
    server.on("/vehicles.html", handleOfVehicles);
    server.on("/drive.html", handleOfDrive);
    server.onNotFound(handleNotFound);
    server.on("/api", handleApiPost);
    server.on("/api/vehicle", handleGetVehicleList);
    server.on("/api/users", handleGetPeopleList);
    server.on("/api/drives", handleDrive);
    server.begin();
    Serial.println("Web server started");

    // Check if maxCapacity is uninitialized
    if (currentCar.maxCapacity == 0)
    {
        currentCar.maxCapacity = 1; // Set default value
    }

    // Dynamically allocate memory for occupants
    person *occupants = new person[currentCar.maxCapacity];

    // Set occupants
    for (int i = 0; i < currentCar.maxCapacity; i++)
    {
        int chosen = 0;
        occupants[i] = people[chosen];
    }
}

void loop()
{
    handle_button_press();
    server.handleClient(); // Handle incoming HTTP requests
    delay(50);             // Check button state every 50ms
    // Update the coordinates (example values)
    locus.update(12.34, 56.78, 100.0); // Pass appropriate values for latitude, longitude, and altitude
}

void something()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcdContent = "This does something";
    lcd.print(lcdContent);
}
