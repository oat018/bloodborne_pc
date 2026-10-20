# bbport online module: libbbnet.so, the PS4 network and PSN libraries with the shadNet client
# (shadp2p, a shadPS4 fork, GPL-2.0), loaded by the runtime only for online play
# (src/runtime_net.c). Its headers (include/) extend the port's shim for these libraries alone;
# everything in it is hidden and bound to itself (-Bsymbolic) except bbnet.h's two calls, so its
# copies of shadPS4's kernel shims and symbol table never meet libbbgpu's.
# Needs protobuf, nlohmann_json, OpenSSL, zlib and miniupnpc (shell.nix); without them the port
# builds without it and plays offline (gpu/CMakeLists.txt: BB_NET).

set(SHADNET_PROTO_OUT "${CMAKE_CURRENT_BINARY_DIR}/shadnet_proto_gen")
file(MAKE_DIRECTORY "${SHADNET_PROTO_OUT}")
add_custom_command(
    OUTPUT "${SHADNET_PROTO_OUT}/shadnet.pb.cc" "${SHADNET_PROTO_OUT}/shadnet.pb.h"
    COMMAND protobuf::protoc "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}/shadps4/shadnet"
            "--cpp_out=${SHADNET_PROTO_OUT}" "${CMAKE_CURRENT_SOURCE_DIR}/shadps4/shadnet/shadnet.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shadps4/shadnet/shadnet.proto")
