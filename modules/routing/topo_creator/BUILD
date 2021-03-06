load("//tools:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "edge_creator",
    srcs = [
        "edge_creator.cc",
    ],
    hdrs = [
        "edge_creator.h",
    ],
    deps = [
        "//modules/common",
        "//modules/map/proto:map_proto",
        "//modules/routing/common:routing_gflags",
        "//modules/routing/proto:routing_proto",
    ],
)

cc_library(
    name = "graph_creator",
    srcs = [
        "graph_creator.cc",
    ],
    hdrs = [
        "graph_creator.h",
    ],
    deps = [
        ":edge_creator",
        ":node_creator",
        "//modules/common",
        "//modules/common/configs:vehicle_config_helper",
        "//modules/common/util",
        "//modules/map/hdmap/adapter:opendrive_adapter",
        "//modules/map/proto:map_proto",
        "//modules/routing/proto:routing_proto",
    ],
)

cc_test(
    name = "graph_creator_test",
    srcs = [
        "graph_creator_test.cc",
    ],
    size = "small",
    deps = [
        ":graph_creator",
        "@gtest//:main",
    ],
)

cc_library(
    name = "node_creator",
    srcs = [
        "node_creator.cc",
    ],
    hdrs = [
        "node_creator.h",
    ],
    deps = [
        "//modules/common",
        "//modules/map/proto:map_proto",
        "//modules/routing/common:routing_gflags",
        "//modules/routing/proto:routing_proto",
    ],
)

cc_binary(
    name = "topo_creator",
    srcs = ["topo_creator.cc"],
    deps = [
        ":graph_creator",
        "//cyber/common:log",
        "//external:gflags",
        "//modules/common",
        "//modules/common/configs:config_gflags",
        "//modules/common/status",
        "//modules/common/util",
        "//modules/map/hdmap:hdmap_util",
        "//modules/map/proto:map_proto",
        "//modules/routing/common:routing_gflags",
        "//modules/routing/proto:routing_proto",
    ],
)

cpplint()
