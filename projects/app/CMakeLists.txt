cmake_minimum_required(VERSION 3.7.2)

project(app C) # create a new C project called 'Hello' 


add_executable(app src/main.c) # add files to our project. Paths are relative to this file.

target_link_libraries(app sel4muslcsys  muslc) # we need to link against the standard C lib for printf


set_property(GLOBAL APPEND PROPERTY apps_property "$<TARGET_FILE:app>")



