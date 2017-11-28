/**
Software License Agreement (BSD)

\file      measurement_report_header_test.cpp
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


#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include "omron_os32c_driver/measurement_report_header.h"
#include "odva_ethernetip/serialization/serializable_buffer.h"
#include "odva_ethernetip/serialization/buffer_writer.h"
#include "odva_ethernetip/serialization/buffer_reader.h"

using namespace boost::asio;
using namespace omron_os32c_driver;
using namespace eip;
using namespace eip::serialization;
using namespace boost::asio;

class MeasurementReportHeaderTest : public :: testing :: Test
{

};

TEST_F(MeasurementReportHeaderTest, test_deserialize)
{
  EIP_BYTE d[] = {
    // scan count
    0xEF, 0xBE, 0xAD, 0xDE,
    // scan rate
    0x40, 0x9C, 0, 0,
    // scan timestamp
    0xAA, 0x55, 0xAA, 0x55,
    // scan beam period
    0x45, 0xA9, 0, 0,
    // machine state
    3, 0,
    // machine stop reason
    7, 0,
    // active zone set
    0x45, 0,
    // zone inputs
    0xAA, 0,
    // detection zone status
    0x0F, 0,
    // output status
    0x07, 0,
    // input status
    0x03, 0,
    // seven segment display
    0x02, 0x04,
    // non-safety config checksum
    0xAA, 0x55,
    // safety config checksum
    0xA5, 0x5A,
    // reserved bytes
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // range report format
    1, 0,
    // reflectivity report format
    1, 0,
    // reserved
    0xFF, 0xFF,
    // number of beams
    0xA5, 0x02,
  };

  BufferReader reader(buffer(d));
  MeasurementReportHeader mrh;
  mrh.deserialize(reader);
  EXPECT_EQ(sizeof(d), reader.getByteCount());
  EXPECT_EQ(sizeof(d), mrh.getLength());
  EXPECT_EQ(0xDEADBEEF, mrh.scan_count);
  EXPECT_EQ(40000, mrh.scan_rate);
  EXPECT_EQ(0x55AA55AA, mrh.scan_timestamp);
  EXPECT_EQ(43333, mrh.scan_beam_period);
  EXPECT_EQ(3, mrh.machine_state);
  EXPECT_EQ(7, mrh.machine_stop_reasons);
  EXPECT_EQ(0x45, mrh.active_zone_set);
  EXPECT_EQ(0xAA, mrh.zone_inputs);
  EXPECT_EQ(0x0F, mrh.detection_zone_status);
  EXPECT_EQ(7, mrh.output_status);
  EXPECT_EQ(3, mrh.input_status);
  EXPECT_EQ(0x0402, mrh.display_status);
  EXPECT_EQ(0x55AA, mrh.non_safety_config_checksum);
  EXPECT_EQ(0x5AA5, mrh.safety_config_checksum);
  EXPECT_EQ(1, mrh.range_report_format);
  EXPECT_EQ(1, mrh.reflectivity_report_format);
  EXPECT_EQ(677, mrh.num_beams);
}

TEST_F(MeasurementReportHeaderTest, test_serialize)
{
  MeasurementReportHeader mrh;
  mrh.scan_count = 0xDEADBEEF;
  mrh.scan_rate = 40000;
  mrh.scan_timestamp = 0x55AA55AA;
  mrh.scan_beam_period = 43333;
  mrh.machine_state = 3;
  mrh.machine_stop_reasons = 7;
  mrh.active_zone_set = 0x45;
  mrh.zone_inputs = 0xAA;
  mrh.detection_zone_status = 0x0F;
  mrh.output_status = 7;
  mrh.input_status = 3;
  mrh.display_status = 0x0402;
  mrh.non_safety_config_checksum = 0x55AA;
  mrh.safety_config_checksum = 0x5AA5;
  mrh.range_report_format = 1;
  mrh.reflectivity_report_format = 1;
  mrh.num_beams = 677;

  EIP_BYTE d[56];
  EXPECT_EQ(sizeof(d), mrh.getLength());
  BufferWriter writer(buffer(d));
  mrh.serialize(writer);
  EXPECT_EQ(sizeof(d), writer.getByteCount());

  EXPECT_EQ(d[0], 0xEF);
  EXPECT_EQ(d[1], 0xBE);
  EXPECT_EQ(d[2], 0xAD);
  EXPECT_EQ(d[3], 0xDE);
  EXPECT_EQ(d[4], 0x40);
  EXPECT_EQ(d[5], 0x9C);
  EXPECT_EQ(d[6], 0);
  EXPECT_EQ(d[7], 0);
  EXPECT_EQ(d[8], 0xAA);
  EXPECT_EQ(d[9], 0x55);
  EXPECT_EQ(d[10], 0xAA);
  EXPECT_EQ(d[11], 0x55);
  EXPECT_EQ(d[12], 0x45);
  EXPECT_EQ(d[13], 0xA9);
  EXPECT_EQ(d[14], 0);
  EXPECT_EQ(d[15], 0);
  EXPECT_EQ(d[16], 3);
  EXPECT_EQ(d[17], 0);
  EXPECT_EQ(d[18], 7);
  EXPECT_EQ(d[19], 0);
  EXPECT_EQ(d[20], 0x45);
  EXPECT_EQ(d[21], 0);
  EXPECT_EQ(d[22], 0xAA);
  EXPECT_EQ(d[23], 0);
  EXPECT_EQ(d[24], 0x0F);
  EXPECT_EQ(d[25], 0);
  EXPECT_EQ(d[26], 0x07);
  EXPECT_EQ(d[27], 0);
  EXPECT_EQ(d[28], 0x03);
  EXPECT_EQ(d[29], 0);
  EXPECT_EQ(d[30], 0x02);
  EXPECT_EQ(d[31], 0x04);
  EXPECT_EQ(d[32], 0xAA);
  EXPECT_EQ(d[33], 0x55);
  EXPECT_EQ(d[34], 0xA5);
  EXPECT_EQ(d[35], 0x5A);
  EXPECT_EQ(d[36], 0);
  EXPECT_EQ(d[37], 0);
  EXPECT_EQ(d[38], 0);
  EXPECT_EQ(d[39], 0);
  EXPECT_EQ(d[40], 0);
  EXPECT_EQ(d[41], 0);
  EXPECT_EQ(d[42], 0);
  EXPECT_EQ(d[43], 0);
  EXPECT_EQ(d[44], 0);
  EXPECT_EQ(d[45], 0);
  EXPECT_EQ(d[46], 0);
  EXPECT_EQ(d[47], 0);
  EXPECT_EQ(d[48], 1);
  EXPECT_EQ(d[49], 0);
  EXPECT_EQ(d[50], 1);
  EXPECT_EQ(d[51], 0);
  EXPECT_EQ(d[52], 0);
  EXPECT_EQ(d[53], 0);
  EXPECT_EQ(d[54], 0xA5);
  EXPECT_EQ(d[55], 0x02);
}


TEST_F(MeasurementReportHeaderTest, test_serialize_and_deserialize)
{
  MeasurementReportHeader mrh;
  mrh.scan_count = 0xDEADBEEF;
  mrh.scan_rate = 40000;
  mrh.scan_timestamp = 0x55AA55AA;
  mrh.scan_beam_period = 43333;
  mrh.machine_state = 3;
  mrh.machine_stop_reasons = 7;
  mrh.active_zone_set = 0x45;
  mrh.zone_inputs = 0xAA;
  mrh.detection_zone_status = 0x0F;
  mrh.output_status = 7;
  mrh.input_status = 3;
  mrh.display_status = 0x0402;
  mrh.non_safety_config_checksum = 0x55AA;
  mrh.safety_config_checksum = 0x5AA5;
  mrh.range_report_format = 1;
  mrh.reflectivity_report_format = 1;
  mrh.num_beams = 677;

  EIP_BYTE d[56];
  EXPECT_EQ(sizeof(d), mrh.getLength());
  BufferWriter writer(buffer(d));
  mrh.serialize(writer);
  EXPECT_EQ(sizeof(d), writer.getByteCount());

  BufferReader reader(buffer(d));
  MeasurementReportHeader mrh2;
  mrh2.deserialize(reader);
  EXPECT_EQ(sizeof(d), reader.getByteCount());
  EXPECT_EQ(sizeof(d), mrh2.getLength());
  EXPECT_EQ(0xDEADBEEF, mrh2.scan_count);
  EXPECT_EQ(40000, mrh2.scan_rate);
  EXPECT_EQ(0x55AA55AA, mrh2.scan_timestamp);
  EXPECT_EQ(43333, mrh2.scan_beam_period);
  EXPECT_EQ(3, mrh2.machine_state);
  EXPECT_EQ(7, mrh2.machine_stop_reasons);
  EXPECT_EQ(0x45, mrh2.active_zone_set);
  EXPECT_EQ(0xAA, mrh2.zone_inputs);
  EXPECT_EQ(0x0F, mrh2.detection_zone_status);
  EXPECT_EQ(7, mrh2.output_status);
  EXPECT_EQ(3, mrh2.input_status);
  EXPECT_EQ(0x0402, mrh2.display_status);
  EXPECT_EQ(0x55AA, mrh2.non_safety_config_checksum);
  EXPECT_EQ(0x5AA5, mrh2.safety_config_checksum);
  EXPECT_EQ(1, mrh2.range_report_format);
  EXPECT_EQ(1, mrh2.reflectivity_report_format);
  EXPECT_EQ(677, mrh2.num_beams);
}
