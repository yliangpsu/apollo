load("//tools:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "time",
    srcs = [
        "time.cc",
    ],
    hdrs = [
        "time.h",
    ],
    deps = [
        "duration",
        "//cyber/common:common",
    ],
)

cc_test(
    name = "time_test",
    size = "small",
    srcs = [
        "time_test.cc",
    ],
    deps = [
        "//cyber:cyber_core",
        "@gtest//:main",
    ],
)

cc_library(
    name = "rate",
    srcs = [
        "rate.cc",
    ],
    hdrs = [
        "rate.h",
    ],
    deps = [
        "time",
        "duration",
        "//cyber/common:common",
    ],
)

cc_library(
    name = "duration",
    srcs = [
        "duration.cc",
    ],
    hdrs = [
        "duration.h",
    ],
    deps = [
        "//cyber/common:common",
    ],
)

cc_test(
    name = "duration_test",
    size = "small",
    srcs = [
        "duration_test.cc",
    ],
    deps = [
        "//cyber:cyber_core",
        "@gtest//:main",
    ],
)

cpplint()
