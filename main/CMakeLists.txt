
file(GLOB SRCS
    "adc/*.cpp"
    "ble/*.cpp"
    "boards/*.cpp"
    "console/*.cpp"
    "context/*.cpp"
    "conversion/*.cpp"
    "led/*.c"
    "tasks/*.cpp"
    "./*.cpp"
)

set(INCLUDE_DIRS
    "."
    "adc"
    "ble"
    "boards"
    "context"
    "tasks"
)


idf_component_register(SRCS ${SRCS}
                    INCLUDE_DIRS ${INCLUDE_DIRS})
