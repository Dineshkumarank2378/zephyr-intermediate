cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(temp_alarm_test)

target_include_directories(app PRIVATE
    include
)

target_sources(app PRIVATE
    src/main.c
    src/temp_alarm.c
    src/sensor_fake.c
    src/test_temp_alarm.c
)