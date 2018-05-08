/** @file
 *
 * Simple rqt plugin to edit trajectories.
 *
 * @author Jan Razlaw
 */

#ifndef RQT_POSE_INTERPOLATOR_TRAJECTORY_EDITOR_H
#define RQT_POSE_INTERPOLATOR_TRAJECTORY_EDITOR_H

#include <fstream>

#include <ros/ros.h>
#include <ros/package.h>

#include <tf/tf.h>
#include <tf/transform_datatypes.h>

#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseArray.h>
#include <rviz_animated_view_controller/CameraMovement.h>
#include <rviz_animated_view_controller/CameraTrajectory.h>

#include <nav_msgs/Path.h>

#include <interactive_markers/interactive_marker_server.h>
#include <interactive_markers/menu_handler.h>

#include <rqt_gui_cpp/plugin.h>

#include <QWidget>
#include <QFileDialog>

#include <rqt_pose_interpolator/utils.h>
#include "ui_trajectory_editor.h"

#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>

#include <spline_library/splines/natural_spline.h>
#include <spline_library/splines/uniform_cr_spline.h>
#include <spline_library/vector.h>


namespace pose_interpolator {

/**
 * @brief Manipulates the rviz camera.
 */
class TrajectoryEditor : public rqt_gui_cpp::Plugin
{

Q_OBJECT
public:
  struct InteractiveMarkerWithTime
  {
    InteractiveMarkerWithTime(visualization_msgs::InteractiveMarker&& input_marker, const double time)
    : marker(input_marker)
      , transition_time(time)
    {
    }

    visualization_msgs::InteractiveMarker marker;
    double transition_time;
  };

  typedef InteractiveMarkerWithTime TimedMarker;
  typedef std::list<TimedMarker> MarkerList;


  /** @brief Constructor. */
  TrajectoryEditor();

  /**
   * @brief Sets up subscribers and publishers and connects GUI to functions.
   *
   * @param context     the plugin context.
   */
  virtual void initPlugin(qt_gui_cpp::PluginContext& context);

  /**
   * @brief Shuts down the subscribers and publishers.
   */
  virtual void shutdownPlugin();

  /**
   * @brief Saves settings. TODO.
   *
   * @param plugin_settings     plugin-specific settings
   * @param instance_settings   instance-specific settings
   */
  virtual void saveSettings(qt_gui_cpp::Settings& plugin_settings,
                            qt_gui_cpp::Settings& instance_settings) const;

  /**
   * @brief Restores settings. TODO.
   *
   * @param plugin_settings     plugin-specific settings
   * @param instance_settings   instance-specific settings
   */
  virtual void restoreSettings(const qt_gui_cpp::Settings& plugin_settings,
                               const qt_gui_cpp::Settings& instance_settings);

  /**
   * @brief Saves the current camera pose in #cam_pose_.
   *
   * @param[in] cam_pose    pointer to current camera pose.
   */
  void camPoseCallback(const geometry_msgs::Pose::ConstPtr& cam_pose);

Q_SIGNALS:
  void updateRequested();

public slots:
  /** @brief Moves rviz camera to currently selected pose.*/
  void moveCamToCurrent();
  /** @brief Moves rviz camera to pose before currently selected one.*/
  void moveCamToPrev();
  /** @brief Moves rviz camera subsequently to the first pose in the trajectory.*/
  void moveCamToFirst();
  /** @brief Moves rviz camera to pose after currently selected one.*/
  void moveCamToNext();
  /** @brief Moves rviz camera subsequently to the last pose in the trajectory.*/
  void moveCamToLast();
  /** @brief Update marker with values from GUI.*/
  void updateCurrentMarker();
  /** @brief Sets currently selected pose to the current pose of the rviz camera.*/
  void setCurrentPoseToCam();
  /** @brief Reconstructs trajectory from current markers. */
  void updateTrajectory();
  /** @brief Sets the frame_id of the markers.*/
  void setMarkerFrames();
  /** @brief Loads a series of markers from a file.*/
  void loadTrajectoryFromFile();
  /** @brief Saves poses and transition times of interactive markers to a file.*/
  void saveTrajectoryToFile();

private:
  /**
   * @brief Creates a CameraMovement hull.
   * @return CameraMovement.
   */
  rviz_animated_view_controller::CameraMovement makeCameraMovement();

  /**
   * @brief Creates an InteractiveMarker hull.
   *
   * @param[in] x   x position of marker.
   * @param[in] y   y position of marker.
   * @param[in] z   z position of marker.
   * @return InteractiveMarker.
   */
  visualization_msgs::InteractiveMarker makeMarker(double x=0.0,
                                                   double y=0.0,
                                                   double z=0.0);

  /**
   * @brief Fills a CameraMovement message with the values of a TimedMarker.
   *
   * @param[in]     marker          marker.
   * @param[out]    cam_movement    message.
   */
  void convertMarkerToCamMovement(const TimedMarker& marker,
                                  rviz_animated_view_controller::CameraMovement& cam_movement);

  /**
   * @brief Moves rviz camera to marker pose by publishing a CameraDisplacement message.
   *
   * @param[in] marker   provides the pose and transition time.
   */
  void moveCamToMarker(const TimedMarker& marker);

