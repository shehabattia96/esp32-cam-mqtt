# assumes you have Platform IO installed
pio platform install espressif32 --with-package framework-espidf --force # this takes a few minutes to install
pio lib install "espressif/esp32-camera@^1.0.0"