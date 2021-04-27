# ESP32-CAM-MQTT

## High level explanation of the source code

1. Initialize camera and start capturing camera stream. If no camera, then skip steps 2 and 5. Event 'No camera' is emitted.
2. Write it to local storage. If no storage devices, then skip storing. Event 'No storage' is emitted. If storage full, overwrite.
3. Attempt to connect to wifi using SSID(`wifiSsid`) and password(`wifiPassword`) in [./.environment_variables.h](./.environment_variables.h). If failed, skip connecting to wifi and skip step 4.
4. Attempt to connect to MQTT server using Host, port and clientID in [./.environment_variables.h](./.environment_variables.h). If failed, skip connecting to mqtt server.
5. Publish camera stream to mqtt server.
