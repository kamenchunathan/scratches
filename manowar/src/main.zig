const std = @import("std");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};

    const allocator = gpa.allocator();
    const manpath_env = std.process.getEnvVarOwned(allocator, "MANPATH") catch null;

    // Run the manpath command if MANPATH is not set
    if (manpath_env == null) {
        const argv = &[_][]const u8{"manpath"};
        var manpath_cmd = std.process.Child.init(argv, allocator);
        manpath_cmd.stdin_behavior = .Ignore;
        manpath_cmd.stderr_behavior = .Pipe;
        manpath_cmd.stdout_behavior = .Pipe;

        try manpath_cmd.spawn();

        var stdout = try std.ArrayListUnmanaged(u8).initCapacity(allocator, 1024);
        defer stdout.deinit(allocator);
        var stderr = try std.ArrayListUnmanaged(u8).initCapacity(allocator, 1024);
        defer stderr.deinit(allocator);

        try manpath_cmd.collectOutput(allocator, &stdout, &stderr, 2048);

        const term = try manpath_cmd.wait();

        switch (term) {
            .Exited => |code| {
                if (code != 0) {
                    std.log.err(
                        "`manpath` exited with code {d}: {s}",
                        .{ code, stderr.items },
                    );
                    return error.ManpathFailed;
                } else {
                    std.log.info("manpath: {s}", .{stdout.items});
                }
            },

            else => {
                std.log.err("`manpath` terminated unexpectedly", .{});
                return error.ManpathFailed;
            },
        }
    }
}