file(GLOB_RECURSE BBNET_SOURCES CONFIGURE_DEPENDS shadps4/core/libraries/network/*.cpp
    shadps4/core/libraries/np/*.cpp shadps4/shadnet/*.cpp)
# BBSC, Bloodborne seamless co-op (GPL-2.0+): its game code and the
# port's stand-ins for shadPS4's UI (bbsc_glue.cpp). Installed only for Bloodborne with BB_BBSC=1.
list(APPEND BBNET_SOURCES shadps4/core/bloodborne_re.cpp shadps4/core/bbsc_persistence.cpp
    shadps4/core/bbsc_memscan.cpp shadps4/core/bbsc_cinematic.cpp shadps4/core/bbsc_party.cpp
    shadps4/core/bbsc_party_journal.cpp bbsc_glue.cpp)
# Trophies and the system dialogs stay in the C runtime; their shadPS4 versions need the
# emulator's UI layers.
list(FILTER BBNET_SOURCES EXCLUDE REGEX
    "np_trophy|trophy_ui|np_sns_facebook_dialog|np_profile_dialog/|np_commerce/")

add_library(bbnet SHARED ${BBNET_SOURCES} "${SHADNET_PROTO_OUT}/shadnet.pb.cc" bbnet_glue.cpp)
# The module's own headers first, then the port's utilities (common/, logging) and shim.
target_include_directories(bbnet PRIVATE include shadps4 shadps4/shadnet "${SHADNET_PROTO_OUT}"
    ${CMAKE_CURRENT_SOURCE_DIR} .. ../shim ../third_party/imgui ../third_party/httplib)
# The version the shadNet client reports to the server (ShadNetClient::BuildVersionString).
set(BBPORT_VERSION "0.6-pre1")
target_compile_definitions(bbnet PRIVATE BOOST_ASIO_STANDALONE NDEBUG
    CPPHTTPLIB_OPENSSL_SUPPORT CPPHTTPLIB_ZLIB_SUPPORT BBNET_PORT_VERSION="${BBPORT_VERSION}")
target_compile_options(bbnet PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden
    -Wno-deprecated-declarations -Wno-missing-field-initializers)
target_link_options(bbnet PRIVATE -Wl,-Bsymbolic -Wl,--exclude-libs,ALL)
# libbbgpu (already in the process) provides shadPS4's common code: logging, threads' names.
set_target_properties(bbnet PROPERTIES BUILD_RPATH "\$ORIGIN" INSTALL_RPATH "\$ORIGIN"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}") # beside libbbgpu.so, where the runtime looks
target_link_libraries(bbnet PRIVATE bbgpu nlohmann_json::nlohmann_json protobuf::libprotobuf
    OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB "${MINIUPNPC_LIBRARY}" Vulkan::Headers fmt::fmt
    Boost::headers magic_enum::magic_enum)
target_include_directories(bbnet PRIVATE "${MINIUPNPC_INCLUDE}")

if (NOT WIN32)
    add_executable(bbsc-party-test EXCLUDE_FROM_ALL ../../tests/test_bbsc_party.cpp
        ../../tests/test_bbsc_party_journal.cpp shadps4/core/bbsc_party.cpp
        shadps4/core/bbsc_party_journal.cpp)
    target_include_directories(bbsc-party-test PRIVATE include shadps4 ..)
    target_compile_options(bbsc-party-test PRIVATE -UNDEBUG -Wall -Wextra -Werror)
    target_link_libraries(bbsc-party-test PRIVATE ZLIB::ZLIB OpenSSL::Crypto)
    set_target_properties(bbsc-party-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

    add_executable(bbsc-join-test EXCLUDE_FROM_ALL ../../tests/test_bbsc_join.cpp
        shadps4/core/bbsc_party.cpp shadps4/core/bbsc_party_journal.cpp ../shim/bbport_guest_code_hooks.cpp ../common/decoder.cpp)
    target_include_directories(bbsc-join-test PRIVATE include shadps4 .. ../shim)
    target_compile_options(bbsc-join-test PRIVATE -UNDEBUG -ffunction-sections -fdata-sections)
    target_link_options(bbsc-join-test PRIVATE -Wl,--gc-sections)
    target_link_libraries(bbsc-join-test PRIVATE fmt::fmt pthread OpenSSL::Crypto Zydis::Zydis ZLIB::ZLIB)
    set_target_properties(bbsc-join-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

    add_executable(bbsc-party-transport-test EXCLUDE_FROM_ALL ../../tests/test_bbsc_party_transport.cpp
        shadps4/core/bbsc_party.cpp)
    target_include_directories(bbsc-party-transport-test PRIVATE include shadps4 .. ../shim)
    target_compile_options(bbsc-party-transport-test PRIVATE -UNDEBUG -ffunction-sections -fdata-sections)
    target_link_options(bbsc-party-transport-test PRIVATE -Wl,--gc-sections)
    target_link_libraries(bbsc-party-transport-test PRIVATE fmt::fmt pthread)
    set_target_properties(bbsc-party-transport-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

    add_executable(bbsc-matching2-admission-test EXCLUDE_FROM_ALL
        ../../tests/test_bbsc_matching2_admission.cpp
        shadps4/core/libraries/np/np_matching2/np_matching2_internal.cpp
        "${SHADNET_PROTO_OUT}/shadnet.pb.cc")
    target_include_directories(bbsc-matching2-admission-test PRIVATE
        include shadps4 shadps4/shadnet "${SHADNET_PROTO_OUT}" "${MINIUPNPC_INCLUDE}" .. ../shim)
    target_compile_options(bbsc-matching2-admission-test PRIVATE
        -UNDEBUG -ffunction-sections -fdata-sections -Wall -Wextra -Werror -Wno-unused-function
        -Wno-unused-parameter)
    target_link_options(bbsc-matching2-admission-test PRIVATE -Wl,--gc-sections)
    target_link_libraries(bbsc-matching2-admission-test PRIVATE fmt::fmt protobuf::libprotobuf pthread)
    set_target_properties(bbsc-matching2-admission-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

    add_executable(bbsc-party-events-test EXCLUDE_FROM_ALL ../../tests/test_bbsc_party_events.cpp
        shadps4/core/bbsc_party.cpp)
    target_include_directories(bbsc-party-events-test PRIVATE include shadps4 ..)
    target_compile_options(bbsc-party-events-test PRIVATE -UNDEBUG -Wall -Wextra -Werror)
    set_target_properties(bbsc-party-events-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

    add_executable(bbsc-http-poll-test EXCLUDE_FROM_ALL ../../tests/test_bbsc_http_poll.cpp)
    target_include_directories(bbsc-http-poll-test PRIVATE include shadps4 .. ../shim ../third_party/httplib)
    target_compile_options(bbsc-http-poll-test PRIVATE -UNDEBUG -ffunction-sections -fdata-sections)
    target_link_options(bbsc-http-poll-test PRIVATE -Wl,--gc-sections)
    target_link_libraries(bbsc-http-poll-test PRIVATE fmt::fmt pthread OpenSSL::Crypto)
    set_target_properties(bbsc-http-poll-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()
