# saniBot

SaniBot is an IoT-based sanitization robot that combines UV-C and mist-based disinfection, ultrasonic sensors obstacle avoidance and Edge AI-powered human detection. It is controlled wirelessly through a built-in web page hosted on an ESP32 with live video streaming and human-detection through ESP32-CAM module over a UART bridge.

SaniBot automates the process using a dual sanitization mode means use of UV-C light and mist spray both while ensuring user safety through real-time monitoring and human detection that automatically disables UV-C exposure whenever a person is nearby.

# Features

Dual Sanitization System — combines UV-C light sterilization with mist-based disinfectant spraying for broader microbial coverage.
Edge AI Human Detection — uses of COCO-SSD model running locally on ESP32-CAM to detect human presence in real time without cloud                                        dependency.
Automatic UV-C Safety Shutdown — instantly disables the UV-C light through a relay module whenever a human is detected.
Switchable Operating Modes — toggle between AUTO mode (sensor-based autonomous movement) and MANUAL mode (user-controlled navigation) from                                the web page. 
Obstacle Detection & Avoidance — front and side-mounted ultrasonic sensors continuously scan for obstacles detection.
Wireless Web page — ESP32 control panel accessible through Wi-Fi for movement control, sanitization toggles and live status monitoring.
Live Video Streaming — ESP32-CAM streams real-time video to the web page for visual monitoring of the environment.
Object Detection Overlay — live bounding-box visualization with class labels and confidence scores rendered directly on the camera feed.


# Hardware Components

ESP32 (main controller)
ESP32-CAM (human detection + video streaming)
Ultrasonic sensors (HC-SR04) ×2
UV-C light module (254 nm germicidal lamp)
Mist spray unit
L298N motor driver
DC motors
Dual-channel relay module
Rechargeable lithium-ion battery pack

# Software Stack

Arduino IDE (C++ firmware for ESP32 and ESP32-CAM)
TensorFlow and COCO-SSD (Edge AI human/object detection)
HTML, CSS, JavaScript (web page)
UART serial protocol (inter-controller communication)

# How It Works

The robot powers on and scans its environment using the ultrasonic sensors and ESP32-CAM.
If a human is detected, the UV-C lamp is automatically switched off via the relay module.
If the area is clear, UV-C and mist sanitization continue normally.
Obstacles trigger automatic direction changes during AUTO mode.
Users can monitor live video, sensor readings, system status, and switch to MANUAL mode to drive the robot directly from the web dashboard.
