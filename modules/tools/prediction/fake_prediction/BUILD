load("//tools:cpplint.bzl", "cpplint")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "libfake_prediction_component.so",
    srcs = [
        "fake_prediction.cc",
    ],
    deps = [
        "//cyber",
        "//external:gflags",
        "//modules/common/adapters:adapter_gflags",
        "//modules/common/util",
        "//modules/prediction/proto:prediction_proto",
    ],
    linkshared = True,
    linkstatic = False,
)

cpplint()
