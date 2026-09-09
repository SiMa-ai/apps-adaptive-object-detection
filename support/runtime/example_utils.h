#pragma once

#include <neat.h>
#include "neat/nodes.h"
#include "neat/models.h"

#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace sima_examples {

void require(bool cond, const std::string& msg);
double time_ms();

struct RtspProbeOptions {
  int payload_type = 96;
  int latency_ms = 200;
  bool rtsp_tcp = true;
  bool debug = false;
  int decoder_num_buffers = 7;
};

struct RtspStreamInfo {
  int width = 0;
  int height = 0;
  int fps = 0;
};

bool probe_rtsp_stream_info(const std::string& url, const RtspProbeOptions& opt,
                            RtspStreamInfo& out);
bool probe_rtsp_encoded(const std::string& url, const RtspProbeOptions& opt, int fps, int w, int h,
                        int tries, int timeout_ms, bool enforce_caps);
bool probe_rtsp_decoded_dims(const std::string& url, const RtspProbeOptions& opt, int tries,
                             int timeout_ms, int& out_w, int& out_h);

std::string prepare_yolo_boxdecode_config(const std::string& src_path,
                                          const std::filesystem::path& root, int img_w, int img_h,
                                          float conf = 0.5f, float nms = 0.5f);

bool infer_dims(const simaai::neat::Tensor& t, int& w, int& h);

bool nv12_to_bgr(const simaai::neat::Tensor& t, cv::Mat& out, std::string& err);
bool nv12_copy_to_cpu_tensor(const simaai::neat::Tensor& t, simaai::neat::Tensor& out,
                             std::string& err);

struct ScoredIndex {
  int index = -1;
  float value = 0.0f;
  float prob = 0.0f;
};

void check_top1(const std::vector<float>& scores, int expected_id, float min_prob,
                const std::string& label);

simaai::neat::Tensor pull_tensor_with_retry(simaai::neat::Run& run, const std::string& label,
                                            int per_try_ms, int tries);

std::string h264_gst_pipeline(const std::filesystem::path& out_path, int width, int height,
                              double fps, int bitrate_kbps = 4000);

bool open_h264_writer(cv::VideoWriter& writer, const std::filesystem::path& out_path, int width,
                      int height, double fps, int bitrate_kbps = 4000, std::string* err = nullptr);

bool extract_bbox_payload(const simaai::neat::Sample& result, std::vector<uint8_t>& payload,
                          std::string& err);

struct MetadataBox {
  std::string id;
  std::string label;
  float confidence = 0.0f;
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
};

std::string metadata_boxes_data_json(const std::string& array_key,
                                     const std::vector<MetadataBox>& boxes);

} // namespace sima_examples
