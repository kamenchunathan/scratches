#pragma once

#include <Eigen/Dense>
#include <format>

namespace core {

enum class EulerOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };

struct Transform {
    Eigen::Quaternionf rotation = Eigen::Quaternionf::Identity();
    Eigen::Vector3f translation = Eigen::Vector3f::Zero();
    Eigen::Vector3f scale       = Eigen::Vector3f::Ones();

    // Construction
    [[nodiscard]] static auto identity() -> Transform;
    [[nodiscard]] static auto from_pos(const Eigen::Vector3f& pos) -> Transform;
    [[nodiscard]] static auto from_rot(const Eigen::Quaternionf& rot) -> Transform;
    [[nodiscard]] static auto from_scale(const Eigen::Vector3f& scale) -> Transform;
    [[nodiscard]] static auto
    from_pos_rot(const Eigen::Vector3f& pos, const Eigen::Quaternionf& rot) -> Transform;
    [[nodiscard]] static auto from_pos_rot_scale(
        const Eigen::Vector3f& pos,
        const Eigen::Quaternionf& rot,
        const Eigen::Vector3f& scale
    ) -> Transform;
    [[nodiscard]] static auto from_matrix(const Eigen::Matrix4f& matrix) -> Transform;
    [[nodiscard]] static auto
    look_at(const Eigen::Vector3f& eye, const Eigen::Vector3f& target, const Eigen::Vector3f& up)
        -> Transform;
    [[nodiscard]] static auto lerp(const Transform& a, const Transform& b, float t) -> Transform;
    [[nodiscard]] static auto slerp(const Transform& a, const Transform& b, float t) -> Transform;

    // Translation - Getters
    [[nodiscard]] auto x() const -> float;
    [[nodiscard]] auto y() const -> float;
    [[nodiscard]] auto z() const -> float;
    [[nodiscard]] auto translation_xy() const -> Eigen::Vector2f;
    [[nodiscard]] auto translation_xz() const -> Eigen::Vector2f;
    [[nodiscard]] auto translation_yz() const -> Eigen::Vector2f;
    [[nodiscard]] auto translation_xyz() const -> Eigen::Vector3f;

    // Translation - Setters
    auto set_x(float x) -> void;
    auto set_y(float y) -> void;
    auto set_z(float z) -> void;
    auto set_translation_xy(const Eigen::Vector2f& xy) -> void;
    auto set_translation_xz(const Eigen::Vector2f& xz) -> void;
    auto set_translation_yz(const Eigen::Vector2f& yz) -> void;
    auto set_translation_xyz(const Eigen::Vector3f& xyz) -> void;

    // Translation - Helpers
    auto translate(const Eigen::Vector3f& delta) -> void;
    auto translate_local(const Eigen::Vector3f& delta) -> void;
    auto translate_x(float amount) -> void;
    auto translate_y(float amount) -> void;
    auto translate_z(float amount) -> void;
    auto move_towards(const Eigen::Vector3f& target, float max_delta) -> void;
    [[nodiscard]] auto distance_to(const Transform& other) const -> float;
    [[nodiscard]] auto direction_to(const Transform& other) const -> Eigen::Vector3f;

    // Rotation - Imaginary Swizzles
    [[nodiscard]] auto rotation_xy() const -> Eigen::Vector2f;
    [[nodiscard]] auto rotation_xz() const -> Eigen::Vector2f;
    [[nodiscard]] auto rotation_xyz() const -> Eigen::Vector3f;
    [[nodiscard]] auto rotation_xyzw() const -> Eigen::Vector4f;

    auto set_rotation_xyz(const Eigen::Vector3f& xyz) -> void;
    auto set_rotation_xyzw(const Eigen::Vector4f& xyzw) -> void;

    // Rotation - Utilities
    [[nodiscard]] auto rotation_conjugate() const -> Eigen::Quaternionf;
    [[nodiscard]] auto rotation_inverse() const -> Eigen::Quaternionf;
    auto rotation_normalise() -> void;
    [[nodiscard]] auto rotation_dot(const Eigen::Quaternionf& other) const -> float;
    [[nodiscard]] auto rotation_angle_to(const Eigen::Quaternionf& other) const -> float;

    // Rotation - Euler (Degrees)
    [[nodiscard]] auto euler_angles(EulerOrder order = EulerOrder::XYZ) const -> Eigen::Vector3f;
    [[nodiscard]] auto pitch() const -> float;
    [[nodiscard]] auto yaw() const -> float;
    [[nodiscard]] auto roll() const -> float;

    auto set_euler_angles(const Eigen::Vector3f& degrees, EulerOrder order = EulerOrder::XYZ)
        -> void;
    auto set_pitch(float degrees) -> void;
    auto set_yaw(float degrees) -> void;
    auto set_roll(float degrees) -> void;

    // Rotation - Apply
    auto rotate(const Eigen::Quaternionf& q) -> void;
    auto rotate_local(const Eigen::Quaternionf& q) -> void;
    auto rotate_around_axis(const Eigen::Vector3f& axis, float angle_rad) -> void;
    auto rotate_around_axis_local(const Eigen::Vector3f& axis, float angle_rad) -> void;
    auto
    rotate_around_point(const Eigen::Vector3f& point, const Eigen::Vector3f& axis, float angle_rad)
        -> void;
    auto rotate_x(float angle_rad) -> void;
    auto rotate_y(float angle_rad) -> void;
    auto rotate_z(float angle_rad) -> void;

