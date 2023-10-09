const std = @import("std");

pub fn build(b: *std.Build) !void {
    const dir = "source/";
    const opt_level = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});
    
    const use_system_libs = false;
    
    const exe = b.addExecutable(.{
        .name = "endless-sky",
        .target = target,
        .optimize = opt_level,
    });
    exe.linkLibCpp();
    exe.strip = (opt_level != .Debug) and (opt_level != .ReleaseSafe);
    exe.addIncludePath(.{
        .cwd_relative = dir,
    });
    exe.addCSourceFile(.{
        .file = .{
            .cwd_relative = dir ++ "main.cpp",
        },
        .flags = &[_][]const u8 {
            "-std=c++17",
        },
    });
    
    if (!use_system_libs) {
        b.vcpkg_root = .{.found = try std.fs.path.join(b.allocator, &.{
            try std.process.getCwdAlloc(b.allocator),
            "vcpkg",
        })};
        try std.io.getStdOut().writeAll(
            b.exec(&.{
                try std.fs.path.join(b.allocator, &.{
                    b.vcpkg_root.found,
                    "vcpkg",
                }),
                "install",
            })
        );
        exe.addVcpkgPaths(.dynamic) catch return error.VcpkgNotFound;
        if (exe.vcpkg_bin_path) |path| {
            const sdl_path = try std.fs.path.join(
                b.allocator,
                &.{
                    path,
                    "SDL2.dll"
                },
            );
            if(target.isWindows()) {
                b.installBinFile(sdl_path, "SDL2.dll");   
            } else {
                b.installBinFile(sdl_path, "SDL2.so");   
            }
        }
    }
    exe.linkSystemLibrary("SDL2");
    
    b.default_step.dependOn(&exe.step);
}
