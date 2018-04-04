#include <multi_object_tracking/multi_object_tracker.h>

namespace MultiHypothesisTracker
{

Tracker::Tracker()
{
  ros::NodeHandle n("~");
  ros::NodeHandle pub_n;

  m_algorithm = std::make_shared<MultiObjectTrackerAlgorithm>();
  m_transform_listener = std::make_shared<tf::TransformListener>();

  m_laser_detection_subscriber = n.subscribe<geometry_msgs::PoseArray>("/object_poses", 30, &Tracker::detectionCallback, this);

  n.param<double>("m_merge_close_hypotheses_distance", m_merge_close_hypotheses_distance, 0.1);
  n.param<double>("m_max_mahalanobis_distance", m_max_mahalanobis_distance, 3.75);
  n.param<std::string>("m_world_frame", m_world_frame, "world");

  m_algorithm->setMergeDistance(m_merge_close_hypotheses_distance);
  m_algorithm->m_multi_hypothesis_tracker.setMaxMahalanobisDistance(m_max_mahalanobis_distance);
}

void Tracker::publish()
{
  m_mot_publisher.publishAll(m_algorithm->getHypotheses());
}

void Tracker::update()
{
  m_algorithm->predictWithoutMeasurement();
}

void Tracker::detectionCallback(const geometry_msgs::PoseArray::ConstPtr& msg)
{
  ROS_DEBUG_STREAM("Laser detection callback.");

  std::vector<Measurement> measurements;
  convert(msg, measurements);

  m_mot_publisher.publish_debug(m_algorithm->getHypotheses());

  if(!transformToFrame(measurements, m_world_frame))
    return;

  m_mot_publisher.publish_measurement_markers(measurements);
  m_mot_publisher.publish_measurement_covariance(measurements);

  m_algorithm->objectDetectionDataReceived(measurements);

  //Full track for first hypothesis
  // std::vector<Hypothesis*> hypotheses = m_algorithm->getHypotheses();
  // Hypothesis3D *hypothesis = (Hypothesis3D *) hypotheses[0];
  // const Eigen::Vector3d& mean = hypothesis->getMean();
  // geometry_msgs::Point p;
  // p.x=mean(0);
  // p.y=mean(1);
  // p.z=mean(2);
  // full_track.points.push_back(p);
  // m_track_linePublisher.publish(full_track);
}

void Tracker::convert(const geometry_msgs::PoseArray::ConstPtr &msg,
                      std::vector<Measurement>& measurements)
{
  Measurement measurement;

  for(size_t i = 0; i < msg->poses.size(); i++)
  {
    measurement.pos(0) = msg->poses[i].position.x;
    measurement.pos(1) = msg->poses[i].position.y;
    measurement.pos(2) = msg->poses[i].position.z;

    //TODO: radu: set covariance for the measurement to be dyamic depending on the altitude of the drone
    double measurementStd = 0.03;
    measurement.cov.setIdentity();
    measurement.cov(0, 0) = measurementStd * measurementStd;
    measurement.cov(1, 1) = measurementStd * measurementStd;
    measurement.cov(2, 2) = measurementStd * measurementStd;

    measurement.color = 'U'; // for unknown
    measurement.frame = msg->header.frame_id;
    measurement.time = msg->header.stamp.toSec();

    measurements.push_back(measurement);
  }
}

bool Tracker::transformToFrame(std::vector<Measurement>& measurements,
                               const std::string target_frame)
{
  for(auto& measurement : measurements)
  {
    geometry_msgs::PointStamped mes_in_origin_frame;
    geometry_msgs::PointStamped mes_in_target_frame;
    mes_in_origin_frame.header.frame_id = measurement.frame;
    mes_in_origin_frame.point.x = measurement.pos(0);
    mes_in_origin_frame.point.y = measurement.pos(1);
    mes_in_origin_frame.point.z = measurement.pos(2);

    // TODO:: add stamp for getting right transform?!

    try
    {
      m_transform_listener->transformPoint(target_frame, mes_in_origin_frame, mes_in_target_frame);
    }
    catch(tf::TransformException& ex)
    {
      ROS_ERROR("Received an exception trying to transform a point from \"%s\" to \"%s\"", mes_in_origin_frame.header.frame_id.c_str(), target_frame.c_str());
      return false;
    }

    measurement.pos(0) = mes_in_target_frame.point.x;
    measurement.pos(1) = mes_in_target_frame.point.y;
    measurement.pos(2) = mes_in_target_frame.point.z;
    measurement.frame = target_frame;
  }

  return true;
}

}


int main(int argc, char** argv)
{
  ros::init(argc, argv, "multi_object_tracking");
  ros::NodeHandle n;

  // TODO: no constant loop rate but rather one update with each detection callback
  ros::Rate loopRate(30);

  MultiHypothesisTracker::Tracker tracker;

  while(n.ok())
  {
    tracker.update();
    ros::spinOnce();
    tracker.publish();
    loopRate.sleep();
  }

  return 0;
}