    auto look_at_target(const Eigen::Vector3f& target, const Eigen::Vector3f& up) -> void;
    auto look_direction(const Eigen::Vector3f& dir, const Eigen::Vector3f& up) -> void;
    auto slerp_to(const Eigen::Quaternionf& target, float t) -> void;
    auto rotate_towards(const Eigen::Quaternionf& target, float max_radians) -> void;

    // Local Axes
    [[nodiscard]] auto forward() const -> Eigen::Vector3f;
    [[nodiscard]] auto back() const -> Eigen::Vector3f;
    [[nodiscard]] auto right() const -> Eigen::Vector3f;
    [[nodiscard]] auto left() const -> Eigen::Vector3f;
    [[nodiscard]] auto up() const -> Eigen::Vector3f;
    [[nodiscard]] auto down() const -> Eigen::Vector3f;

    // Scale
    [[nodiscard]] auto scale_x() const -> float;
    [[nodiscard]] auto scale_y() const -> float;
    [[nodiscard]] auto scale_z() const -> float;
    [[nodiscard]] auto scale_xy() const -> Eigen::Vector2f;
    [[nodiscard]] auto scale_xyz() const -> Eigen::Vector3f;

    auto set_scale_x(float x) -> void;
    auto set_scale_y(float y) -> void;
    auto set_scale_z(float z) -> void;
    auto set_scale_uniform(float s) -> void;
    auto scale_by(const Eigen::Vector3f& s) -> void;
    auto scale_uniform_by(float s) -> void;

    [[nodiscard]] auto scale_max() const -> float;
    [[nodiscard]] auto scale_min() const -> float;
    [[nodiscard]] auto is_uniform_scale(float epsilon = 1e-6f) const -> bool;
    [[nodiscard]] auto has_negative_scale() const -> bool;

    // Matrix Conversion
    [[nodiscard]] auto to_mat4() const -> Eigen::Matrix4f;
    [[nodiscard]] auto to_mat4x3() const -> Eigen::Matrix<float, 3, 4>;
    [[nodiscard]] auto to_mat3() const -> Eigen::Matrix3f;
    [[nodiscard]] auto to_rotation_mat3() const -> Eigen::Matrix3f;
    [[nodiscard]] auto to_rotation_mat4() const -> Eigen::Matrix4f;
    [[nodiscard]] auto to_normal_matrix() const -> Eigen::Matrix3f;
    [[nodiscard]] auto to_mat4_inverse() const -> Eigen::Matrix4f;
    [[nodiscard]] auto to_trs_inverse() const -> Transform;
    [[nodiscard]] auto to_view_matrix() const -> Eigen::Matrix4f;

    [[nodiscard]] auto translation_matrix() const -> Eigen::Matrix4f;
    [[nodiscard]] auto rotation_matrix() const -> Eigen::Matrix4f;
    [[nodiscard]] auto scale_matrix() const -> Eigen::Matrix4f;

    // Space Conversion
    [[nodiscard]] auto to_local_space(const Eigen::Vector3f& world_point) const -> Eigen::Vector3f;
    [[nodiscard]] auto to_world_space(const Eigen::Vector3f& local_point) const -> Eigen::Vector3f;
    [[nodiscard]] auto transform_point(const Eigen::Vector3f& point) const -> Eigen::Vector3f;
    [[nodiscard]] auto transform_direction(const Eigen::Vector3f& direction) const
        -> Eigen::Vector3f;
    [[nodiscard]] auto transform_normal(const Eigen::Vector3f& normal) const -> Eigen::Vector3f;
    [[nodiscard]] auto inverse_transform_point(const Eigen::Vector3f& point) const
        -> Eigen::Vector3f;
    [[nodiscard]] auto inverse_transform_direction(const Eigen::Vector3f& direction) const
        -> Eigen::Vector3f;

    // Composition
    [[nodiscard]] auto compose(const Transform& other) const -> Transform;
    [[nodiscard]] auto operator*(const Transform& other) const -> Transform;
    [[nodiscard]] static auto relative(const Transform& from, const Transform& to) -> Transform;

    // Comparison & Validation
    [[nodiscard]] auto equals(const Transform& other, float epsilon = 1e-6f) const -> bool;
    [[nodiscard]] auto is_identity(float epsilon = 1e-6f) const -> bool;
    [[nodiscard]] auto is_valid() const -> bool;
    auto normalise() -> void;
};

} // namespace core

template<>
struct std::formatter<core::Transform> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const core::Transform& t, std::format_context& ctx) const {
        return std::format_to(
            ctx.out(),
            "Transform(pos: [{}, {}, {}], rot: [{}, {}, {}, {}], scale: [{}, {}, {}])",
            t.translation.x(),
            t.translation.y(),
            t.translation.z(),
            t.rotation.w(),
            t.rotation.x(),
            t.rotation.y(),
            t.rotation.z(),
            t.scale.x(),
            t.scale.y(),
            t.scale.z()
        );
    }
};
