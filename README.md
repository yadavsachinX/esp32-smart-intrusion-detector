# Smart Intrusion Detector with ESP32-CAM

This project uses the ESP32-CAM to detect intrusions using a PIR sensor and a reed switch, and reports events to Blynk IoT. It also includes video streaming, image capture, and Blynk dashboard integration.

## Features
- Intrusion detection with PIR sensor and reed switch
- Real-time photo capture and video stream via local server
- Event-based alerts to Blynk IoT (Intrusion, Reed Open, Motion Only, Armed/Disarmed)
- Manual control of system status and image capture from Blynk app
- Fake image feedback with terminal-style logs

## Hardware Used
- ESP32-CAM (AI Thinker)
- PIR Sensor
- Reed Switch
- Buzzer
- LEDs

## Blynk Integration
- Events: intrusion, system_armed, system_disarmed, reed_open, motion_only
- Datastreams: system_control (V1), intrusion_log (V3), photo_button (V5), photo_feedback (V2), video_stream (V4)

## Local Web Server Routes
- `/` - Main interface
- `/stream` - Live camera feed
- `/capture` - Snapshot capture
- `/list` - List captured images
- `/delete?name=...` - Delete image

## Screenshots
*(Add screenshots of Blynk dashboard and live stream if possible)*
