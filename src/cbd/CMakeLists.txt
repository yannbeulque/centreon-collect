add_definitions(-DCENTREON_BROKER_VERSION=\"${COLLECT_VERSION}\")

# Standalone binary.
add_executable(cbd
  ${CMAKE_SOURCE_DIR}/src/cbd/main.cc
  ${CMAKE_SOURCE_DIR}/src/cbd/config/applier/init.cc
  ${CMAKE_SOURCE_DIR}/src/cbd/config/applier/logger.cc
  ${CMAKE_SOURCE_DIR}/src/cbd/broker_impl.cc
  ${CMAKE_SOURCE_DIR}/src/cbd/brokerrpc.cc)

#Flags needed to include all symbols in binary.
target_link_libraries(cbd CONAN_PKG::grpc CONAN_PKG::protobuf CONAN_PKG::protoc_installer
  "-Wl,--whole-archive" ccb_rpc ccb_core ccb_sql "-Wl,--no-whole-archive" CONAN_PKG::json11 CONAN_PKG::spdlog CONAN_PKG::fmt CONAN_PKG::openssl)