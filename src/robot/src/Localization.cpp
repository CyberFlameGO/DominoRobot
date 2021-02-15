#include "Localization.h"

#include <Eigen/Dense>
#include <math.h>
#include "constants.h"

Localization::Localization()
: pos_(0,0,0),
  vel_(0,0,0),
  update_fraction_at_zero_vel_(cfg.lookup("localization.update_fraction_at_zero_vel")),
  val_for_zero_update_(cfg.lookup("localization.val_for_zero_update")),
  mm_x_offset_(cfg.lookup("localization.mm_x_offset")),
  mm_y_offset_(cfg.lookup("localization.mm_y_offset"))
{}

void Localization::updatePositionReading(Point global_position)
{
    // The x,y,a here is from the center of the marvlemind pair, so we need to transform it to the actual center of the
    // robot (i.e. center of rotation)
    Eigen::Vector3f raw_measured_position = {global_position.x, global_position.y, global_position.a};
    Eigen::Vector3f local_offset = {mm_x_offset_/1000.0f, mm_y_offset_/1000.0f, 0.0f};
    float cA = cos(global_position.a);
    float sA = sin(global_position.a);
    Eigen::Matrix3f rotation;
    rotation << cA, -sA, 0.0f, 
                sA, cA, 0.0f,
                0.0f, 0.0f, 1.0f;
    Eigen::Vector3f adjusted_measured_position = raw_measured_position - rotation * local_offset;
    
    // Generate an update fraction based on the current velocity since we know the beacons are less accurate when moving
    Eigen::Vector3f v = {vel_.vx, vel_.vy, vel_.va};
    float total_v = v.norm();
    float slope = update_fraction_at_zero_vel_ / -val_for_zero_update_;
    float update_fraction = update_fraction_at_zero_vel_ + slope * total_v;
    update_fraction = std::max(std::min(update_fraction, update_fraction_at_zero_vel_), 0.0f);
    
    // Actually update the position based on the observed position and the update fraction
    pos_.x += update_fraction * (adjusted_measured_position(0) - pos_.x);
    pos_.y += update_fraction * (adjusted_measured_position(1) - pos_.y);
    pos_.a += update_fraction * (adjusted_measured_position(2) - pos_.a);

}

void Localization::updateVelocityReading(Velocity local_cart_vel, float dt)
{
    // Convert local cartesian velocity to global cartesian velocity using the last estimated angle
    float cA = cos(pos_.a);
    float sA = sin(pos_.a);
    vel_.vx = cA * local_cart_vel.vx - sA * local_cart_vel.vy;
    vel_.vy = sA * local_cart_vel.vx + cA * local_cart_vel.vy;
    vel_.va = local_cart_vel.va;

    // Compute new position estimate
    pos_.x += vel_.vx * dt;
    pos_.y += vel_.vy * dt;
    pos_.a += vel_.va * dt;
}