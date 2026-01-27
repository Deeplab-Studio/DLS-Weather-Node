# 🌦️ DLS Weather Node

The purpose of this project is to support the **DLS Weather** initiative by enabling the deployment of **community-driven weather nodes**.

With this service, collected weather data can be transmitted **for free to up to 5 different platforms simultaneously** through a **single API**.

Supported platforms:

- 📡 **APRS Weather Station** (for amateur radio operators)
- 🌍 **PWSWeather**
- ☁️ **WeatherCloud**
- 🌡️ **Weather Underground**
- 🌬️ **Windy.com**

---

## 🔌 ESP32 Hardware Connections

The following sensors can be easily connected to the ESP32 via **I2C**:

- **BMP280**
- **BME280**
- **BME680**

## ![Configuration](docs/images/diagram.png)

## 🔑 Getting an API Key

### 1️⃣ Step – Register

👉 [https://wx.deeplabstudio.com/register](https://wx.deeplabstudio.com/register)\
![Configuration](docs/images/img1.png)

### 2️⃣ Step – Go to the API Keys Page

👉 [https://wx.deeplabstudio.com/dashboard/api-keys](https://wx.deeplabstudio.com/dashboard/api-keys)\
![Configuration](docs/images/img2.png)

### 3️⃣ Step – Generate a New Key

Click on the `+ Generate Key` button.\
![Configuration](docs/images/img3.png)

### 4️⃣ Step – Copy the API Key

Copy and securely store your generated API key.\
![Configuration](docs/images/img4.png)

---

## ⚙️ Installing DLS Weather Node on ESP32

### 1️⃣ Step – Open DLS Weather Installer

👉 [https://deeplab-studio.github.io/DLS-Weather-Node/](https://deeplab-studio.github.io/DLS-Weather-Node/)\
![Configuration](docs/images/img5.png)

### 2️⃣ Step – Connect and Install

- Click **Connect**
- Select **Install DLSWeather**\
  ![Configuration](docs/images/img6.png)

### 3️⃣ Step – Configure Settings

After installation, configure:

- Wi-Fi credentials
- API Key
- Sensor settings\
  ![Configuration](docs/images/img7.png)

### 4️⃣ Step – Verify on Website

You can monitor your node from the web dashboard: 👉 [https://wx.deeplabstudio.com/](https://wx.deeplabstudio.com/)

---

## 🔗 Using DLS Weather API in Other Projects

The DLS Weather API is open for integration with your own projects.

### API Endpoint

```
POST https://wx-api.deeplabstudio.com/v1/ingest/weather
```

### Required Headers

```
Content-Type: application/json
x-api-key: <api_key>
```

### Example Payload

```json
{
  "stationId": "ST-E9F9AF",
  "timestamp": 1769449600,
  "location": {
    "lat": 0.0,
    "lon": 0.0
  },
  "environment": {
    "temperature": 22.5, // Optional
    "humidity": 60, // Optional
    "pressure": 1013 // Optional
  },
  "wind": {
    "speed": 5.2, // Optional
    "direction": 180 // Optional
  },
  "rain": {
    "rate": 0, // Optional
    "daily": 2.5 // Optional
  }
}
```

---

## 🤝 Contribution & Support

This is a community-driven project. Feel free to contribute, deploy nodes, or share feedback to help improve the DLS Weather ecosystem.

📡 **DLS Weather – Community Powered Weather Data Network**
