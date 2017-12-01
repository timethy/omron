/**
Software License Agreement (BSD)

\file      os32c.h
\authors   Kareem Shehata <kareem@shehata.ca>
\copyright Copyright (c) 2015, Clearpath Robotics, Inc., All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the
   following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
   following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of Clearpath Robotics nor the names of its contributors may be used to endorse or promote
   products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WAR-
RANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, IN-
DIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OMRON_OS32C_DRIVER_OS32C_H
#define OMRON_OS32C_DRIVER_OS32C_H

#include <gtest/gtest_prod.h>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <sensor_msgs/LaserScan.h>

#include "odva_ethernetip/session.h"
#include "odva_ethernetip/socket/socket.h"
#include "omron_os32c_driver/measurement_report.h"
#include "omron_os32c_driver/measurement_report_config.h"
#include "omron_os32c_driver/range_and_reflectance_measurement.h"

using std::vector;
using boost::shared_ptr;
using sensor_msgs::LaserScan;
using eip::Session;
using eip::socket::Socket;

#define DEG2RAD(a) (a * M_PI / 180.0)
#define RAD2DEG(a) (a * 180.0 / M_PI)

namespace omron_os32c_driver {


typedef enum
{
  NO_TOF_MEASUREMENTS       = 0,
  RANGE_MEASURE_50M         = 1,
  RANGE_MEASURE_32M_PZ      = 2,
  RANGE_MEASURE_16M_WZ1PZ   = 3,
  RANGE_MEASURE_8M_WZ2WZ1PZ = 4,
  RANGE_MEASURE_TOF_4PS     = 5,
} OS32C_RANGE_FORMAT;

typedef enum
{
  NO_TOT_MEASUREMENTS               = 0,
  REFLECTIVITY_MEASURE_TOT_ENCODED  = 1,
  REFLECTIVITY_MEASURE_TOT_4PS      = 2,
} OS32C_REFLECTIVITY_FORMAT;

/**
 * Main interface for the OS32C Laser Scanner. Produces methods to access the
 * laser scanner from a high level, including setting parameters and getting
 * single scans.
 */
class OS32C : public Session
{
public:
  /**
   * Construct a new OS32C instance.
   * @param socket Socket instance to use for communication with the lidar
   */
  OS32C(shared_ptr<Socket> socket, shared_ptr<Socket> io_socket)
    : Session(socket, io_socket), start_angle_(ANGLE_MAX), end_angle_(ANGLE_MIN),
      connection_num_(-1), mrc_sequence_num_(1)
  {
  }

  static const double ANGLE_MIN;
  static const double ANGLE_MAX;
  static const double ANGLE_INC;
  static const double DISTANCE_MIN;
  static const double DISTANCE_MAX;

  /**
   * Get the range format code. Does a Get Single Attribute to the scanner
   * to get the current range format.
   * @return range format code.
   * @see OS32C_RANGE_FORMAT
   */
  EIP_UINT getRangeFormat();

  /**
   * Set the range format code for the scanner. Does a Set Single Attribute to
   * the scanner to set the range format.
   * @param format The range format code to set
   * @see OS32C_REFLECTIVITY_FORMAT
   */
  void setRangeFormat(EIP_UINT format);

  /**
   * Get the reflectivity format code. Does a Get Single Attribute to the scanner
   * to get the current reflectivity format.
   * @return reflectivity format code.
   * @see OS32C_REFLECTIVITY_FORMAT
   */
  EIP_UINT getReflectivityFormat();

  /**
   * Set the reflectivity format code for the scanner. Does a Set Single Attribute to
   * the scanner to set the reflectivity format.
   * @param format The reflectivity format code to set
   * @see OS32C_RANGE_FORMAT
   */
  void setReflectivityFormat(EIP_UINT format);


  /**
   * Select which beams are to be measured. Must be set before requesting
   * measurements. Angles are in ROS conventions. Zero is straight ahead.
   * Positive numbers are CCW and all numbers are in radians.
   * @param start_angle Start angle in ROS conventions
   * @param end_angle End angle in ROS conventions
   */
  void selectBeams(double start_angle, double end_angle);

