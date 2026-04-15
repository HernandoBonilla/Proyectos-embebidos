# Sistema Distribuido de Dos Nodos

## Descripción general

Este proyecto implementa un sistema de monitoreo distribuido basado en dos nodos ESP32:

- **Nodo 1:** nodo central con pantalla LCD, RTC, sensor de pulso e IMU local.
- **Nodo 2:** nodo portátil ubicado en el pecho con sensor MPU6050.

## Objetivo

Adquirir datos fisiológicos y de movimiento, transmitir la información entre nodos usando **ESP-NOW** y consolidar el registro en un computador.

## Arquitectura

Nodo 2 -> ESP-NOW -> Nodo 1 -> LCD + PC

## Estados de movimiento

- QUIETO
- DE_PIE
- ACOSTADO
- CAMINANDO
- CAIDA
- INDETERMINADO

## Módulos principales

### Nodo 1
- `main.c`
- `app_data.h`
- `common_packet.h`
- `display/`
- `pulse/`
- `imu/`
- `rtc/`
- `i2c_bus/`

### Nodo 2
- `main.c`
- `app_data.h`
- `common_packet.h`
- `i2c_bus.c`
- `mpu6050.c`
- `motion_detector.c`
