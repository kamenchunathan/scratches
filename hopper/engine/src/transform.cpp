#include "transform.hpp"
#include <Eigen/Geometry>
#include <numbers>

#ifdef _MSC_VER
    #define _USE_MATH_DEFINES
#endif
#include <cmath>

namespace core {

// Construction
auto Transform::identity() -> Transform {
    return Transform {};
}

auto Transform::from_pos(const Eigen::Vector3f& pos) -> Transform {
    Transform t;
    t.translation = pos;
    return t;
}

auto Transform::from_rot(const Eigen::Quaternionf& rot) -> Transform {
    Transform t;
    t.rotation = rot;
    return t;
}

auto Transform::from_scale(const Eigen::Vector3f& scale) -> Transform {
    Transform t;
    t.scale = scale;
    return t;
}

auto Transform::from_pos_rot(const Eigen::Vector3f& pos, const Eigen::Quaternionf& rot)
    -> Transform {
    Transform t;
    t.translation = pos;
    t.rotation    = rot;
    return t;
}

auto Transform::from_pos_rot_scale(
    const Eigen::Vector3f& pos,
    const Eigen::Quaternionf& rot,
    const Eigen::Vector3f& scale
) -> Transform {
    Transform t;
    t.translation = pos;
    t.rotation    = rot;
    t.scale       = scale;
    return t;
}

auto Transform::from_matrix(const Eigen::Matrix4f& matrix) -> Transform {
    Transform t;
    t.translation = matrix.block<3, 1>(0, 3);

    Eigen::Matrix3f rotation_scale_matrix = matrix.block<3, 3>(0, 0);
    t.scale.x()                           = rotation_scale_matrix.col(0).norm();
    t.scale.y()                           = rotation_scale_matrix.col(1).norm();
    t.scale.z()                           = rotation_scale_matrix.col(2).norm();

    Eigen::Matrix3f rotation_matrix = rotation_scale_matrix;
    rotation_matrix.col(0) /= t.scale.x();
    rotation_matrix.col(1) /= t.scale.y();
    rotation_matrix.col(2) /= t.scale.z();

    t.rotation = Eigen::Quaternionf(rotation_matrix);
    return t;
}

auto Transform::look_at(
    const Eigen::Vector3f& eye,
    const Eigen::Vector3f& target,
    const Eigen::Vector3f& up
) -> Transform {
    Transform t;
    t.translation = eye;
    t.look_at_target(target, up);
    return t;
}

auto Transform::lerp(const Transform& a, const Transform& b, float t) -> Transform {
    Transform res;
    res.translation = a.translation + (b.translation - a.translation) * t;
    res.rotation    = a.rotation.slerp(t, b.rotation);
    res.scale       = a.scale + (b.scale - a.scale) * t;
    return res;
}

auto Transform::slerp(const Transform& a, const Transform& b, float t) -> Transform {
    return lerp(a, b, t);
}

// Translation - Getters
auto Transform::x() const -> float {
    return translation.x();
}
auto Transform::y() const -> float {
    return translation.y();
}
auto Transform::z() const -> float {
    return translation.z();
}
auto Transform::translation_xy() const -> Eigen::Vector2f {
    return translation.head<2>();
}
auto Transform::translation_xz() const -> Eigen::Vector2f {
    return {translation.x(), translation.z()};
}
auto Transform::translation_yz() const -> Eigen::Vector2f {
    return translation.tail<2>();
}
auto Transform::translation_xyz() const -> Eigen::Vector3f {
    return translation;
}

// Translation - Setters
auto Transform::set_x(float x) -> void {
    translation.x() = x;
}
auto Transform::set_y(float y) -> void {
    translation.y() = y;
}
auto Transform::set_z(float z) -> void {
    translation.z() = z;
}
auto Transform::set_translation_xy(const Eigen::Vector2f& xy) -> void {
    translation.head<2>() = xy;
}
auto Transform::set_translation_xz(const Eigen::Vector2f& xz) -> void {
    translation.x() = xz.x();
    translation.z() = xz.y();
}
auto Transform::set_translation_yz(const Eigen::Vector2f& yz) -> void {
    translation.tail<2>() = yz;
}
auto Transform::set_translation_xyz(const Eigen::Vector3f& xyz) -> void {
    translation = xyz;
}

// Translation - Helpers
auto Transform::translate(const Eigen::Vector3f& delta) -> void {
    translation += delta;
}
auto Transform::translate_local(const Eigen::Vector3f& delta) -> void {
    translation += rotation * delta;
}
auto Transform::translate_x(float amount) -> void {
    translation.x() += amount;
}
auto Transform::translate_y(float amount) -> void {
    translation.y() += amount;
}
auto Transform::translate_z(float amount) -> void {
    translation.z() += amount;
}
auto Transform::move_towards(const Eigen::Vector3f& target, float max_delta) -> void {
    Eigen::Vector3f delta = target - translation;
    float dist            = delta.norm();
    if (dist <= max_delta || dist < 1e-6f) {
        translation = target;
    } else {
        translation += (delta / dist) * max_delta;
    }
}
auto Transform::distance_to(const Transform& other) const -> float {
    return (other.translation - translation).norm();
}
auto Transform::direction_to(const Transform& other) const -> Eigen::Vector3f {
    return (other.translation - translation).normalized();
}

// Rotation - Imaginary Swizzles
auto Transform::rotation_xy() const -> Eigen::Vector2f {
    return {rotation.x(), rotation.y()};
}
auto Transform::rotation_xz() const -> Eigen::Vector2f {
    return {rotation.x(), rotation.z()};
}
auto Transform::rotation_xyz() const -> Eigen::Vector3f {
    return {rotation.x(), rotation.y(), rotation.z()};
}
auto Transform::rotation_xyzw() const -> Eigen::Vector4f {
    return {rotation.x(), rotation.y(), rotation.z(), rotation.w()};
}

auto Transform::set_rotation_xyz(const Eigen::Vector3f& xyz) -> void {
    rotation.x() = xyz.x();
    rotation.y() = xyz.y();
    rotation.z() = xyz.z();
}
auto Transform::set_rotation_xyzw(const Eigen::Vector4f& xyzw) -> void {
    rotation.x() = xyzw.x();
    rotation.y() = xyzw.y();
    rotation.z() = xyzw.z();
    rotation.w() = xyzw.w();
}

// Rotation - Utilities
auto Transform::rotation_conjugate() const -> Eigen::Quaternionf {
    return rotation.conjugate();
}
auto Transform::rotation_inverse() const -> Eigen::Quaternionf {
    return rotation.inverse();
}
auto Transform::rotation_normalise() -> void {
    rotation.normalize();
}
auto Transform::rotation_dot(const Eigen::Quaternionf& other) const -> float {
    return rotation.dot(other);
}
auto Transform::rotation_angle_to(const Eigen::Quaternionf& other) const -> float {
    return rotation.angularDistance(other);
}

// Rotation - Euler (Degrees)
auto Transform::euler_angles(EulerOrder order) const -> Eigen::Vector3f {
    Eigen::Vector3f angles;
    switch (order) {
        case EulerOrder::XYZ:
            angles = rotation.toRotationMatrix().eulerAngles(0, 1, 2);
            break;
        case EulerOrder::XZY:
            angles = rotation.toRotationMatrix().eulerAngles(0, 2, 1);
            break;
        case EulerOrder::YXZ:
            angles = rotation.toRotationMatrix().eulerAngles(1, 0, 2);
            break;
        case EulerOrder::YZX:
            angles = rotation.toRotationMatrix().eulerAngles(1, 2, 0);
            break;
        case EulerOrder::ZXY:
            angles = rotation.toRotationMatrix().eulerAngles(2, 0, 1);
            break;
        case EulerOrder::ZYX:
            angles = rotation.toRotationMatrix().eulerAngles(2, 1, 0);
            break;
    }
    return angles * 180.0f / std::numbers::pi_v<float>;
}

auto Transform::pitch() const -> float {
    return euler_angles(EulerOrder::XYZ).x();
}
auto Transform::yaw() const -> float {
    return euler_angles(EulerOrder::XYZ).y();
}
auto Transform::roll() const -> float {
    return euler_angles(EulerOrder::XYZ).z();
}

auto Transform::set_euler_angles(const Eigen::Vector3f& degrees, EulerOrder order) -> void {
    Eigen::Vector3f radians = degrees * std::numbers::pi_v<float> / 180.0f;
    switch (order) {
        case EulerOrder::XYZ:
            rotation = Eigen::AngleAxisf(radians.x(), Eigen::Vector3f::UnitX())
                * Eigen::AngleAxisf(radians.y(), Eigen::Vector3f::UnitY())
                * Eigen::AngleAxisf(radians.z(), Eigen::Vector3f::UnitZ());
            break;
        case EulerOrder::XZY:
            rotation = Eigen::AngleAxisf(radians.x(), Eigen::Vector3f::UnitX())
                * Eigen::AngleAxisf(radians.y(), Eigen::Vector3f::UnitZ())
                * Eigen::AngleAxisf(radians.z(), Eigen::Vector3f::UnitY());
            break;
        case EulerOrder::YXZ:
            rotation = Eigen::AngleAxisf(radians.x(), Eigen::Vector3f::UnitY())
                * Eigen::AngleAxisf(radians.y(), Eigen::Vector3f::UnitX())
                * Eigen::AngleAxisf(radians.z(), Eigen::Vector3f::UnitZ());
            break;
        case EulerOrder::YZX:
            rotation = Eigen::AngleAxisf(radians.x(), Eigen::Vector3f::UnitY())
                * Eigen::AngleAxisf(radians.y(), Eigen::Vector3f::UnitZ())
                * Eigen::AngleAxisf(radians.z(), Eigen::Vector3f::UnitX());
            break;
        case EulerOrder::ZXY:
            rotation = Eigen::AngleAxisf(radians.x(), Eigen::Vector3f::UnitZ())
                * Eigen::AngleAxisf(radians.y(), Eigen::Vector3f::UnitX())
                * Eigen::AngleAxisf(radians.z(), Eigen::Vector3f::UnitY());
            break;
        case EulerOrder::ZYX:
            rotation = Eigen::AngleAxisf(radians.x(), Eigen::Vector3f::UnitZ())
                * Eigen::AngleAxisf(radians.y(), Eigen::Vector3f::UnitY())
                * Eigen::AngleAxisf(radians.z(), Eigen::Vector3f::UnitX());
            break;
    }
}

auto Transform::set_pitch(float degrees) -> void {
    Eigen::Vector3f angles = euler_angles(EulerOrder::XYZ);
    angles.x()             = degrees;
    set_euler_angles(angles, EulerOrder::XYZ);
}
auto Transform::set_yaw(float degrees) -> void {
    Eigen::Vector3f angles = euler_angles(EulerOrder::XYZ);
    angles.y()             = degrees;
    set_euler_angles(angles, EulerOrder::XYZ);
}
auto Transform::set_roll(float degrees) -> void {
    Eigen::Vector3f angles = euler_angles(EulerOrder::XYZ);
    angles.z()             = degrees;
    set_euler_angles(angles, EulerOrder::XYZ);
}

// Rotation - Apply
auto Transform::rotate(const Eigen::Quaternionf& q) -> void {
    rotation = q * rotation;
}
auto Transform::rotate_local(const Eigen::Quaternionf& q) -> void {
    rotation = rotation * q;
}
auto Transform::rotate_around_axis(const Eigen::Vector3f& axis, float angle_rad) -> void {
    rotate(Eigen::Quaternionf(Eigen::AngleAxisf(angle_rad, axis)));
}
auto Transform::rotate_around_axis_local(const Eigen::Vector3f& axis, float angle_rad) -> void {
    rotate_local(Eigen::Quaternionf(Eigen::AngleAxisf(angle_rad, axis)));
}
auto Transform::rotate_around_point(
    const Eigen::Vector3f& point,
    const Eigen::Vector3f& axis,
    float angle_rad
) -> void {
    Eigen::Vector3f dir = translation - point;
    Eigen::Quaternionf q(Eigen::AngleAxisf(angle_rad, axis));
    dir         = q * dir;
    translation = point + dir;
    rotate(q);
}
auto Transform::rotate_x(float angle_rad) -> void {
    rotate_around_axis(Eigen::Vector3f::UnitX(), angle_rad);
}
auto Transform::rotate_y(float angle_rad) -> void {
    rotate_around_axis(Eigen::Vector3f::UnitY(), angle_rad);
}
auto Transform::rotate_z(float angle_rad) -> void {
    rotate_around_axis(Eigen::Vector3f::UnitZ(), angle_rad);
}

auto Transform::look_at_target(const Eigen::Vector3f& target, const Eigen::Vector3f& up) -> void {
    look_direction(target - translation, up);
}
auto Transform::look_direction(const Eigen::Vector3f& dir, const Eigen::Vector3f& up) -> void {
    if (dir.squaredNorm() < 1e-6f)
        return;
    Eigen::Vector3f z = -dir.normalized();
    Eigen::Vector3f x = up.cross(z).normalized();
    Eigen::Vector3f y = z.cross(x);

    Eigen::Matrix3f m;
    m.col(0) = x;
    m.col(1) = y;
    m.col(2) = z;
    rotation = Eigen::Quaternionf(m);
}
auto Transform::slerp_to(const Eigen::Quaternionf& target, float t) -> void {
    rotation = rotation.slerp(t, target);
}
auto Transform::rotate_towards(const Eigen::Quaternionf& target, float max_radians) -> void {
    float angle = rotation.angularDistance(target);
    if (angle <= max_radians) {
        rotation = target;
    } else {
        slerp_to(target, max_radians / angle);
    }
}

// Local Axes
auto Transform::forward() const -> Eigen::Vector3f {
    return rotation * -Eigen::Vector3f::UnitZ();
}
auto Transform::back() const -> Eigen::Vector3f {
    return rotation * Eigen::Vector3f::UnitZ();
}
auto Transform::right() const -> Eigen::Vector3f {
    return rotation * Eigen::Vector3f::UnitX();
}
auto Transform::left() const -> Eigen::Vector3f {
    return rotation * -Eigen::Vector3f::UnitX();
}
auto Transform::up() const -> Eigen::Vector3f {
    return rotation * Eigen::Vector3f::UnitY();
}
auto Transform::down() const -> Eigen::Vector3f {
    return rotation * -Eigen::Vector3f::UnitY();
}

// Scale
auto Transform::scale_x() const -> float {
    return scale.x();
}
auto Transform::scale_y() const -> float {
    return scale.y();
}
auto Transform::scale_z() const -> float {
    return scale.z();
}
auto Transform::scale_xy() const -> Eigen::Vector2f {
    return scale.head<2>();
}
auto Transform::scale_xyz() const -> Eigen::Vector3f {
    return scale;
}

auto Transform::set_scale_x(float x) -> void {
    scale.x() = x;
}
auto Transform::set_scale_y(float y) -> void {
    scale.y() = y;
}
auto Transform::set_scale_z(float z) -> void {
    scale.z() = z;
}
auto Transform::set_scale_uniform(float s) -> void {
    scale.fill(s);
}
auto Transform::scale_by(const Eigen::Vector3f& s) -> void {
    scale = scale.cwiseProduct(s);
}
auto Transform::scale_uniform_by(float s) -> void {
    scale *= s;
}

auto Transform::scale_max() const -> float {
    return scale.maxCoeff();
}
auto Transform::scale_min() const -> float {
    return scale.minCoeff();
}
auto Transform::is_uniform_scale(float epsilon) const -> bool {
    return std::abs(scale.x() - scale.y()) < epsilon && std::abs(scale.x() - scale.z()) < epsilon;
}
auto Transform::has_negative_scale() const -> bool {
    return scale.x() < 0.0f || scale.y() < 0.0f || scale.z() < 0.0f;
}

// Matrix Conversion
auto Transform::to_mat4() const -> Eigen::Matrix4f {
    Eigen::Matrix4f m   = Eigen::Matrix4f::Identity();
    m.block<3, 3>(0, 0) = rotation.toRotationMatrix() * scale.asDiagonal();
    m.block<3, 1>(0, 3) = translation;
    return m;
}
auto Transform::to_mat4x3() const -> Eigen::Matrix<float, 3, 4> {
    Eigen::Matrix<float, 3, 4> m;
    m.block<3, 3>(0, 0) = rotation.toRotationMatrix() * scale.asDiagonal();
    m.block<3, 1>(0, 3) = translation;
    return m;
}
auto Transform::to_mat3() const -> Eigen::Matrix3f {
    return rotation.toRotationMatrix() * scale.asDiagonal();
}
auto Transform::to_rotation_mat3() const -> Eigen::Matrix3f {
    return rotation.toRotationMatrix();
}
auto Transform::to_rotation_mat4() const -> Eigen::Matrix4f {
    Eigen::Matrix4f m   = Eigen::Matrix4f::Identity();
    m.block<3, 3>(0, 0) = rotation.toRotationMatrix();
    return m;
}
auto Transform::to_normal_matrix() const -> Eigen::Matrix3f {
    return (rotation.toRotationMatrix() * scale.asDiagonal()).inverse().transpose();
}
auto Transform::to_mat4_inverse() const -> Eigen::Matrix4f {
    return to_mat4().inverse();
}
auto Transform::to_trs_inverse() const -> Transform {
    Transform res;
    res.scale       = Eigen::Vector3f(1.0f / scale.x(), 1.0f / scale.y(), 1.0f / scale.z());
    res.rotation    = rotation.inverse();
    res.translation = res.rotation * (res.scale.asDiagonal() * -translation);
    return res;
}
auto Transform::to_view_matrix() const -> Eigen::Matrix4f {
    Eigen::Matrix4f m   = Eigen::Matrix4f::Identity();
    Eigen::Matrix3f rot = rotation.toRotationMatrix().transpose();
    m.block<3, 3>(0, 0) = rot;
    m.block<3, 1>(0, 3) = rot * -translation;
    return m;
}

auto Transform::translation_matrix() const -> Eigen::Matrix4f {
    Eigen::Matrix4f m   = Eigen::Matrix4f::Identity();
    m.block<3, 1>(0, 3) = translation;
    return m;
}
auto Transform::rotation_matrix() const -> Eigen::Matrix4f {
    return to_rotation_mat4();
}
auto Transform::scale_matrix() const -> Eigen::Matrix4f {
    Eigen::Matrix4f m   = Eigen::Matrix4f::Identity();
    m.block<3, 3>(0, 0) = scale.asDiagonal();
    return m;
}

// Space Conversion
auto Transform::to_local_space(const Eigen::Vector3f& world_point) const -> Eigen::Vector3f {
    return inverse_transform_point(world_point);
}
auto Transform::to_world_space(const Eigen::Vector3f& local_point) const -> Eigen::Vector3f {
    return transform_point(local_point);
}
auto Transform::transform_point(const Eigen::Vector3f& point) const -> Eigen::Vector3f {
    return rotation * (scale.asDiagonal() * point) + translation;
}
auto Transform::transform_direction(const Eigen::Vector3f& direction) const -> Eigen::Vector3f {
    return rotation * (scale.asDiagonal() * direction);
}
auto Transform::transform_normal(const Eigen::Vector3f& normal) const -> Eigen::Vector3f {
    return (rotation * scale.asDiagonal()).inverse().transpose() * normal;
}
auto Transform::inverse_transform_point(const Eigen::Vector3f& point) const -> Eigen::Vector3f {
    return scale.asDiagonal().inverse() * (rotation.inverse() * (point - translation));
}
auto Transform::inverse_transform_direction(const Eigen::Vector3f& direction) const
    -> Eigen::Vector3f {
    return scale.asDiagonal().inverse() * (rotation.inverse() * direction);
}

// Composition
auto Transform::compose(const Transform& other) const -> Transform {
    Transform res;
    res.translation = transform_point(other.translation);
    res.rotation    = rotation * other.rotation;
    res.scale       = scale.cwiseProduct(other.scale);
    return res;
}
auto Transform::operator*(const Transform& other) const -> Transform {
    return compose(other);
}
auto Transform::relative(const Transform& from, const Transform& to) -> Transform {
    return from.to_trs_inverse().compose(to);
}

// Comparison & Validation
auto Transform::equals(const Transform& other, float epsilon) const -> bool {
    return translation.isApprox(other.translation, epsilon)
        && rotation.isApprox(other.rotation, epsilon) && scale.isApprox(other.scale, epsilon);
}
auto Transform::is_identity(float epsilon) const -> bool {
    return translation.isZero(epsilon) && rotation.isApprox(Eigen::Quaternionf::Identity(), epsilon)
        && scale.isApprox(Eigen::Vector3f::Ones(), epsilon);
}
auto Transform::is_valid() const -> bool {
    return translation.allFinite() && rotation.coeffs().allFinite() && scale.allFinite();
}
auto Transform::normalise() -> void {
    rotation.normalize();
}

} // namespace core