  /**
   * @brief Sets members to pose of currently moved interactive marker.
   * @param[in] feedback    feedback the interaction with the interactive marker generates.
   */
  void processFeedback(const visualization_msgs::InteractiveMarkerFeedbackConstPtr& feedback);

  /**
   * @brief Rotates a vector by a quaternion.
   *
   * @param[in] vector  input vector.
   * @param[in] quat    input rotation.
   * @return the rotated vector.
   */
  tf::Vector3 rotateVector(const tf::Vector3& vector,
                           const geometry_msgs::Quaternion& quat);

  /**
   * @brief Safes poses and times of markers to yaml file.
   *
   * @param[in] file_path   path to the file.
   */
  void safeTrajectoryToFile(const std::string& file_path);

  /**
   * @brief Publishes the trajectory that is defined by the markers.
   *
   * @param[in] feedback    feedback from selected marker.
   */
  void publishTrajectory(const visualization_msgs::InteractiveMarkerFeedbackConstPtr& feedback);

  /**
   * @brief Adds a marker between the selected marker and the one before.
   *
   * @param[in] feedback    feedback from selected marker.
   */
  void addMarkerBefore(const visualization_msgs::InteractiveMarkerFeedbackConstPtr& feedback);

  /**
   * @brief Adds a marker at the pose of the selected marker - useful for sharp edges in splines.
   *
   * @param[in] feedback    feedback from selected marker.
   */
  void addMarkerHere(const visualization_msgs::InteractiveMarkerFeedbackConstPtr& feedback);

  /**
   * @brief Adds a marker between the selected marker and the one after.
   *
   * @param[in] feedback    feedback from selected marker.
   */
  void addMarkerBehind(const visualization_msgs::InteractiveMarkerFeedbackConstPtr& feedback);

  /**
   * @brief Removes the selected marker.
   *
   * @param[in] feedback    feedback from selected marker.
   */
  void removeMarker(const visualization_msgs::InteractiveMarkerFeedbackConstPtr& feedback);

  /**
   * @brief Saves markers in server.
   *
   * @param[in] markers     markers.
   */
  void fillServer(MarkerList& markers);

  /**
   * @brief Load marker poses from a file.
   *
   * @param[in] nh          node handle.
   * @param[in] param_name  root name of the parameters.
   */
  void loadParams(const ros::NodeHandle& nh,
                  const std::string& param_name);

  /**
   * @brief Gets marker with specified name.
   *
   * @param[in] marker_name name of marker.
   * @return marker with marker_name.
   */
  InteractiveMarkerWithTime& getMarkerByName(const std::string& marker_name);

  /**
   * @brief Sets the input as the #current_marker_.
   *
   * Additionally updates the GUI elements and sets the color of the input marker from to green.
   *
   * @param[in] marker  input marker.
   */
  void setCurrentTo(TimedMarker& marker);

  /**
   * @brief Resets the #current_marker_ and dependencies from old_current to new_current.
   *
   * Dependencies are the marker member, the marker server and the GUI.
   *
   * @param[in,out] old_current  marker that was current before.
   * @param[in,out] new_current  marker that is current now.
   */
  void setCurrentFromTo(TimedMarker& old_current,
                        TimedMarker& new_current);

  /**
   * @brief Sets the Pose in the GUI to the provided input values.
   *
   * @param[in] pose            pose
   * @param[in] transition_time transition time
   */
  void updatePoseInGUI(const geometry_msgs::Pose& pose,
                       double transition_time=0.5);

  /**
   * @brief Sets the value of the spin_box to the value without triggering a signal.
   *
   * @param[in,out] spin_box    the updated spin box.
   * @param[in]     value       the value the spin box is set to.
   */
  void setValueQuietly(QDoubleSpinBox* spin_box, double value);

  /**
   * @brief Interpolate markers using a spline and safe that in spline_poses.
   *
   * @param[in]     markers         markers to be interpolated.
   * @param[out]    spline_poses    constructed spline.
   * @param[in]     frequency       resolution - number of spline points between each marker pair.
   */
  void splinify(const MarkerList& markers,
                std::vector<geometry_msgs::Pose>& spline_poses,
                std::vector<double>& spline_transition_times,
                double frequency,
                bool duplicate_start = true,
                bool duplicate_end = true);

  /** @brief Ui object - connection to GUI. */
  Ui::trajectory_editor ui_;
  /** @brief Widget. */
  QWidget* widget_;

  /** @brief Publishes camera trajectory messages. */
  ros::Publisher camera_trajectory_pub_;
  /** @brief Publishes the trajectory that is defined by the markers. */
  ros::Publisher view_poses_array_pub_;
  /** @brief Subscribes to the camera pose. */
  ros::Subscriber camera_pose_sub_;

  /** @brief Connects markers to callbacks. */
  interactive_markers::MenuHandler menu_handler_;
  /** @brief Stores markers - needed for #menu_handler. */
  std::shared_ptr<interactive_markers::InteractiveMarkerServer> server_;

  /** @brief Current camera pose. */
  geometry_msgs::Pose cam_pose_;

  /** @brief Currently selected marker. */
  TimedMarker current_marker_;

  /** @brief Currently maintained list of TimedMarkers. */
  MarkerList markers_;

  uint8_t interpolation_speed_;
};

} // namespace

#endif //RQT_POSE_INTERPOLATOR_TRAJECTORY_EDITOR_H
