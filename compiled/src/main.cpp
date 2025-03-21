#include <Wire.h>
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <iostream>
using namespace std;
// Setup variables
int picked = 0;

struct vehicle
{
    String brand;
    String model;
    float weight;
    int LPlate;
    int maxCapacity;
};

struct person
{
    float weight;
    float height;
    int age;
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
const char *password = "GitoutGitoutGitoutofmyhead";

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
    bool leftButtonPressed = digitalRead(LEFT_BUTTON_PIN) == LOW;
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
        
                /* 输入框样式 */
                .input-field {
                    flex: 1;
                    padding: 8px 12px;
                    border: 1px solid #ddd;
                    border-radius: 4px;
                    font-size: 14px;
                }
        
                /* 按钮样式 */
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
                    })
                    if (jData.statuses == "Timeout") {
                        document.getElementById('OLLable').textContent = "Online Vehicle: Timeout";
                        throw new Error('Network response was not ok');
                    }
                    else if (jData.statuses == "Fail") {
                        document.getElementById('OLLable').textContent = "Online Vehicle: Err";
                    } else {
                        document.getElementById('OLLable').textContent = `Online Vehicle: ${jData.Data}`;
                    }
                }
        
                async function GetUserName() {
                    const jData = await InvokeRequest({
                        cmd: 'GetUserName'
                    })
                    if (jData.statuses == "Timeout") {
                        document.getElementById('UName').textContent = "User Name: Timeout";
                        throw new Error('Network response was not ok');
                    } else if (jData.statuses == "Fail") {
                        document.getElementById('UName').textContent = "User Name: Err";
                    } else {
                        document.getElementById('UName').textContent = `User Name: ${jData.Data}`;
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
                        var rJson = {
                            statuses: ""
                        };
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
    server.send(200, "text/html", html.c_str());
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
        
                /* 输入框样式 */
                .input-field {
                    flex: 1;
                    padding: 8px 12px;
                    border: 1px solid #ddd;
                    border-radius: 4px;
                    font-size: 14px;
                }
        
                /* 按钮样式 */
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
                // 添加列表项函数（修改后）
                async function addListItem(text = -1) {
                    const input = document.getElementById('itemInput');
                    const listContainer = document.getElementById('listContent');
                    let content = text === -1 ? input.value.trim() : text;
        
                    // 输入验证
                    if (content === "") {
                        alert('Please input valid data');
                        input.focus();
                        return false;
                    }
        
                    try {
                        // 先进行服务端添加
                        const addSuccess = await AddPeople(content);
                        if (!addSuccess) return false;
        
                        // 更新UI
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
        
                // 删除行函数（优化后）
                async function deleteListItem(button) {
                    const item = button.closest('.list-item');
                    const content = item.querySelector('span').textContent;
        
                    try {
                        // 先执行服务端删除
                        const deleteSuccess = await DelPeople(content);
                        if (!deleteSuccess) return;
        
                        // 更新UI
                        item.remove();
                        const listContainer = document.getElementById('listContent');
                        if (listContainer.children.length === 0) {
                            listContainer.innerHTML = '<div class="empty-state">No item yet</div>';
                        }
                    } catch (error) {
                        console.error('Delete item failed:', error);
                    }
                }
        
                async function GetListOfPeople() {
                    try {
                        const rJson = await InvokeRequest({ cmd: "GetListOfPeople" });
        
                        if (rJson.statuses === "Success") {
                            const listContainer = document.getElementById('listContent');
                            listContainer.innerHTML = '';
        
                            if (!rJson.data || rJson.data === "") {
                                listContainer.innerHTML = '<div class="empty-state">No item yet</div>';
                                return;
                            }
        
                            const TArray = rJson.data.split("|");
                            for (const x of TArray) {
                                if (x.trim()) {
                                    await addListItem(x.trim());
                                }
                            }
                        } else {
                            handleError(rJson.statuses, 'get List Of People');
                        }
                    } catch (error) {
                        console.error('Get list failed:', error);
                    }
                }
        
                async function AddPeople(content) {
                    try {
                        const rJson = await InvokeRequest({
                            cmd: "AddPeople",
                            Name: content
                        });
        
                        if (rJson.statuses === "Success") {
                            return true;
                        }
                        handleError(rJson.statuses, 'add people');
                        return false;
                    } catch (error) {
                        console.error('Add failed:', error);
                        return false;
                    }
                }
        
                async function DelPeople(content) {
                    try {
                        const rJson = await InvokeRequest({
                            cmd: "DelPeople",
                            Name: content
                        });
        
                        if (rJson.statuses === "Success") {
                            return true;
                        }
                        handleError(rJson.statuses, 'delete people');
                        return false;
                    } catch (error) {
                        console.error('Delete failed:', error);
                        return false;
                    }
                }
        
                function handleError(status, operation) {
                    const messageMap = {
                        Timeout: `Operation timeout: ${operation}`,
                        Fail: `Operation failed: ${operation}`,
                        default: `Unknown error: ${operation}`
                    };
        
                    alert(messageMap[status] || messageMap.default);
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
    server.send(200, "text/html", html.c_str());
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
        
                /* 输入框样式 */
                .input-field {
                    flex: 1;
                    padding: 8px 12px;
                    border: 1px solid #ddd;
                    border-radius: 4px;
                    font-size: 14px;
                }
        
                /* 按钮样式 */
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
        
            <!-- 在原有HTML的.page div内添加以下内容 -->
            <div class="page active">
                <h1>DashBoard Of Vehicles</h1>
        
                <!-- 车辆状态显示 -->
                <div id="vehicleList" class="vehicle-list">
                    <!-- 动态生成的车辆卡片 -->
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
        
                <!-- 操作表单区域 -->
                <h2>Vehicle Operations</h2>
        
                <!-- Change Brand -->
                <div class="input-group">
                    <input type="text" class="input-field" id="changeBrandId" placeholder="Vehicle ID">
                    <input type="text" class="input-field" id="changeBrandValue" placeholder="New Brand">
                    <button class="submit-btn" onclick="handleChangeBrand()">Change Brand</button>
                </div>
        
                <!-- Change Model -->
                <div class="input-group">
                    <input type="text" class="input-field" id="changeModelId" placeholder="Vehicle ID">
                    <input type="text" class="input-field" id="changeModelValue" placeholder="New Model">
                    <button class="submit-btn" onclick="handleChangeModel()">Change Model</button>
                </div>
        
                <!-- Change Weight -->
                <div class="input-group">
                    <input type="text" class="input-field" id="changeWeightId" placeholder="Vehicle ID">
                    <input type="number" class="input-field" id="changeWeightValue" placeholder="New Weight (kg)">
                    <button class="submit-btn" onclick="handleChangeWeight()">Change Weight</button>
                </div>
        
                <!-- Change License Plate -->
                <div class="input-group">
                    <input type="text" class="input-field" id="changeLicenseId" placeholder="Vehicle ID">
                    <input type="text" class="input-field" id="changeLicenseValue" placeholder="New License Plate">
                    <button class="submit-btn" onclick="handleChangeLicense()">Change License</button>
                </div>
        
                <!-- Change Max Capacity -->
                <div class="input-group">
                    <input type="text" class="input-field" id="changeCapacityId" placeholder="Vehicle ID">
                    <input type="number" class="input-field" id="changeCapacityValue" placeholder="New Max Capacity">
                    <button class="submit-btn" onclick="handleChangeCapacity()">Change Capacity</button>
                </div>
        
                </br>
                <!-- Add Vehicle -->
                <div class="input-group" style="flex-wrap: wrap; gap: 8px;">
                    <input type="text" class="input-field" id="addVehicleBrand" placeholder="Brand">
                    <input type="text" class="input-field" id="addVehicleModel" placeholder="Model">
                    <input type="number" class="input-field" id="addVehicleWeight" placeholder="Weight (kg)">
                    <input type="text" class="input-field" id="addVehicleLicense" placeholder="License Plate">
                    <input type="number" class="input-field" id="addVehicleCapacity" placeholder="Max Capacity">
                    <button class="submit-btn" onclick="handleAddVehicle()">Add Vehicle</button>
                </div>
                </br>
                <!-- Delete Vehicle -->
                <div class="input-group">
                    <input type="text" class="input-field" id="deleteVehicleId" placeholder="Vehicle ID">
                    <button class="submit-btn" onclick="handleDeleteVehicle()">Delete Vehicle</button>
                </div>
        
                <!-- 结果展示 -->
                <div class="result-box" id="resultBox"></div>
            </div>
        
            <script>
                // 新增操作函数
                async function handleChangeBrand() {
                    const vehicleId = document.getElementById('changeBrandId').value;
                    const newValue = document.getElementById('changeBrandValue').value;
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
                    const jData = await InvokeRequest({
                        cmd: 'ChangeWeight',
                        VehicleID: vehicleId,
                        NewWeight: newValue
                    });
                    showResult(jData);
                }
        
                async function handleChangeLicense() {
                    const vehicleId = document.getElementById('changeLicenseId').value;
                    const newValue = document.getElementById('changeLicenseValue').value;
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
                    const jData = await InvokeRequest({
                        cmd: 'ChangeMaxCapacity',
                        VehicleID: vehicleId,
                        NewCapacity: newValue
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
                    const jData = await InvokeRequest({
                        cmd: 'AddVehicle',
                        Data: newVehicle
                    });
                    showResult(jData);
                }
        
                async function handleDeleteVehicle() {
                    const vehicleId = document.getElementById('deleteVehicleId').value;
                    const jData = await InvokeRequest({
                        cmd: 'DeleteVehicle',
                        VehicleID: vehicleId
                    });
                    showResult(jData);
                }
        
                // 结果展示函数
                function showResult(jData) {
                    const resultBox = document.getElementById('resultBox');
                    if (jData.statuses === "Success") {
                        resultBox.textContent = `操作成功: ${jData.message || ''}`;
                        resultBox.style.color = 'green';
                        GetVehicles(); // 刷新车辆列表
                    } else {
                        resultBox.textContent = `操作失败: ${jData.message || '未知错误'}`;
                        resultBox.style.color = 'red';
                    }
                }
        
                // 获取车辆列表
                async function GetVehicles() {
                    const jData = await InvokeRequest({ cmd: 'GetVehicles' });
                    if (jData.statuses === "Success") {
                        const container = document.getElementById('vehicleList');
                        container.innerHTML = '';
                        jData.Data.forEach(vehicle => {
                            container.innerHTML += `
                            <div class="status-card">
                                <h3>${vehicle.Name}</h3>
                                <p>Type: ${vehicle.Type}</p>
                                <p>Location: ${vehicle.Location}</p>
                                <p>Brand: ${vehicle.Brand}</p>
                                <p>Model: ${vehicle.Model}</p>
                                <p>Weight: ${vehicle.Weight}kg</p>
                                <p>License: ${vehicle.LicensePlate}</p>
                                <p>Capacity: ${vehicle.MaxCapacity}人</p>
                            </div>
                        `;
                        });
                    }
                }
        
                // 初始化时获取车辆列表
                document.addEventListener('DOMContentLoaded', function () {
                    GetVehicles();
                });
            </script>
        </body>
        
        </html>
        )html_delimiter";
    server.send(200, "text/html", html.c_str());
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
                <!-- 步骤1: 车辆选择 -->
                <div id="step1" class="step-active step-container">
                    <h2>choose vehicle (remain seat: <span id="remainingSeats">-</span>)</h2>
                    <div id="vehicleList">
                        <div class="loader"></div>
                    </div>
                </div>
        
                <!-- 步骤2: 乘客管理 -->
                <div id="step2" class="step-container">
                    <div id="selectedVehicleInfo">
                        <h3>Chosen Vehicle: <span id="selectedVehicleName">-</span></h3>
                        <p>license plate: <span id="selectedVehicleLicense">-</span></p>
                        <p>Max People count: <span id="selectedVehicleCapacity">-</span> </p>
                    </div>
        
                    <h2>Add People</h2>
                    <div class="input-group">
                        <select class="input-field" id="existingUsers">
                            <option value="">Loading...</option>
                        </select>
                        <button class="submit-btn" onclick="addExistingUser()">Add People</button>
                    </div>
        
                    <div class="input-group">
                        <input type="text" class="input-field" id="customUserName" placeholder="自定义用户名称">
                        <button class="submit-btn" onclick="addCustomUser()">Add Customize People</button>
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
                // 系统状态
                const state = {
                    selectedVehicle: null,
                    passengers: [],
                    vehicles: [],
                    users: []
                };
        
                // API 配置
                const API_ENDPOINTS = {
                    VEHICLES: '/api/vehicles',
                    USERS: '/api/users',
                    DRIVES: '/api/drives'
                };
        
                // 初始化页面
                document.addEventListener('DOMContentLoaded', async () => {
                    try {
                        await Promise.all([fetchVehicles(), fetchUsers()]);
                        renderVehicles();
                        renderUserOptions();
                    } catch (error) {
                        showMessage('Fail to load data，pls try again', 'red');
                    }
                });
        
                // API 请求方法
                async function apiRequest(url, method = 'GET', body = null) {
                    const headers = { 'Content-Type': 'application/json' };
                    const config = { method, headers };
                    if (body) config.body = JSON.stringify(body);
        
                    try {
                        const response = await fetch(url, config);
                        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
                        return await response.json();
                    } catch (error) {
                        console.error('Fail to fetch API:', error);
                        throw error;
                    }
                }
        
                // 获取车辆数据
                async function fetchVehicles() {
                    const data = await apiRequest(API_ENDPOINTS.VEHICLES);
                    state.vehicles = data.map(v => ({
                        id: v.id,
                        name: v.name,
                        license: v.license_plate,
                        capacity: v.max_capacity
                    }));
                }
        
                // 获取用户数据
                async function fetchUsers() {
                    const data = await apiRequest(API_ENDPOINTS.USERS);
                    state.users = data.map(u => ({
                        id: u.id,
                        name: u.full_name
                    }));
                }
        
                // 渲染车辆列表
                function renderVehicles() {
                    const container = document.getElementById('vehicleList');
                    container.innerHTML = state.vehicles.map(vehicle => `
                <div class="status-card" onclick="selectVehicle('${vehicle.id}')">
                    <h3>${vehicle.name}</h3>
                    <p>License: ${vehicle.license}</p>
                    <p>Max People: ${vehicle.capacity} 人</p>
                </div>
            `).join('');
                }
        
                // 选择车辆
                function selectVehicle(vehicleId) {
                    state.selectedVehicle = state.vehicles.find(v => v.id === vehicleId);
                    state.passengers = [];
        
                    document.getElementById('selectedVehicleName').textContent = state.selectedVehicle.name;
                    document.getElementById('selectedVehicleLicense').textContent = state.selectedVehicle.license;
                    document.getElementById('selectedVehicleCapacity').textContent = state.selectedVehicle.capacity;
        
                    document.getElementById('step1').classList.remove('step-active');
                    document.getElementById('step2').classList.add('step-active');
                    updateUI();
                }
        
                // 渲染用户选项
                function renderUserOptions() {
                    const select = document.getElementById('existingUsers');
                    select.innerHTML = '<option value="">Choose People</option>' +
                        state.users.map(user => `
                    <option value="${user.id}">${user.name}</option>
                `).join('');
                }
        
                // 添加乘客相关方法保持不变
                // (保持原有的 addExistingUser, addCustomUser, addEmptySeat, addPassenger, removePassenger 等方法)
        
                // 开始行程（修改为API提交）
                async function startDrive() {
                    if (state.passengers.length === 0) {
                        showMessage('Err: Pls add unless one people/free seat', 'red');
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
                            showMessage('The trip has started successfully! Departure time: ' + new Date().toLocaleString(), 'green');
                            resetSelection();
                        } else {
                            showMessage('Fail to start: ' + (response.error || 'Unexpected err'), 'red');
                        }
                    } catch (error) {
                        showMessage('Fail to connect to server', 'red');
                    }
                }
        
                // 其他辅助方法保持不变
                // (保持原有的 updateUI, resetSelection, showMessage 等方法)
            </script>
        </body>
        
        </html>
        )html_delimiter";
    server.send(200, "text/html", html.c_str());
    cout << "User accessed drive.html" << endl;
}

void handleNotFound()
{
    server.send(404, "text/plain", "404: Not Found - The requested URL was not found on this server.");
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
    digitalWrite(YELLOW_LED_PIN, LOW);

    // Turn on RGB LED
    digitalWrite(RGB_LED_PIN, HIGH);

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