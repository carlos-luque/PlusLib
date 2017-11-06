CMAKE_MINIMUM_REQUIRED(VERSION 3.3.0)

PROJECT(SeekCameraIDE)

SET(SeekCameraInterface_SRCS
  include/seek.h
  include/SeekCam.h
  include/SeekDevice.h
  include/SeekLogging.h
  include/SeekThermal.h
  include/SeekThermalPro.h
)

ADD_CUSTOM_TARGET(SeekCameraIDE SOURCES ${SeekCameraInterface_SRCS})
SET_PROPERTY(TARGET SeekCameraIDE PROPERTY FOLDER Interfaces)