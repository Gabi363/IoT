file(GLOB_RECURSE srcs "main.c", "src/*.c" )
idf_component_register(SRCS "main2.c" "${srcs}"
                       INCLUDE_DIRS "./include")
# file(GLOB SRCS "src/*.c", "mqtt_manager.c")

# idf_component_register(
#     SRCS ${SRCS}  # Use the collected source files
#     INCLUDE_DIRS "include"  # Specify the include directory
# )

# idf_component_register(
#     SRCS "bmx280.c"
#     INCLUDE_DIRS "./include"
#     REQUIRES driver
# )