#inject boardname https://community.platformio.org/t/inject-board-name-into-code/21526/2

Import("env")

#env.Append(CPPDEFINES=[("CONFIG_SDKCONFIG_DEFAULTS", "sdkconfig.defaults")])

env.Append(CPPDEFINES=[("USER_SETUP_LOADED", 1)])
env.Append(CPPDEFINES=[("ILI9341_2_DRIVER", 1)])
env.Append(CPPDEFINES=[("SPI_FREQUENCY", 48000000)])
env.Append(CPPDEFINES=[("SUPPORT_TRANSACTIONS", 1)])
env.Append(CPPDEFINES=[("USE_LITTLEFS", 1)])

env.Append(CPPDEFINES=[("TFT_CS", 4)])
env.Append(CPPDEFINES=[("TFT_DC", 43)])    
env.Append(CPPDEFINES=[("TFT_MISO", 8)])
env.Append(CPPDEFINES=[("TFT_MOSI", 9)])
env.Append(CPPDEFINES=[("TFT_SCLK", 7)])
#env.Append(CPPDEFINES=[("TFT_SCLK", 43)])
#env.Append(CPPDEFINES=[("USE_HSPI_PORT")]) 

env.Append(CPPDEFINES=[("TFT_RST", 44)])
#env.Append(CPPDEFINES=[("TFT_RST", -1)])
