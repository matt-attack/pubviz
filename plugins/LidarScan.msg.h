#pragma once

#include <pubsub/Serialization.h>
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1)
struct lidar_processing__LidarScan
{
  uint8_t lidar_id[16];
  float lidar_x;
  float lidar_y;
  float lidar_z;
  float lidar_pitch;
  float lidar_roll;
  float lidar_yaw;
  double vehicle_x;
  double vehicle_y;
  double vehicle_z;
  double vehicle_rx;
  double vehicle_ry;
  double vehicle_rz;
  double vehicle_rw;
  uint32_t num_points;
  uint8_t* points;
  uint32_t points_length;
};

#pragma pack(pop)
struct ps_msg_field_t lidar_processing__LidarScan_fields[] = {
  { FT_UInt8, FF_NONE, "lidar_id", 16, 0 }, 
  { FT_Float32, FF_NONE, "lidar_x", 1, 0 }, 
  { FT_Float32, FF_NONE, "lidar_y", 1, 0 }, 
  { FT_Float32, FF_NONE, "lidar_z", 1, 0 }, 
  { FT_Float32, FF_NONE, "lidar_pitch", 1, 0 }, 
  { FT_Float32, FF_NONE, "lidar_roll", 1, 0 }, 
  { FT_Float32, FF_NONE, "lidar_yaw", 1, 0 }, 
  { FT_Float64, FF_NONE, "vehicle_x", 1, 0 }, 
  { FT_Float64, FF_NONE, "vehicle_y", 1, 0 }, 
  { FT_Float64, FF_NONE, "vehicle_z", 1, 0 }, 
  { FT_Float64, FF_NONE, "vehicle_rx", 1, 0 }, 
  { FT_Float64, FF_NONE, "vehicle_ry", 1, 0 }, 
  { FT_Float64, FF_NONE, "vehicle_rz", 1, 0 }, 
  { FT_Float64, FF_NONE, "vehicle_rw", 1, 0 }, 
  { FT_UInt32, FF_NONE, "num_points", 1, 0 }, 
  { FT_UInt8, FF_NONE, "points", 0, 0 }, 
};

void* lidar_processing__LidarScan_decode(const void* data, struct ps_allocator_t* allocator)
{
  char* p = (char*)data;
  int len = sizeof(struct lidar_processing__LidarScan);
  struct lidar_processing__LidarScan* out = (struct lidar_processing__LidarScan*)allocator->alloc(len, allocator->context);
  memcpy(out->lidar_id, p, sizeof(uint8_t)*16);
  p += sizeof(uint8_t)*16;
  out->lidar_x = *((float*)p);
  p += sizeof(float);
  out->lidar_y = *((float*)p);
  p += sizeof(float);
  out->lidar_z = *((float*)p);
  p += sizeof(float);
  out->lidar_pitch = *((float*)p);
  p += sizeof(float);
  out->lidar_roll = *((float*)p);
  p += sizeof(float);
  out->lidar_yaw = *((float*)p);
  p += sizeof(float);
  out->vehicle_x = *((double*)p);
  p += sizeof(double);
  out->vehicle_y = *((double*)p);
  p += sizeof(double);
  out->vehicle_z = *((double*)p);
  p += sizeof(double);
  out->vehicle_rx = *((double*)p);
  p += sizeof(double);
  out->vehicle_ry = *((double*)p);
  p += sizeof(double);
  out->vehicle_rz = *((double*)p);
  p += sizeof(double);
  out->vehicle_rw = *((double*)p);
  p += sizeof(double);
  out->num_points = *((uint32_t*)p);
  p += sizeof(uint32_t);
  int len_points = *(uint32_t*)p;
  p += 4;
  out->points_length = len_points;
  out->points = (uint8_t*)allocator->alloc(len_points*sizeof(uint8_t), allocator->context);
  memcpy(out->points, p, len_points*sizeof(uint8_t));
  p += len_points*sizeof(uint8_t);
  return (void*)out;
}

