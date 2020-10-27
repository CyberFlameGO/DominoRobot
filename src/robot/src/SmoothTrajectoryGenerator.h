#ifndef SmoothTrajectoryGenerator_h
#define SmoothTrajectoryGenerator_h

#include <Eigen/Dense>

struct Point
{
    float x_;
    float y_;
    float a_;

    Point(float x=0, float y=0, float a=0)
    : x_(x), y_(y), a_(a)
    {}

    std::string toString() const
    {
        char s[100];
        sprintf(s, "[x: %.3f, y: %.3f, a: %.3f]", x_, y_, a_);
        return static_cast<std::string>(s);
    }

    bool operator== (const Point& other) const
    {
        return x_ == other.x_ && y_ == other.y_ && a_ == other.a_;
    }
};

struct Velocity
{
    float vx_;
    float vy_;
    float va_;

    Velocity(float vx=0, float vy=0, float va=0)
    : vx_(vx), vy_(vy), va_(va)
    {}

    std::string toString() const
    {
        char s[100];
        sprintf(s, "[vx: %.3f, vy: %.3f, va: %.3f]", vx_, vy_, va_);
        return static_cast<std::string>(s);
    }

    bool operator== (const Velocity& other) const 
    {
        return vx_ == other.vx_ && vy_ == other.vy_ && va_ == other.va_;
    }

    bool nearZero(float eps=1e-6) const
    {
        if (fabs(vx_) < eps && fabs(vy_) < eps && fabs(va_) < eps)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
};

// Return structure for a trajectory point lookup that contains all the info about a point in time the controller
// needs to drive the robot
struct PVTPoint
{
    Point position_;
    Velocity velocity_;
    float time_;

    std::string toString() const
    {
      char s[200];
      sprintf(s, "[Position: %s, Velocity: %s, T: %.3f]", position_.toString().c_str(), velocity_.toString().c_str(), time_);
      return static_cast<std::string>(s);
    }

};

// Contains info about the maximum dynamic limits of a trajectory
struct DynamicLimits
{
    float max_vel_;
    float max_acc_;
    float max_jerk_;

    DynamicLimits operator* (float c)
    {
        DynamicLimits rtn;
        rtn.max_vel_ = c * max_vel_;
        rtn.max_acc_ = c * max_acc_;
        rtn.max_jerk_ = c * max_jerk_;
        return rtn;
    }
};

// A fully defined point for switching from one region of the trajectory
// to another - needed for efficient lookup without building a huge table
struct SwitchPoint
{
    float t_;
    float p_;
    float v_;
    float a_;
};

// Parameters defining a 1-D S-curve trajectory
struct SCurveParameters
{
    float v_lim_;
    float a_lim_;
    float j_lim_;
    SwitchPoint switch_points_[8];

    std::string toString() const
    {
      char s[256];
      sprintf(s, "    Limits: [v: %.3f, a: %.3f, j: %.3f]\n    Switch times: [%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f]", v_lim_, a_lim_, j_lim_,
      switch_points_[0].t_,switch_points_[1].t_,switch_points_[2].t_,switch_points_[3].t_,switch_points_[4].t_,switch_points_[5].t_,switch_points_[6].t_,switch_points_[7].t_);
      return static_cast<std::string>(s);
    }
};

// Everything needed to define a point to point s-curve trajectory in X, Y, and angle
struct Trajectory
{
    Eigen::Vector2f trans_direction_;
    int rot_direction_;
    Point initialPoint_;
    SCurveParameters trans_params_;
    SCurveParameters rot_params_;
    bool complete_;

    std::string toString() const
    {
      char s[1000];
      sprintf(s, "Trajectory Parameters:\nTranslation:\n  Direction: [%.2f, %.2f]\n  S-Curve:\n%s\nRotation:\n  Direction: %i\n  S-Curve:\n%s\n", 
        trans_direction_[0], trans_direction_[1], trans_params_.toString().c_str(), rot_direction_, rot_params_.toString().c_str());
      return static_cast<std::string>(s);
    }
};

struct SolverParameters
{
    int num_loops_;
    float alpha_decay_;
    float beta_decay_;
    float exponent_decay_;
};

// All the pieces needed to define the motion planning problem
struct MotionPlanningProblem
{
    Eigen::Vector3f initialPoint_;
    Eigen::Vector3f targetPoint_;
    DynamicLimits translationalLimits_;
    DynamicLimits rotationalLimits_;  
    SolverParameters solver_params_;
};

// Helper methods - making public for easier testing
MotionPlanningProblem buildMotionPlanningProblem(Point initialPoint, Point targetPoint, bool fineMode, const SolverParameters& solver);
Trajectory generateTrajectory(MotionPlanningProblem problem);
bool generateSCurve(float dist, DynamicLimits limits, const SolverParameters& solver, SCurveParameters* params);
void populateSwitchTimeParameters(SCurveParameters* params, float dt_j, float dt_a, float dt_v);
bool synchronizeParameters(SCurveParameters* params1, SCurveParameters* params2);
bool solveInverse(SCurveParameters* params);
std::vector<float> lookup_1D(float time, const SCurveParameters& params);
std::vector<float> computeKinematicsBasedOnRegion(const SCurveParameters& params, int region, float dt);


class SmoothTrajectoryGenerator
{

  public:
    SmoothTrajectoryGenerator();

    // Generates a trajectory that starts at the initial point and ends at the target point. Setting fineMode to true makes the 
    // adjusts the dynamic limits for a more accurate motion. Returns a bool indicating if trajectory generation was
    // successful
    bool generatePointToPointTrajectory(Point initialPoint, Point targetPoint, bool fineMode);

    // Generates a trajectory that attempts to maintain the target velocity for a specified time. Note that the current implimentation
    // of this does not give a guarantee on the accuracy of the velocity if the specified velocity and move time would violate the dynamic 
    // limits of the fine or coarse movement mode. Returns a bool indicating if trajectory generation was successful
    bool generateConstVelTrajectory(Point initialPoint, Velocity velocity, float moveTime, bool fineMode);

    // Looks up a point in the current trajectory based on the time, in seconds, from the start of the trajectory
    PVTPoint lookup(float time);

  private:

    // The current trajectory - this lets the generation class hold onto this and just provide a lookup method
    // since I don't have a need to pass the trajectory around anywhere
    Trajectory currentTrajectory_;
    
    // These need to be part of the class because they need to be loaded at construction time, not
    // program initialization time (i.e. as globals). This is because the config file is not
    // yet loaded at program start up time.
    SolverParameters solver_params_;
};

#endif