  /**
   * Make an explicit request for a single Range and Reflectance scan
   * @return Range and reflectance data received
   * @throw std::logic_error if data not received
   */
  RangeAndReflectanceMeasurement getSingleRRScan();

  /**
   * Calculate the beam number on the lidar for a given ROS angle. Note that
   * in ROS angles are given as radians CCW with zero being straight ahead,
   * While the lidar start its scan at the most CCW position and moves positive
   * CW, with zero being at halfway through the scan.
   * Based on my calculations and the OS32C docs, there are 677 beams and the scan
   * area is 135.4 to -135.4 degrees, with a 0.4 degree pitch. The beam centres
   * must then be at 135.2, 134.8, ... 0.4, 0, 0.4, ... -134.8, -135.2.
   * @param angle Radians CCW from straight ahead
   * @return OS32C beam number
   */
  static inline int calcBeamNumber(double angle)
  {
    return (ANGLE_MAX - angle + ANGLE_INC / 2) / ANGLE_INC;
  }

  /**
   * Calculate the ROS angle for a beam given the OS32C beam number
   * @param beam_num Beam number, starting with 0 being the most CCW beam and
   *  positive moving CW around the scan
   * @return ROS Angle
   */
  static inline double calcBeamCentre(int beam_num)
  {
    return ANGLE_MAX - beam_num * ANGLE_INC;
  }

  /**
   * Populate the unchanging parts of a ROS LaserScan, including the start_angle and stop_angle,
   * which are configured by the user but ultimately reported by the device.
   * @param ls Laserscan message to populate.
   */
  void fillLaserScanStaticConfig(sensor_msgs::LaserScan* ls);
 
  /**
   * Common helper
   * @param header
   * @param range_data
   * @param reflectance_data
   * @param ls
   */
  static void convertToLaserScan(const MeasurementReportHeader& header,
                                 const vector<EIP_UINT>& range_data,
                                 sensor_msgs::LaserScan* ls);

  /**
   * Helper to convert a Range and Reflectance Measurement to a ROS LaserScan. LaserScan
   * is passed as a pointer to avoid a bunch of memory allocation associated with resizing a vector
   * on each scan.
   * @param rr Measurement to convert
   * @param ls Laserscan message to populate.
   */
  static void convertToLaserScan(const RangeAndReflectanceMeasurement& rr, sensor_msgs::LaserScan* ls);

  /**
   * Helper to convert a Measurement Report to a ROS LaserScan
   * @param mr Measurement to convert
   * @param ls Laserscan message to populate.
   */
  static void convertToLaserScan(const MeasurementReport& mr, sensor_msgs::LaserScan* ls);

  void sendMeasurementReportConfigUDP();

  MeasurementReport receiveMeasurementReportUDP();
  RangeAndReflectanceMeasurement receiveRangeAndReflectanceUDP();

  void startUDPIO();

private:
  // allow unit tests to access the helpers below for direct testing
  FRIEND_TEST(OS32CTest, test_calc_beam_mask_all);
  FRIEND_TEST(OS32CTest, test_calc_beam_at_90);
  FRIEND_TEST(OS32CTest, test_calc_beam_boundaries);
  FRIEND_TEST(OS32CTest, test_calc_beam_invalid_args);
  FRIEND_TEST(OS32CTest, test_convert_to_laserscan);

  double start_angle_;
  double end_angle_;

  // data for sending to lidar to keep UDP session alive
  int connection_num_;
  MeasurementReportConfig mrc_;
  EIP_UDINT mrc_sequence_num_;

  /**
   * Helper to calculate the mask for a given start and end beam angle
   * @param start_angle Angle of the first beam in the scan
   * @param end_angle Angle of the last beam in the scan
   * @param mask Holder for the mask data. Must be 88 bytes
   */
  void calcBeamMask(double start_angle, double end_angle, EIP_BYTE mask[]);
};

} // namespace omron_os32c_driver

#endif  // OMRON_OS32C_DRIVER_OS32C_H
