project(pgld)
cmake_minimum_required(VERSION 2.6)

add_subdirectory(src)

INSTALL(FILES logrotate DESTINATION /etc/logrotate.d RENAME pgld)


