/**
Software License Agreement (BSD)

\file      os32c.cpp
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


#include <ros/ros.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio.hpp>

#include "omron_os32c_driver/os32c.h"
#include "odva_ethernetip/serialization/serializable_buffer.h"
#include "odva_ethernetip/cpf_packet.h"
#include "odva_ethernetip/cpf_item.h"
#include "odva_ethernetip/sequenced_address_item.h"
#include "odva_ethernetip/sequenced_data_item.h"

using std::cout;
using std::endl;
using boost::shared_ptr;
using boost::make_shared;
using boost::asio::buffer;
using eip::Session;
using eip::serialization::SerializableBuffer;
using eip::RRDataResponse;
using eip::CPFItem;
using eip::CPFPacket;
using eip::SequencedAddressItem;
using eip::SequencedDataItem;
using omron_os32c_driver::RangeAndReflectanceMeasurement;

namespace omron_os32c_driver {

const double OS32C::ANGLE_MIN = DEG2RAD(-135.2);
const double OS32C::ANGLE_MAX = DEG2RAD( 135.2);
const double OS32C::ANGLE_INC = DEG2RAD(0.4);
const double OS32C::DISTANCE_MIN = 0.002;
const double OS32C::DISTANCE_MAX = 50;

EIP_UINT OS32C::getRangeFormat()
{
  mrc_.range_report_format = getSingleAttribute(0x73, 1, 4, (EIP_UINT)0);
  return mrc_.range_report_format;
}

void OS32C::setRangeFormat(EIP_UINT format)
{
  setSingleAttribute(0x73, 1, 4, format);
  mrc_.range_report_format = format;
}

EIP_UINT OS32C::getReflectivityFormat()
{
  mrc_.reflectivity_report_format = getSingleAttribute(0x73, 1, 5, (EIP_UINT)0);
  return mrc_.reflectivity_report_format;
}

void OS32C::setReflectivityFormat(EIP_UINT format)
{
  setSingleAttribute(0x73, 1, 5, format);
  mrc_.reflectivity_report_format = format;
}

void OS32C::calcBeamMask(double start_angle, double end_angle, EIP_BYTE mask[])
{
  if (start_angle > (ANGLE_MAX + ANGLE_INC / 2))
  {
    throw std::invalid_argument("Start angle is greater than max");
  }
  if (end_angle < (ANGLE_MIN - ANGLE_INC / 2))
  {
    throw std::invalid_argument("End angle is greater than max");
  }
  if (start_angle - end_angle <= ANGLE_INC)
  {
    throw std::invalid_argument("Starting angle is less than ending angle");
  }

  int start_beam = 0; //calcBeamNumber(start_angle);
  int end_beam = 676; //calcBeamNumber(end_angle);
  start_angle_ = calcBeamCentre(start_beam);
  end_angle_ = calcBeamCentre(end_beam);

  // figure out where we're starting and ending in the array
  int start_byte = start_beam / 8;
  int start_bit = start_beam - start_byte * 8;
  int end_byte = end_beam / 8;
  int end_bit = end_beam - end_byte * 8;

  // clear out all of the whole bytes that need to be set in one shot
  if (start_byte > 0)
  {
    memset(mask, 0, start_byte);
  }

  // if we're starting in the middle of the byte, then we need to do some bit math
  if (start_bit)
  {
    // clear out the bits at the start of the byte, while making the higher bits 1
    mask[start_byte] = ~((1 << start_bit) - 1);
  }
  else
  {
    --start_byte;
  }

  // mark all of the whole bytes
  memset(mask + start_byte + 1, 0xFF, end_byte - start_byte - 1);

  // clear out the bits above the end beam
  mask[end_byte] = (1 << (end_bit + 1)) - 1;

  // zero out everything after the end
  memset(mask + end_byte + 1, 0, 87 - end_byte);
}

void OS32C::selectBeams(double start_angle, double end_angle)
{
  calcBeamMask(start_angle, end_angle, mrc_.beam_selection_mask);
  shared_ptr<SerializableBuffer> sb = make_shared<SerializableBuffer>(
    buffer(mrc_.beam_selection_mask));
  setSingleAttributeSerializable(0x73, 1, 12, sb);
}

RangeAndReflectanceMeasurement OS32C::getSingleRRScan()
{
  RangeAndReflectanceMeasurement rr;
  getSingleAttributeSerializable(0x75, 1, 3, rr);
  return rr;
}

void OS32C::fillLaserScanStaticConfig(sensor_msgs::LaserScan* ls)
{
  ls->angle_max = end_angle_;
  ls->angle_min = start_angle_;
  ls->angle_increment = -ANGLE_INC;
  ls->range_min = DISTANCE_MIN;
  ls->range_max = DISTANCE_MAX;
}

void OS32C::convertToLaserScan(const RangeAndReflectanceMeasurement& rr, sensor_msgs::LaserScan* ls)
{
  if (rr.range_data.size() != rr.header.num_beams ||
    rr.reflectance_data.size() != rr.header.num_beams)
  {
    throw std::invalid_argument("Number of beams does not match vector size");
  }

  // Beam period is in ns
  ls->time_increment = rr.header.scan_beam_period / 1000000000.0;
  // Scan period is in microseconds.
  ls->scan_time = rr.header.scan_rate / 1000000.0;

  // TODO: this currently makes assumptions of the report format. Should likely
  // accomodate all of them, or at least anything reasonable.
  ls->ranges.resize(rr.header.num_beams);
  ls->intensities.resize(rr.header.num_beams);
  for (int i = 0; i < rr.header.num_beams; ++i)
  {
    if (rr.range_data[i] == 0x0001)
    {
      // noisy beam detected
      ls->ranges[i] = 0;
    }
    else if (rr.range_data[i] == 0xFFFF)
    {
      // no return
      ls->ranges[i] = DISTANCE_MAX;
    }
    else
    {
      ls->ranges[i] = rr.range_data[i] / 1000.0;
    }
    ls->intensities[i] = rr.reflectance_data[i];
  }
}

void OS32C::convertToLaserScan(const MeasurementReport& mr, sensor_msgs::LaserScan* ls)
{
  if (mr.measurement_data.size() != mr.header.num_beams)
  {
    throw std::invalid_argument("Number of beams does not match vector size");
  }

  // Beam period is in ns.
  ls->time_increment = mr.header.scan_beam_period / 1000000000.0;
  // Scan period is in microseconds.
  ls->scan_time = mr.header.scan_rate / 1000000.0;

  // TODO: this currently makes assumptions of the report format. Should likely
  // accomodate all of them, or at least anything reasonable.
  ls->ranges.resize(mr.header.num_beams);
  for (int i = 0; i < mr.header.num_beams; ++i)
  {
    if (mr.measurement_data[i] == 0x0001)
    {
      // noisy beam detected
      ls->ranges[i] = 0.0;
    }
    else if (mr.measurement_data[i] == 0xFFFF)
    {
      // no return
      ls->ranges[i] = DISTANCE_MAX;
    }
    else
    {
      if (mr.header.range_report_format == RANGE_MEASURE_TOF_4PS) {
        double static METER_PER_8PS = 0.00119916983 * 0.5;
        const EIP_UINT tof = mr.measurement_data[i]; // x 4ps
        ls->ranges[i] = static_cast<float>(METER_PER_8PS * tof);
      } else
      {
        ls->ranges[i] = mr.measurement_data[i] / 1000.0f;
      }
    }
  }
}

void OS32C::sendMeasurementReportConfigUDP()
{
  // TODO: check that connection is valid
  CPFPacket pkt;
  shared_ptr<SequencedAddressItem> address =
    make_shared<SequencedAddressItem>(
      getConnection(connection_num_).o_to_t_connection_id, mrc_sequence_num_++);
  shared_ptr<MeasurementReportConfig> data = make_shared<MeasurementReportConfig>();
  *data = mrc_;
  pkt.getItems().push_back(CPFItem(0x8002, address));
  pkt.getItems().push_back(CPFItem(0x00B1, data));
  sendIOPacket(pkt);
}

MeasurementReport OS32C::receiveMeasurementReportUDP()
{
  CPFPacket pkt = receiveIOPacket();
  if (pkt.getItemCount() != 2)
  {
    throw std::logic_error("IO Packet received with wrong number of items");
  }
  if (pkt.getItems()[1].getItemType() != 0x00B1)
  {
    throw std::logic_error("IO Packet received with wrong data type");
  }

  SequencedDataItem<MeasurementReport> data;
  pkt.getItems()[1].getDataAs(data);
  return data;
}

void OS32C::startUDPIO()
{
  EIP_CONNECTION_INFO_T o_to_t, t_to_o;
  o_to_t.assembly_id = 0x71;
  o_to_t.buffer_size = 0x006E;
  o_to_t.rpi = 0x00177FA0;
  t_to_o.assembly_id = 0x66;
  t_to_o.buffer_size = 0x0584;
  t_to_o.rpi = 40000; //0x00013070;

  connection_num_ = createConnection(o_to_t, t_to_o);
}

} // namespace os32c
