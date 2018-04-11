#ifndef __HYPOTHESIS_H__
#define __HYPOTHESIS_H__

#include <Eigen/Eigenvalues>

#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <memory> // std::shared_ptr

#include <multi_object_tracking/hungarian.h>
#include <limits.h> // for INT_MAX

#include <sys/time.h>
#include <iostream>
#include <map>

#include <multi_object_tracking/kalman_filter.h>
#include <multi_object_tracking/utils.h>

/**
 * @brief Description of detection //TODO: rename
 */
struct Measurement
{
  Eigen::Vector3f pos;      ///< position of detection
  Eigen::Matrix3f cov;      ///< covariance of detection
  uint8_t color;            ///< TODO: adapt description - ascii code of the first letter of the color of the detected object ('r'ed, 'b'lue, 'g'reen, 'y'ellow, 'o'range, 'u'nknown)
  std::string frame;        ///< frame_id of detection
  double time;              ///< time_stamp of the detection - in time of day
};


namespace MultiHypothesisTracker
{

struct TrackerParameters
{
  double cov_x_per_sec;
  double cov_y_per_sec;
  double cov_z_per_sec;
  double cov_vx_per_sec;
  double cov_vy_per_sec;
  double cov_vz_per_sec;
  double alpha_vx_vx_per_sec;
  double alpha_vx_vy_per_sec;
  double alpha_vy_vy_per_sec;
  double alpha_vz_vz_per_sec;

  double init_cov;
  double max_cov;

  double measurementStd;

  double ambiguous_dist;
};


class Hypothesis
{
public:

  Hypothesis();
  virtual ~Hypothesis(){};

  virtual const TrackerParameters& getParameters();

  inline unsigned int getID(){ return m_id; }

  // TODO: delete? - never used, just set
  inline unsigned int getNumStateDimensions() { return m_numStateDimensions; }

  inline Eigen::Vector3f& getMean(){ return m_kalman.m_state.block<3,1>(0, 0); }
  inline Eigen::Matrix3f& getCovariance(){ return m_covariance; }
  inline uint8_t getColor(){ return m_color; }
  inline bool isStatic(){ return m_is_static; }

  virtual void initialize(const Measurement& measurement,
                          unsigned int id);

  virtual bool isSpurious();

  virtual void detected();
  virtual void undetected();

  inline float getDetectionRate() { return m_detection_rate; }
  inline float getMisdetectionRate() { return m_misdetection_rate; }

  virtual void predict( float dt, Eigen::Vector3f& control );
  virtual void correct( const Measurement& measurement );

  inline double get_born_time(){return m_born_time;}
  inline void detected_absolute(){m_times_measured++;}  //total number of times that hypothesis had a measurement
  inline Eigen::Vector3f get_velocity(){ return m_velocity;}
  inline Measurement get_latest_measurement(){ return m_latest_measurement;}
  inline double get_latest_measurement_time(){ return m_last_measurement_time;}

  KalmanFilter m_kalman;

protected:

  Eigen::Vector3f m_last_mean_with_measurement;
  // double m_last_prediction_time;
  bool m_is_first_position;
  Eigen::Vector3f m_velocity;
  Measurement m_previous_measurement;
  Measurement m_latest_measurement;
  void velocity_decay();
  void verify_static();
  bool m_is_static;
  Eigen::Vector3f m_first_position_in_track;
  Eigen::Vector3f m_max_velocity_in_track;
  double m_born_time;
  int m_times_measured;




  Eigen::Vector3f m_mean;
  Eigen::Matrix3f m_covariance;
  uint8_t m_color;
  double m_last_measurement_time;    //needed to calculate if it's spurious or not.
  float m_detection_rate;
  float m_misdetection_rate;

  unsigned int m_id;

  unsigned int m_numStateDimensions;

  double m_static_distance_threshold;
};

class HypothesisFactory
{
public:
  HypothesisFactory(){}
  virtual ~HypothesisFactory(){}

  virtual std::shared_ptr<Hypothesis> createHypothesis();
};

};

#endif //__HYPOTHESIS_H__
