namespace core {

struct Time {
    double delta_seconds = 0.0;
    double total_seconds = 0.0;
};

struct FixedUpdateConfig {
    double fixed_delta_seconds = 1.0 / 60.0;
};

struct FixedUpdateAccumulator {
    double accumulated_seconds = 0.0;
};

} // namespace core
