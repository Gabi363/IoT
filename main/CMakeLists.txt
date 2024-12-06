file(GLOB_RECURSE srcs "main.c" "src/*.c")
idf_component_register(SRCS "mqtt_client.c" "${srcs}"
                       INCLUDE_DIRS "./include")