struct ps_msg_t lidar_processing__LidarScan_encode(struct ps_allocator_t* allocator, const void* data)
{
  const struct lidar_processing__LidarScan* msg = (const struct lidar_processing__LidarScan*)data;
  int len = sizeof(struct lidar_processing__LidarScan);
  // calculate the encoded length of the message
  len += msg->points_length*sizeof(uint8_t);
  len -= 1*sizeof(char*);
  struct ps_msg_t omsg;
  ps_msg_alloc(len, allocator, &omsg);
  char* start = (char*)ps_get_msg_start(omsg.data);
  memcpy(start, msg->lidar_id, sizeof(uint8_t)*16);
  start += sizeof(uint8_t)*16;
  memcpy(start, &msg->lidar_x, sizeof(float));
  start += sizeof(float);
  memcpy(start, &msg->lidar_y, sizeof(float));
  start += sizeof(float);
  memcpy(start, &msg->lidar_z, sizeof(float));
  start += sizeof(float);
  memcpy(start, &msg->lidar_pitch, sizeof(float));
  start += sizeof(float);
  memcpy(start, &msg->lidar_roll, sizeof(float));
  start += sizeof(float);
  memcpy(start, &msg->lidar_yaw, sizeof(float));
  start += sizeof(float);
  memcpy(start, &msg->vehicle_x, sizeof(double));
  start += sizeof(double);
  memcpy(start, &msg->vehicle_y, sizeof(double));
  start += sizeof(double);
  memcpy(start, &msg->vehicle_z, sizeof(double));
  start += sizeof(double);
  memcpy(start, &msg->vehicle_rx, sizeof(double));
  start += sizeof(double);
  memcpy(start, &msg->vehicle_ry, sizeof(double));
  start += sizeof(double);
  memcpy(start, &msg->vehicle_rz, sizeof(double));
  start += sizeof(double);
  memcpy(start, &msg->vehicle_rw, sizeof(double));
  start += sizeof(double);
  memcpy(start, &msg->num_points, sizeof(uint32_t));
  start += sizeof(uint32_t);
  *(uint32_t*)start = msg->points_length;
  start += 4;
  memcpy(start, msg->points, msg->points_length*sizeof(uint8_t));
  start += msg->points_length*sizeof(uint8_t);
  return omsg;
}
struct ps_message_definition_t lidar_processing__LidarScan_def = { 3568689580, "lidar_processing__LidarScan", 16, lidar_processing__LidarScan_fields, lidar_processing__LidarScan_encode, lidar_processing__LidarScan_decode, 0, 0 };

#ifdef __cplusplus
#include <memory>
namespace lidar_processing
{
namespace msg
{
struct LidarScan: public lidar_processing__LidarScan
{
  ~LidarScan()
  {
    if (this->lidar_id)
      free(this->lidar_id);
    if (this->points)
      free(this->points);
  }

  LidarScan(const LidarScan& obj)
  {
    lidar_x = obj.lidar_x; 
    lidar_y = obj.lidar_y; 
    lidar_z = obj.lidar_z; 
    lidar_pitch = obj.lidar_pitch; 
    lidar_roll = obj.lidar_roll; 
    lidar_yaw = obj.lidar_yaw; 
    vehicle_x = obj.vehicle_x; 
    vehicle_y = obj.vehicle_y; 
    vehicle_z = obj.vehicle_z; 
    vehicle_rx = obj.vehicle_rx; 
    vehicle_ry = obj.vehicle_ry; 
    vehicle_rz = obj.vehicle_rz; 
    vehicle_rw = obj.vehicle_rw; 
    num_points = obj.num_points; 
    points = (uint8_t*)malloc(obj.points_length*sizeof(uint8_t));
    points_length = obj.points_length;
    memcpy(points, obj.points, obj.points_length*sizeof(uint8_t));
  }

  LidarScan& operator=(const LidarScan& obj)
  {
    if (this->lidar_id)
      free(this->lidar_id);
    if (this->points)
      free(this->points);

    lidar_x = obj.lidar_x; 
    lidar_y = obj.lidar_y; 
    lidar_z = obj.lidar_z; 
    lidar_pitch = obj.lidar_pitch; 
    lidar_roll = obj.lidar_roll; 
    lidar_yaw = obj.lidar_yaw; 
    vehicle_x = obj.vehicle_x; 
    vehicle_y = obj.vehicle_y; 
    vehicle_z = obj.vehicle_z; 
    vehicle_rx = obj.vehicle_rx; 
    vehicle_ry = obj.vehicle_ry; 
    vehicle_rz = obj.vehicle_rz; 
    vehicle_rw = obj.vehicle_rw; 
    num_points = obj.num_points; 
    points = (uint8_t*)malloc(obj.points_length*sizeof(uint8_t));
    points_length = obj.points_length;
    memcpy(points, obj.points, obj.points_length*sizeof(uint8_t));
    return *this;
  }

  LidarScan()
  {
    points = 0;
    points_length = 0;
  }

  static const ps_message_definition_t* GetDefinition()
  {
    return &lidar_processing__LidarScan_def;
  }

  ps_msg_t Encode() const
  {
    return lidar_processing__LidarScan_encode(&ps_default_allocator, this);
  }

  static LidarScan* Decode(const void* data)
  {
    return (LidarScan*)lidar_processing__LidarScan_decode(data, &ps_default_allocator);
  }
};
typedef std::shared_ptr<LidarScan> LidarScanSharedPtr;
}
}
#endif
