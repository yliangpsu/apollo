load("//tools:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])


cc_proto_library(
    name = "tl_preprocess_proto",
    deps = [ 
        ":tl_preprocess_proto_lib",
    ],  
)

proto_library(
    name = "tl_preprocess_proto_lib",
    srcs = [ 
        "tl_preprocess.proto",
    ],  
)


cc_library(
    name = "multi_camera_projection",
    srcs = [
        "multi_camera_projection.cc",
    ],
    hdrs = [ 
        "multi_camera_projection.h",
    ],
    deps = [
        ":pose",
        "//cyber",
        "//modules/common/util:file_util",
        "//modules/perception/base:base",
        "//modules/perception/camera/common:common",
        "//modules/perception/common/io:io_util",
        "//modules/perception/common/sensor_manager:sensor_manager",
    ],
)


cc_library(
    name = "pose",
    srcs = [
        "pose.cc",
    ],
    hdrs = [ 
        "pose.h",
    ],
    deps = [
        "//cyber",
        "//modules/perception/base:base",
    ],
)



cc_library(
    name = "tl_preprocessor",
    srcs = [
        "tl_preprocessor.cc",
    ],
    hdrs = [ 
        "tl_preprocessor.h",
    ],
    linkopts = [
        "-lboost_thread",
        "-lboost_system",
    ],
    deps = [
        ":multi_camera_projection",
        "//cyber",
        "//external:gflags",
        "//modules/common/util:file_util",
        "//modules/perception/base:base",
        "//modules/perception/camera/common:common",
        "//modules/perception/camera/lib/interface:base_init_options",
        "//modules/perception/camera/lib/interface:interface",
        "//modules/perception/lib/config_manager:config_manager",
        "//modules/perception/lib/utils:utils",
        "@eigen",
    ],
)

cpplint()
