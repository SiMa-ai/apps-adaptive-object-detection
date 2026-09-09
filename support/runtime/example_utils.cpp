#include "example_utils.h"

#include "ffprobe_command.h"
#include "support/object_detection/obj_detection_utils.h"

#include <neat.h>
#include "neat/nodes.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace sima_examples {

namespace {

int fps_from_rate(const std::string& value) {
  if (value.empty() || value == "0/0" || value == "0/1")
    return 0;
  try {
    const auto slash = value.find('/');
    double fps = 0.0;
    if (slash == std::string::npos) {
      fps = std::stod(value);
    } else {
      const double den = std::stod(value.substr(slash + 1));
      if (den <= 0.0)
        return 0;
      fps = std::stod(value.substr(0, slash)) / den;
    }
    return fps > 0.0 ? static_cast<int>(std::lround(fps)) : 0;
  } catch (...) {
    return 0;
  }
}

void fill_missing_stream_info(RtspStreamInfo& dst, const RtspStreamInfo& src) {
  if (dst.width <= 0)
    dst.width = src.width;
  if (dst.height <= 0)
    dst.height = src.height;
  if (dst.fps <= 0)
    dst.fps = src.fps;
}

RtspStreamInfo probe_ffprobe_rtsp_stream_info(const std::string& url, bool rtsp_tcp) {
  RtspStreamInfo info;
  const std::string command = build_ffprobe_rtsp_stream_info_command(url, rtsp_tcp);

  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) {
    return info;
  }

  int avg_fps = 0;
  int r_fps = 0;
  std::array<char, 256> buffer{};
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
    std::string line(buffer.data());
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
      line.pop_back();
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      continue;
    }
    const std::string key = line.substr(0, eq);
    const std::string value = line.substr(eq + 1);
    if (key == "width") {
      info.width = std::atoi(value.c_str());
    } else if (key == "height") {
      info.height = std::atoi(value.c_str());
    } else if (key == "avg_frame_rate") {
      avg_fps = fps_from_rate(value);
    } else if (key == "r_frame_rate") {
      r_fps = fps_from_rate(value);
    }
  }
  pclose(pipe);
  info.fps = avg_fps > 0 ? avg_fps : r_fps;
  return info;
}

RtspStreamInfo probe_opencv_rtsp_stream_info(const std::string& url) {
  RtspStreamInfo info;
  cv::VideoCapture cap(url);
  if (!cap.isOpened()) {
    return info;
  }
  info.width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
  info.height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
  info.fps = static_cast<int>(std::lround(cap.get(cv::CAP_PROP_FPS)));
  cap.release();
  return info;
}

std::string trim_copy(const std::string& s) {
  const std::string whitespace = " \t\r\n";
  const size_t start = s.find_first_not_of(whitespace);
  if (start == std::string::npos)
    return "";
  const size_t end = s.find_last_not_of(whitespace);
  return s.substr(start, end - start + 1);
}

} // namespace

void require(bool cond, const std::string& msg) {
  if (!cond)
    throw std::runtime_error(msg);
}

double time_ms() {
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

bool probe_rtsp_stream_info(const std::string& url, const RtspProbeOptions& opt,
                            RtspStreamInfo& out) {
  out = RtspStreamInfo{};
  fill_missing_stream_info(out, probe_ffprobe_rtsp_stream_info(url, opt.rtsp_tcp));
  fill_missing_stream_info(out, probe_opencv_rtsp_stream_info(url));

  return out.width > 0 && out.height > 0;
}

bool infer_dims(const simaai::neat::Tensor& t, int& w, int& h) {
  w = t.width();
  h = t.height();
  if ((w <= 0 || h <= 0) && t.shape.size() >= 2) {
    h = static_cast<int>(t.shape[0]);
    w = static_cast<int>(t.shape[1]);
  }
  return (w > 0 && h > 0);
}

bool nv12_to_bgr(const simaai::neat::Tensor& t, cv::Mat& out, std::string& err) {
  if (!t.is_nv12()) {
    err = "expected NV12 tensor";
    return false;
  }
  int w = 0;
  int h = 0;
  if (!infer_dims(t, w, h)) {
    err = "invalid tensor dimensions";
    return false;
  }
  std::vector<uint8_t> nv12 = t.copy_nv12_contiguous();
  if (nv12.empty()) {
    err = "NV12 copy failed";
    return false;
  }
  cv::Mat yuv(h + h / 2, w, CV_8UC1, nv12.data());
  cv::cvtColor(yuv, out, cv::COLOR_YUV2BGR_NV12);
  return true;
}

bool extract_bbox_payload(const simaai::neat::Sample& result, std::vector<uint8_t>& payload,
                          std::string& err) {
  return objdet::extract_bbox_payload(result, payload, err);
}

} // namespace sima_examples
