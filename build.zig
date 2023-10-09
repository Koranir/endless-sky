const std = @import("std");

fn doVcpkg(
    b: *std.Build,
    git: []const u8,
    cwd: []const u8,
) !void {
    // TODO: Fix Lol
    const vcpkg_github_url = "https://github.com/Microsoft/vcpkg";
    const vcpkg_clone_into = try std.fs.path.join(b.allocator, &.{
        cwd,
        "vcpkg",
    });
    std.log.info("Vcpkg Clone Dir: {s}", .{vcpkg_clone_into});

    const git_path = try std.fs.path.join(b.allocator, &.{
        vcpkg_clone_into,
        ".git",
    });
    var vcpkg_needs_install = false;
    std.fs.accessAbsolute(git_path, .{}) catch |err| {
        if (err == error.FileNotFound) {
            vcpkg_needs_install = true;
        } else {
            std.log.err("Unexpected filesystem error while accessing {s} : {!}", .{ git_path, err });
            return err;
        }
    };
    std.log.debug("Vcpkg Needs Install: {}", .{vcpkg_needs_install});

    if (vcpkg_needs_install) {
        std.log.info("Cloning Vcpkg into {s}", .{vcpkg_clone_into});
        const vcpkg_result = b.exec(&.{
            git,
            "clone",
            "--quiet",
            vcpkg_github_url,
            vcpkg_clone_into,
        });
        std.log.info("Git: {s}", .{vcpkg_result});
    }

    const res = try std.process.Child.exec(.{
        .allocator = b.allocator,
        .argv = &.{
            git,
            "rev-parse",
            "HEAD",
        },
        .cwd = vcpkg_clone_into,
    });
    const baseline_sha: []const u8 = res.stdout;
    std.log.info("Vcpkg HEAD SHA: {s}", .{baseline_sha});
    const vcpkg_config_json_file = try std.fs.openFileAbsolute(try std.fs.path.join(b.allocator, &.{
        cwd,
        "vcpkg-configuration.json",
    }), .{ .mode = std.fs.File.OpenMode.read_only });
    defer vcpkg_config_json_file.close();
    const max_bytes = std.math.maxInt(usize);
    const vcpkg_config_json = try vcpkg_config_json_file.readToEndAlloc(b.allocator, max_bytes);

    const parsed_json = try std.json.parseFromSliceLeaky(struct {
        @"default-registry": struct {
            kind: []const u8,
            baseline: []const u8,
        },
        @"overlay-triplets": []const []const u8,
    }, b.allocator, vcpkg_config_json, .{});
    if (std.mem.eql(u8, baseline_sha, parsed_json.@"default-registry".baseline)) {
        std.log.info("Vcpkg Up-to-Date", .{});
    } else {
        std.log.info("Fetching changes from Vcpkg upstream", .{});
        const fetch_res = try std.process.Child.exec(.{
            .allocator = b.allocator,
            .argv = &.{
                git,
                "fetch",
                "--quiet",
                "--prune",
            },
            .cwd = vcpkg_clone_into,
        });
        std.log.info("Git: {s}", .{fetch_res.stdout});   
    }
    
    const vcpkg_exe_path = try std.fs.path.join(b.allocator, &.{
        vcpkg_clone_into,
        if (b.host.target.os.tag == std.Target.Os.Tag.windows) "vcpkg.exe" else "vcpkg",
    });
    var vcpkg_needs_bootstrap = false;
    std.fs.accessAbsolute(vcpkg_exe_path, .{}) catch |err| {
        if (err == error.FileNotFound) {
            vcpkg_needs_bootstrap = true;
        } else {
            std.log.err("Unexpected filesystem error while accessing {s} : {!}", .{ git_path, err });
            return err;
        }
    };
    
    if (vcpkg_needs_bootstrap) {
        std.log.info("Bootstrap-Vcpkg: {s}", .{b.exec(&.{
            try std.fs.path.join(b.allocator, &.{
                vcpkg_clone_into,
                "bootstrap-vcpkg",
            }),
        })});
    }
    
    std.log.info("Vcpkg: {s}", .{b.exec(&.{
        vcpkg_exe_path,
        "install",
    })});
    
    return;
}

pub fn build(b: *std.Build) !void {
    try doVcpkg(b, "git", try std.process.getCwdAlloc(b.allocator));
    
    
    // const dir = "source/";
    // const opt_level = b.standardOptimizeOption(.{});
    // const target = b.standardTargetOptions(.{});

    // const use_system_libs = false;

    // const exe = b.addExecutable(.{
    //     .name = "endless-sky",
    //     .target = target,
    //     .optimize = opt_level,
    // });
    // exe.linkLibCpp();
    // exe.strip = (opt_level != .Debug) and (opt_level != .ReleaseSafe);
    // exe.addIncludePath(.{
    //     .cwd_relative = dir,
    // });
    // exe.addCSourceFile(.{
    //     .file = .{
    //         .cwd_relative = dir ++ "main.cpp",
    //     },
    //     .flags = &[_][]const u8{
    //         "-std=c++17",
    //     },
    // });

    // if (!use_system_libs) {
    //     b.vcpkg_root = .{ .found = try std.fs.path.join(b.allocator, &.{
    //         try std.process.getCwdAlloc(b.allocator),
    //         "vcpkg",
    //     }) };
    //     try std.io.getStdOut().writeAll(b.exec(&.{
    //         try std.fs.path.join(b.allocator, &.{
    //             b.vcpkg_root.found,
    //             "vcpkg",
    //         }),
    //         "install",
    //     }));
    //     exe.addVcpkgPaths(.dynamic) catch return error.VcpkgNotFound;
    //     if (exe.vcpkg_bin_path) |path| {
    //         const sdl_path = try std.fs.path.join(
    //             b.allocator,
    //             &.{ path, "SDL2.dll" },
    //         );
    //         if (target.isWindows()) {
    //             b.installBinFile(sdl_path, "SDL2.dll");
    //         } else {
    //             b.installBinFile(sdl_path, "SDL2.so");
    //         }
    //     }
    // }
    // exe.linkSystemLibrary("SDL2");

    // b.default_step.dependOn(&exe.step);
}
