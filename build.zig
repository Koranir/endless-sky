const std = @import("std");

const sources = @import("build_sources.zig");

pub fn build(b: *std.Build) !void {
    _ = b;
    // _ = b.exec(&.{
    //     "git", "clone", "https://github.com/libsdl-org/SDL.git", "build/sdl2",
    // });

    // const opt = b.standardOptimizeOption(.{});
    // const tgt = b.standardTargetOptions(.{});

    // try buildSdl2(b);

    // const flags: []const []const u8 = &.{
    //     "-std=c++14",
    //     "-Wall",
    //     "-Werror",
    // };

    // const exe = b.addExecutable(.{
    //     .name = "Endless-Sky",
    //     .optimize = opt,
    //     .target = tgt,
    // });

    // exe.addCSourceFile(.{ .file = .{ .cwd_relative = "source/main.cpp" }, .flags = flags });
    // exe.linkLibCpp();
    // b.default_step.dependOn(&exe.step);
    // b.installArtifact(exe);

    // for (sources.lib) |source| {
    //     const lib = b.addStaticLibrary(.{
    //         .name = std.fs.path.basename(source),
    //         .optimize = opt,
    //         .target = tgt,
    //     });
    //     lib.addCSourceFile(.{ .file = .{ .cwd_relative = source }, .flags = flags });
    //     lib.linkLibCpp();
    //     exe.step.dependOn(&lib.step);
    //     exe.linkLibrary(lib);
    // }

    // const run = b.addRunArtifact(exe);
    // _ = run;
}

pub fn buildSdl2(b: *std.Build, tgt: std.Zig.CrossTarget) !void {
    _ = tgt;
    // const sdl2 = b.addStaticLibrary(.{
    //     .name = "sdl2",
    //     .optimize = .ReleaseFast,
    //     .target = tgt,
    // });

    b.exec(&.{
        "git", "clone", "https://github.com/libsdl-org/SDL.git", "build/sdl2",
    });
}
