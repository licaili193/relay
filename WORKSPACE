workspace(name = "relay")

# Import the rule that can download tarballs using HTTP.
load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)

load(
    "@bazel_tools//tools/build_defs/repo:git.bzl",
    "git_repository",
)

git_repository(
    name = "ffmpeg",
    remote = "https://github.com/jonnrb/bazel_ffmpeg.git",
    commit = "01e1d136f85af8be10453224dcf7445f0d1bcabd",
    patch_cmds = [
        "./make_converter/gen.sh",
    ],
)