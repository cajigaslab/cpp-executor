#include <thalamus/tracing.hpp>
#include <iostream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <boost/asio.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>

#include <chrono>

//#include "tools/fonts/TestFontMgr.h"
#include "tools/fonts/FontToolUtils.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "src/gpu/ganesh/gl/GrGLDefines.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkData.h"
#include "include/core/SkImage.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPath.h"
#include "include/core/SkPaint.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkRect.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkFontMgr.h"
#include "include/ports/SkTypeface_win.h"
#include "include/encode/SkPngEncoder.h"
//#include "include/ports/SkFontMgr_fontconfig.h"
//#include "include/ports/SkFontScanner_FreeType.h"
#include "json/json.h"

#include <grpcpp/grpcpp.h>
#include <thalamus.pb.h>
#include <thalamus.grpc.pb.h>
#include <task_controller.pb.h>
#include <task_controller.grpc.pb.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <task.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __clang__
PERFETTO_TRACK_EVENT_STATIC_STORAGE();
#endif

using namespace std::chrono_literals;
using namespace std::placeholders;

static std::chrono::steady_clock::time_point first_start;

//void draw(SkCanvas* canvas) {
//	const SkScalar scale = 256.0f;
//	const SkScalar R = 0.45f * scale;
//	const SkScalar TAU = 6.2831853f;
//	SkPath path;
//	path.moveTo(R, 0.0f);
//	for (int i = 1; i < 7; ++i) {
//    	SkScalar theta = 3 * i * TAU / 7;
//    	path.lineTo(R * cos(theta), R * sin(theta));
//	}
//	path.close();
//	SkPaint p;
//	p.setAntiAlias(true);
//	canvas->clear(SK_ColorBLUE);
//	//canvas->translate(0.5f * scale, 0.5f * scale);
//	canvas->rotate(90e-3 * std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - first_start).count());
//	canvas->drawPath(path, p);
//}
//
//void raster(int width, int height,
//        	void (*draw)(SkCanvas*),
//        	const char* path) {
//	sk_sp<SkSurface> rasterSurface(SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height)));
//	SkCanvas* rasterCanvas = rasterSurface->getCanvas();
//	draw(rasterCanvas);
//	sk_sp<SkImage> img(rasterSurface->makeImageSnapshot());
//	if (!img) { return; }
//	sk_sp<SkData> png = SkPngEncoder::Encode(nullptr, img.get(), {});
//	if (!png) { return; }
//	SkFILEWStream out(path);
//	(void)out.write(png->data(), png->size());
//}

template<typename T>
class WriteReactor : public grpc::ClientWriteReactor<T> {
public:
  std::mutex mutex;
  std::condition_variable condition;
  std::list<T> out;
  grpc::ClientContext context;
  bool done = false;
  bool writing = false;
  ~WriteReactor() override {
    //grpc::ClientBidiReactor<task_controller_grpc::TaskResult, task_controller_grpc::TaskConfig>::StartWritesDone();
    context.TryCancel();
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&] { return done; });
  }
  void start() {
    grpc::ClientWriteReactor<T>::StartCall();
  }
  void OnWriteDone(bool ok) override {
    if(!ok) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
      }
      condition.notify_all();
      return;
    }
    std::lock_guard<std::mutex> lock(mutex);
    out.pop_front();
    if(!out.empty()) {
      grpc::ClientWriteReactor<T>::StartWrite(&out.front());
    } else {
      writing = false;
    }
  }
  void write(T&& message) {
    std::lock_guard<std::mutex> lock(mutex);
    out.push_back(std::move(message));
    if(!writing) {
      grpc::ClientWriteReactor<T>::StartWrite(&out.front());
      writing = true;
    }
  }
  void OnDone(const grpc::Status&) override {
    std::cout << "OnDone" << std::endl;
    {
      std::lock_guard<std::mutex> lock(mutex);
      done = true;
    }
    condition.notify_all();
  }
};

template<typename T>
class ReadReactor : public grpc::ClientReadReactor<T> {
public:
  std::mutex mutex;
  std::condition_variable condition;
  T in;
  grpc::ClientContext context;
  bool done = false;
  boost::asio::io_context& io_context;
  std::function<void(const T&)> callback;
  ReadReactor(boost::asio::io_context& _io_context, const std::function<void(const T&)>& _callback) : io_context(_io_context), callback(_callback) {}
  ~ReadReactor() override {
    //grpc::ClientBidiReactor<task_controller_grpc::TaskResult, task_controller_grpc::TaskConfig>::StartWritesDone();
    context.TryCancel();
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&] { return done; });
  }
  void start() {
    grpc::ClientReadReactor<T>::StartRead(&in);
    grpc::ClientReadReactor<T>::StartCall();
  }
  void OnReadDone(bool ok) override {
    if(!ok) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
      }
      condition.notify_all();
      return;
    }
    boost::asio::post(io_context, [&,c_in=std::move(in)] {
      callback(c_in);
    });
    grpc::ClientReadReactor<T>::StartRead(&in);
  }
  void OnDone(const grpc::Status&) override {
    std::cout << "OnDone" << std::endl;
    {
      std::lock_guard<std::mutex> lock(mutex);
      done = true;
    }
    condition.notify_all();
  }
};

static SDL_Window *mainwindow; /* Our window handle */
static SDL_Window * operatorwindow; /* Our window handle */
static SDL_GLContext maincontext; /* Our opengl context handle */
static SDL_GLContext operatorcontext; /* Our opengl context handle */

static void proj_assert(bool assertion, const std::string& message) {
  if(!assertion) {
    std::cerr << message;
    std::abort();
  }
}

struct Window {
  SDL_Window* sdl_window;
  SDL_GLContext sdl_context;
  sk_sp<const GrGLInterface> gl_face = nullptr;
  sk_sp<GrDirectContext> context = nullptr;
  sk_sp<SkSurface> surface = nullptr;
  SkSurfaceProps props = SkSurfaceProps();
  SkCanvas* gpuCanvas = nullptr;
  int width = 0, height = 0;
  sk_sp<SkSurface> fb_surface = nullptr;
  SkCanvas* fb_gpuCanvas = nullptr;
  SkSurfaceProps fb_props = SkSurfaceProps();

  void clean() {
    if (surface) {
      surface.reset();
    }
    if (context) {
      context->abandonContext();
      context.reset();
    }
    if (gl_face) {
      gl_face.reset();
    }
    if (fb_surface) {
      fb_surface.reset();
    }
  }

  void rebuild(int new_width, int new_height, GrGLuint buffer = std::numeric_limits<GrGLuint>::max()) {
    this->width = new_width;
    this->height = new_height;
    bool success = SDL_GL_MakeCurrent(sdl_window, sdl_context);
    proj_assert(success, "SDL_GL_MakeCurrent failed");
    if (surface) {
      surface.reset();
    }
    if (context) {
      context->abandonContext();
      context.reset();
    }
    if (gl_face) {
      gl_face.reset();
    }

    gl_face = GrGLMakeNativeInterface();
    context = GrDirectContexts::MakeGL(gl_face);

    GrGLFramebufferInfo info;
    info.fFBOID = 0;
    info.fFormat = GR_GL_RGB8;

    GrBackendRenderTarget target = GrBackendRenderTargets::MakeGL(width, height, 0, 8, info);

    surface = SkSurfaces::WrapBackendRenderTarget(context.get(), target,
      kBottomLeft_GrSurfaceOrigin,
      kRGB_888x_SkColorType, nullptr, &props);

    gpuCanvas = surface->getCanvas();

    if (buffer != std::numeric_limits<GrGLuint>::max()) {
      GrGLFramebufferInfo fbInfo;
      fbInfo.fFBOID = static_cast<GrGLuint>(buffer);
      fbInfo.fFormat = GR_GL_RGB8;

      GrBackendRenderTarget fbTarget = GrBackendRenderTargets::MakeGL(width, height, 0, 8, fbInfo);

      fb_surface = SkSurfaces::WrapBackendRenderTarget(context.get(), fbTarget,
        kBottomLeft_GrSurfaceOrigin,
        kRGB_888x_SkColorType, nullptr, &fb_props);

      fb_gpuCanvas = fb_surface->getCanvas();
    }
   }
};

struct FrameBuffer {
  const GrGLInterface::Functions* fFunctions = nullptr;
  GrGLuint buffer = 0;
  GrGLuint texture = 0;
  FrameBuffer(const FrameBuffer&) = delete;
  FrameBuffer(FrameBuffer&& other) {
    fFunctions = other.fFunctions;
    take(std::move(other));
  }
  FrameBuffer& operator=(FrameBuffer&& other) {
    fFunctions = other.fFunctions;
    clear();
    take(std::move(other));
    return *this;
  }
  FrameBuffer(const GrGLInterface::Functions* _fFunctions, int width, int height)
    : fFunctions(_fFunctions) {
    auto glerr = fFunctions->fGetError();
    proj_assert(glerr == 0, "GL Error");
    fFunctions->fGenFramebuffers(1, &buffer);
    glerr = fFunctions->fGetError();
    fFunctions->fBindFramebuffer(GR_GL_FRAMEBUFFER, buffer);
    glerr = fFunctions->fGetError();
    // The texture we're going to render to
    fFunctions->fGenTextures(1, &texture);
    glerr = fFunctions->fGetError();

    // "Bind" the newly created texture : all future texture functions will modify this texture
    fFunctions->fBindTexture(GR_GL_TEXTURE_2D, texture);
    glerr = fFunctions->fGetError();

    // Give an empty image to OpenGL ( the last "0" )
    fFunctions->fTexImage2D(GR_GL_TEXTURE_2D, 0, GR_GL_RGB, width, height, 0, GR_GL_RGB, GR_GL_UNSIGNED_BYTE, nullptr);
    glerr = fFunctions->fGetError();

    // Poor filtering. Needed !
    fFunctions->fTexParameteri(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_MAG_FILTER, GR_GL_NEAREST);
    glerr = fFunctions->fGetError();
    fFunctions->fTexParameteri(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_MIN_FILTER, GR_GL_NEAREST);
    glerr = fFunctions->fGetError();

    fFunctions->fFramebufferTexture2D(GR_GL_FRAMEBUFFER, GR_GL_COLOR_ATTACHMENT0, GR_GL_TEXTURE_2D, texture, 0);
    glerr = fFunctions->fGetError();
    GrGLenum DrawBuffers[1] = { GR_GL_COLOR_ATTACHMENT0 };
    fFunctions->fDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    glerr = fFunctions->fGetError();
    //auto buffer_status = fFunctions->fCheckFramebufferStatus(GR_GL_FRAMEBUFFER);
    glerr = fFunctions->fGetError();
  }

  void clear() {
    if (texture) {
      fFunctions->fDeleteTextures(1, &texture);
      texture = 0;
    }
    if (buffer) {
      fFunctions->fDeleteFramebuffers(1, &buffer);
      buffer = 0;
    }
  }

  void take(FrameBuffer&& other) {
    std::swap(buffer, other.buffer);
    std::swap(texture, other.texture);
  }


  ~FrameBuffer() {
    clear();
  }
};

class ExecutionReactor : public grpc::ClientBidiReactor<task_controller_grpc::TaskResult, task_controller_grpc::TaskConfig> {
public:
  std::mutex mutex;
  std::condition_variable condition;
  task_controller_grpc::TaskConfig received_task_config;
  std::optional<task_controller_grpc::TaskConfig> available_task_config;
  bool sending = false;
  task_controller_grpc::TaskResult sending_task_result;
  grpc::ClientContext context;
  bool done = false;
  ~ExecutionReactor() override {
    //grpc::ClientBidiReactor<task_controller_grpc::TaskResult, task_controller_grpc::TaskConfig>::StartWritesDone();
    context.TryCancel();
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&] { return done; });
  }
  void start() {
    grpc::ClientBidiReactor<task_controller_grpc::TaskResult, task_controller_grpc::TaskConfig>::StartRead(&received_task_config);
    grpc::ClientBidiReactor<task_controller_grpc::TaskResult, task_controller_grpc::TaskConfig>::StartCall();
  }
  void OnReadDone(bool ok) override {
    if(!ok) {
      return;
    }
    std::lock_guard<std::mutex> lock(mutex);
    available_task_config = received_task_config;
    grpc::ClientBidiReactor<task_controller_grpc::TaskResult, task_controller_grpc::TaskConfig>::StartRead(&received_task_config);
  }
  void OnWriteDone(bool ok) override {
    std::cout << "wrote " << ok << std::endl;
    if(!ok) {
      return;
    }
    {
      std::lock_guard<std::mutex> lock(mutex);
      sending = false;
    }
    condition.notify_all();
  }
  void OnDone(const grpc::Status&) override {
    std::cout << "OnDone" << std::endl;
    {
      std::lock_guard<std::mutex> lock(mutex);
      done = true;
    }
    condition.notify_all();
  }
  std::optional<task_controller_grpc::TaskConfig> receive_task_config() {
    std::lock_guard<std::mutex> lock(mutex);
    std::optional<task_controller_grpc::TaskConfig> result;
    std::swap(result, available_task_config);
    return result;
  }
  void send_task_result(const task_controller_grpc::TaskResult& result) {
    std::cout << "1" << std::endl;
    std::unique_lock<std::mutex> lock(mutex);
    std::cout << "2" << std::endl;
    condition.wait(lock, [&] { return !sending; });
    std::cout << "3" << std::endl;
    sending_task_result = result;
    grpc::ClientBidiReactor<task_controller_grpc::TaskResult, task_controller_grpc::TaskConfig>::StartWrite(&sending_task_result);
    sending = true;
  }
};

#include <lua/typeface.hpp>
#include <lua/fontmanager.hpp>
#include <lua/fontstyle.hpp>
#include <lua/font.hpp>
#include <lua/color.hpp>
#include <lua/paint.hpp>
#include <lua/canvas.hpp>
#include <lua/rect.hpp>
#include <state.hpp>
#include <state_manager.hpp>

static WriteReactor<thalamus_grpc::Text>* lualog_reactor;

static int lualog(lua_State* L) {
  auto text = luaL_checkstring(L, -1);
  thalamus_grpc::Text message;
  message.set_time(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
  message.set_text(text);
  lualog_reactor->write(std::move(message));
  return 0;
}

static double get_double(const thalamus::ObservableCollection::Value& value) {
  if (std::holds_alternative<double>(value)) {
    return std::get<double>(value);
  }
  return double(std::get<long long>(value));
}

static void gl_example(int width, int height, task_controller_grpc::TaskController::Stub& stub, thalamus_grpc::Thalamus::Stub& thalamus_stub) {
  auto L = luaL_newstate();
  proj_assert(L, "Failed to allocate Lua State");
  luaL_openlibs(L);

  luaL_requiref(L, "typeface", luaopen_typeface, false);
  luaL_requiref(L, "canvas", luaopen_canvas, false);

  lua_getglobal(L, "package");
  lua_getfield(L, -1, "path");
  lua_pushstring(L, ";lua/?.lua");
  lua_concat(L, 2);
  lua_setfield(L, -2, "path");

  lua_getfield(L, -1, "preload");
  lua_pushcfunction(L, luaopen_rect);
  lua_setfield(L, -2, "rect");
  lua_pushcfunction(L, luaopen_color);
  lua_setfield(L, -2, "color");
  lua_pushcfunction(L, luaopen_paint);
  lua_setfield(L, -2, "paint");
  lua_pushcfunction(L, luaopen_fontmanager);
  lua_setfield(L, -2, "fontmanager");
  lua_pushcfunction(L, luaopen_fontstyle);
  lua_setfield(L, -2, "fontstyle");
  lua_pushcfunction(L, luaopen_font);
  lua_setfield(L, -2, "font");

  lua_pushcfunction(L, lualog);
  lua_setglobal(L, "thalamus_log");

  auto err = luaL_dofile(L, "lua/main.lua");
  if(err) {
    auto err_str = luaL_tolstring(L, -1, nullptr);
    std::cerr << err_str << std::endl;
    std::abort();
  }

  boost::asio::io_context io_context;
  boost::asio::io_context logic_io_context;
  //boost::optional<boost::asio::io_service::work> m_active = boost::asio::io_service::work(io_context);

  Context context;

  auto& io = ImGui::GetIO();

  Window main_window{mainwindow, maincontext};
  //Window& operator_window = main_window;
  thalamus_grpc::Empty empty;

  ExecutionReactor reactor;
  stub.async()->execution(&reactor.context, &reactor);
  reactor.start();

  WriteReactor<thalamus_grpc::Text> log_reactor;
  lualog_reactor = &log_reactor;
  thalamus_stub.async()->log(&log_reactor.context, &empty, &log_reactor);
  log_reactor.start();

  std::mutex mutex;
  std::unique_ptr<Task> task = nullptr;

  int windowx = 0;
  int windowy = 0;
  ReadReactor<thalamus_grpc::AnalogResponse> touch_reactor(logic_io_context, [&](const thalamus_grpc::AnalogResponse& response) {
    double x, y;
    int count = 0;
    auto& data = response.data();
    for(auto i = 0;i < response.spans_size();++i) {
      auto& span = response.spans(i);
      if(span.name() == "X" && span.begin() < span.end()) {
        x = data[span.end() - 1];
        ++count;
      } else if(span.name() == "Y" && span.begin() < span.end()) {
        y = data[span.end() - 1];
        ++count;
      }
    }
    if(count == 2) {
      //std::cout << x - windowx << " " << y - windowy << std::endl;
      std::lock_guard<std::mutex> lock(mutex);
      if(!task) {
        return;
      }
      task->touch(x - windowx, y - windowy);
    }
  });
  thalamus_grpc::AnalogRequest touch_request;
  touch_request.mutable_node()->set_type("TOUCH_SCREEN");
  thalamus_stub.async()->analog(&touch_reactor.context, &touch_request, &touch_reactor);
  touch_reactor.start();

  std::vector<double> last_scales(8);
  std::vector<double> scales(8);
  ReadReactor<thalamus_grpc::AnalogResponse> gaze_reactor(logic_io_context, [&](const thalamus_grpc::AnalogResponse& response) {
    double x, y;
    int count = 0;
    auto& data = response.data();
    for(auto i = 0;i < response.spans_size();++i) {
      auto& span = response.spans(i);
      if(span.name() == "X" && span.begin() < span.end()) {
        x = data[span.end() - 1];
        ++count;
      } else if(span.name() == "Y" && span.begin() < span.end()) {
        y = data[span.end() - 1];
        ++count;
      }
    }
    if(count == 2) {
      std::lock_guard<std::mutex> lock(mutex);
      if(x >= 0) {
        if(y >= 0) {
          x *= scales[0];
          y *= -scales[1];
        } else {
          x *= scales[2];
          y *= -scales[3];
        }
      } else {
        if(y >= 0) {
          x *= scales[4];
          y *= -scales[5];
        } else {
          x *= scales[6];
          y *= -scales[7];
        }
      }
      if(!task) {
        return;
      }
      task->gaze(x, y);
    }
  });
  thalamus_grpc::AnalogRequest gaze_request;
  gaze_request.mutable_node()->set_type("OCULOMATIC");
  thalamus_stub.async()->analog(&gaze_reactor.context, &gaze_request, &gaze_reactor);
  gaze_reactor.start();

  auto state = std::make_shared<thalamus::ObservableDict>();
  thalamus::ObservableDictPtr eye_scaling;
  thalamus::ObservableDictPtr i;
  thalamus::ObservableDictPtr ii;
  thalamus::ObservableDictPtr iii;
  thalamus::ObservableDictPtr iv;
  thalamus::ObservableCollection::RecursiveObserver observer = [&] (thalamus::ObservableCollection* source, thalamus::ObservableCollection::Action,
                       const thalamus::ObservableCollection::Key& key,
                       const thalamus::ObservableCollection::Value& value) {
    if(source == state.get()) {
      auto key_str = std::get<std::string>(key);
      if(key_str == "eye_scaling") {
        eye_scaling = std::get<thalamus::ObservableDictPtr>(value);
        eye_scaling->recap(std::bind(observer, eye_scaling.get(), _1, _2, _3));
      }
    } else if (source == eye_scaling.get()) {
      auto key_str = std::get<std::string>(key);
      if(key_str == "I") {
        i = std::get<thalamus::ObservableDictPtr>(value);
        i->recap(std::bind(observer, i.get(), _1, _2, _3));
      } else if (key_str == "II") {
        ii = std::get<thalamus::ObservableDictPtr>(value);
        ii->recap(std::bind(observer, ii.get(), _1, _2, _3));
      } else if (key_str == "III") {
        iii = std::get<thalamus::ObservableDictPtr>(value);
        iii->recap(std::bind(observer, iii.get(), _1, _2, _3));
      } else if (key_str == "IV") {
        iv = std::get<thalamus::ObservableDictPtr>(value);
        iv->recap(std::bind(observer, iv.get(), _1, _2, _3));
      }
    } else if (source == i.get()) {
      auto key_str = std::get<std::string>(key);
      auto value_double = get_double(value);
      std::lock_guard<std::mutex> lock(mutex);
      if(key_str == "x") {
        scales[0] = value_double;
      } else if (key_str == "y") {
        scales[1] = value_double;
      }
    } else if (source == ii.get()) {
      auto key_str = std::get<std::string>(key);
      auto value_double = get_double(value);
      std::lock_guard<std::mutex> lock(mutex);
      if(key_str == "x") {
        scales[2] = value_double;
      } else if (key_str == "y") {
        scales[3] = value_double;
      }
    } else if (source == iii.get()) {
      auto key_str = std::get<std::string>(key);
      auto value_double = get_double(value);
      std::lock_guard<std::mutex> lock(mutex);
      if(key_str == "x") {
        scales[4] = value_double;
      } else if (key_str == "y") {
        scales[5] = value_double;
      }
    } else if (source == iv.get()) {
      auto key_str = std::get<std::string>(key);
      auto value_double = get_double(value);
      std::lock_guard<std::mutex> lock(mutex);
      if(key_str == "x") {
        scales[6] = value_double;
      } else if (key_str == "y") {
        scales[7] = value_double;
      }
    }
  };
  state->recursive_changed.connect(observer);

  thalamus::StateManager state_manager(&thalamus_stub, state, logic_io_context);

  Window operator_window{operatorwindow, operatorcontext};
  std::string a;
  main_window.rebuild(width, height);
  operator_window.rebuild(width, height);

  bool success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);
  proj_assert(success, "SDL_GL_MakeCurrent failed");
  FrameBuffer frameBuffer(&operator_window.gl_face->fFunctions, 512, 512);
  operator_window.rebuild(width, height, frameBuffer.buffer);
  frameBuffer.fFunctions = &operator_window.gl_face->fFunctions;

  auto draw_ui = [&] {
    auto scale = std::min(256.0/main_window.width, 256.0 / main_window.height);
    operator_window.gl_face->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, 0);
    operator_window.gl_face->fFunctions.fViewport(0, 0, int(io.DisplaySize.x), int(io.DisplaySize.y));
    //auto glerr = operator_window.gl_face->fFunctions.fGetError();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGui::Begin("Operator View", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);                          // Create a window called "Hello, world!" and append into it.
    ImGui::Image(frameBuffer.texture, ImVec2(float(main_window.width* scale), float(main_window.height * scale)), ImVec2(0, 1), ImVec2(1, 0));
    if (ImGui::BeginTable("split", 4))
    {
      ImGui::TableNextColumn(); ImGui::Text("I");
      ImGui::TableNextColumn();
      ImGui::TableNextColumn(); ImGui::Text("II");
      ImGui::TableNextColumn();
      ImGui::TableNextColumn(); ImGui::PushItemWidth(-1); ImGui::InputDouble("##Ix", &scales[0], 1);
      ImGui::TableNextColumn(); ImGui::PushItemWidth(-1); ImGui::InputDouble("##Iy", &scales[1], 1);
      ImGui::TableNextColumn(); ImGui::PushItemWidth(-1); ImGui::InputDouble("##IIx", &scales[2], 1);
      ImGui::TableNextColumn(); ImGui::PushItemWidth(-1); ImGui::InputDouble("##IIy", &scales[3], 1);
      ImGui::TableNextColumn(); ImGui::Text("III");
      ImGui::TableNextColumn();
      ImGui::TableNextColumn(); ImGui::Text("IV");
      ImGui::TableNextColumn();
      ImGui::TableNextColumn(); ImGui::InputDouble("##IIIx", &scales[4], 1);
      ImGui::TableNextColumn(); ImGui::InputDouble("##IIIy", &scales[5], 1);
      ImGui::TableNextColumn(); ImGui::InputDouble("##IVx", &scales[6], 1);
      ImGui::TableNextColumn(); ImGui::InputDouble("##IVy", &scales[7], 1);
      ImGui::EndTable();
    }
    //ImGui::InputText("Ix2", &a);
    //ImGui::InputDouble("Iy2", &scales[1], 1);
    //ImGui::Button("Iy2a");
    //ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  };

  auto task_frequency = 120;
  auto task_interval = 1000ms/task_frequency;

	std::cout << 1 << std::endl;
	SDL_Event event;
	first_start = std::chrono::steady_clock::now();

  auto stop = [&] {
    io_context.stop();
    logic_io_context.stop();
  };

  boost::asio::steady_timer window_timer(io_context);
  std::function<void(const boost::system::error_code&)> update_window = [&] (const boost::system::error_code& ec) {
    if(ec) {
      std::cerr << ec.message() << std::endl;
      return;
    }

    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      switch (event.type) {
      case SDL_EVENT_WINDOW_MOVED:
        {
          windowx = event.window.data1;
          windowy = event.window.data2;
        }
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        if (event.window.windowID == SDL_GetWindowID(mainwindow)) {
          main_window.rebuild(event.window.data1, event.window.data2);
          success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);
          proj_assert(success, "SDL_GL_MakeCurrent failed");
          std::cout << "Remake buffer" << std::endl;
          frameBuffer = FrameBuffer(&operator_window.gl_face->fFunctions, event.window.data1, event.window.data2);
          operator_window.rebuild(event.window.data1, event.window.data2, frameBuffer.buffer);
          frameBuffer.fFunctions = &operator_window.gl_face->fFunctions;
        }
        //else if (event.window.windowID == SDL_GetWindowID(operatorwindow)) {
        //  operator_window.rebuild(event.window.data1, event.window.data2, FramebufferName);
        //}
        break;
      case SDL_EVENT_MOUSE_MOTION:
        if(event.motion.windowID == SDL_GetWindowID(mainwindow)) {
          std::lock_guard<std::mutex> lock(mutex);
          if(task) {
            if(event.motion.state & SDL_BUTTON_LMASK) {
              task->touch(int(event.motion.x), int(event.motion.y));
            } 
            if (event.motion.state & SDL_BUTTON_RMASK) {
              task->gaze(int(event.motion.x), int(event.motion.y));
            }
          }
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if(event.button.windowID == SDL_GetWindowID(mainwindow)) {
          std::lock_guard<std::mutex> lock(mutex);
          if(task) {
            if(event.button.button == SDL_BUTTON_LEFT) {
              task->touch(int(event.button.x), int(event.button.y));
            } 
            if (event.button.button == SDL_BUTTON_RIGHT) {
              task->gaze(int(event.button.x), int(event.button.y));
            }
          }
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        if(event.button.windowID == SDL_GetWindowID(mainwindow)) {
          std::lock_guard<std::mutex> lock(mutex);
          if(task && event.button.button == SDL_BUTTON_LEFT) {
            task->touch(-1, -1);
          } 
        }
        break;
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      case SDL_EVENT_QUIT:
      	std::cout << "quit" << std::endl;
        stop();
      	break;
      case SDL_EVENT_KEY_UP:
        switch(event.key.key) {
        case SDLK_F:
          if(event.key.mod & SDL_KMOD_LCTRL) {
            auto flags = SDL_GetWindowFlags(mainwindow);
            auto new_flag = flags & SDL_WINDOW_FULLSCREEN ? 0 : SDL_WINDOW_FULLSCREEN;
            SDL_SetWindowFullscreen(mainwindow, new_flag);
          }
          break;
        case SDLK_ESCAPE:
          SDL_SetWindowFullscreen(mainwindow, 0);
          break;
        }
        break;
      }
    }

    {
      std::lock_guard<std::mutex> lock(mutex);
      success = SDL_GL_MakeCurrent(mainwindow, maincontext);
      proj_assert(success, "SDL_GL_MakeCurrent failed");
      main_window.gl_face->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, 0);
      //auto glerr = main_window.gl_face->fFunctions.fGetError();
      main_window.gl_face->fFunctions.fViewport(0, 0, main_window.width, main_window.height);
      main_window.gpuCanvas->resetMatrix();
      main_window.gpuCanvas->clear(SK_ColorBLACK);
      if(task) {
        task->draw(main_window.gpuCanvas, View::SUBJECT);
      }
      main_window.context->flush();

      success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);
      operator_window.gl_face->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, frameBuffer.buffer);
      //glerr = operator_window.gl_face->fFunctions.fGetError();
      operator_window.gl_face->fFunctions.fViewport(0, 0, operator_window.width, operator_window.height);
      operator_window.fb_gpuCanvas->resetMatrix();
      operator_window.fb_gpuCanvas->clear(SK_ColorBLACK);
      if(task) {
        task->draw(operator_window.fb_gpuCanvas, View::OPERATOR);
      }
      operator_window.context->flush();

      last_scales = scales;
      draw_ui();
      if (i) {
        if (abs(last_scales[0] - scales[0]) > 1e-4) {
          (*i)["x"].assign(scales[0]);
        }
        if (abs(last_scales[1] - scales[1]) > 1e-4) {
          (*i)["y"].assign(scales[1]);
        }
      }
      if (ii) {
        if (abs(last_scales[2] - scales[2]) > 1e-4) {
          (*ii)["x"].assign(scales[2]);
        }
        if (abs(last_scales[3] - scales[3]) > 1e-4) {
          (*ii)["y"].assign(scales[3]);
        }
      }
      if (iii) {
        if (abs(last_scales[4] - scales[4]) > 1e-4) {
          (*iii)["x"].assign(scales[4]);
        }
        if (abs(last_scales[5] - scales[5]) > 1e-4) {
          (*iii)["y"].assign(scales[5]);
        }
      }
      if (iv) {
        if (abs(last_scales[6] - scales[6]) > 1e-4) {
          (*iv)["x"].assign(scales[6]);
        }
        if (abs(last_scales[7] - scales[7]) > 1e-4) {
          (*iv)["y"].assign(scales[7]);
        }
      }
    }
    success = SDL_GL_MakeCurrent(mainwindow, maincontext);
    SDL_GL_SwapWindow(mainwindow);
    success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);
    SDL_GL_SwapWindow(operatorwindow);

    window_timer.expires_after(4ms);
    window_timer.async_wait(update_window);
  };
  window_timer.expires_after(4ms);
  window_timer.async_wait(update_window);

  boost::asio::steady_timer logic_timer(logic_io_context);
  std::function<void(const boost::system::error_code&)> update_logic = [&] (const boost::system::error_code& ec) {
    if(ec) {
      std::cerr << ec.message() << std::endl;
      return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    if(task) {
      task->update();
      auto status = task->status();
      if(status.has_value()) {
        std::cout << "Respond" << std::endl;
        reactor.send_task_result(*status);
        task.reset();
      }
    } else {
      auto task_config = reactor.receive_task_config();
      if(task_config.has_value()) {
        std::cout << task_config->body() << std::endl;
        std::stringstream stream(task_config->body());
        stream >> context.config;
        std::cout << context.config;
        task.reset(make_task(context, L));
      }
    }

    logic_timer.expires_after(4ms);
    logic_timer.async_wait(update_logic);
  };
  logic_timer.expires_after(4ms);
  logic_timer.async_wait(update_logic);

  SDL_GetWindowPosition(mainwindow, &windowx, &windowy);

  std::thread update_thread([&] {
    logic_io_context.run();
  });
  io_context.run();

  update_thread.join();
  
  lua_close(L);
  success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);
  frameBuffer.clear();
  operator_window.clean();
  success = SDL_GL_MakeCurrent(mainwindow, maincontext);
  main_window.clean();
}

int main(int, char*[]) {
  std::cout << 100 << std::endl;
  //SDL_GLContext maincontext; /* Our opengl context handle */
  std::cout << 100 << std::endl;

  if (!SDL_Init(SDL_INIT_VIDEO)) /* Initialize SDL's Video subsystem */ {
	  std::cout << "Unable to initialize SDL" << std::endl;
	  std::terminate();
  }
  std::cout << 101 << std::endl;

  /* Request opengl 3.2 context.
   * SDL doesn't have the ability to choose which profile at this time of writing,
   * but it should default to the core profile */
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  /* Turn on double buffering with a 24bit Z buffer.
   * You may need to change this to 16 or 32 for your system */
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  std::cout << 102 << std::endl;

  /* Create our window centered at 512x512 resolution */
  mainwindow = SDL_CreateWindow("SUBJECT",
  	512, 512, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  std::cout << 103 << std::endl;
  if (!mainwindow) /* Die if creation failed */ {
	  std::cout << "Unable to create window" << std::endl;
	  std::terminate();
  }
  std::cout << 104 << std::endl;

  operatorwindow = SDL_CreateWindow("OPERATOR",
    384, 384, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  std::cout << 103 << std::endl;
  if (!operatorwindow) /* Die if creation failed */ {
    std::cout << "Unable to create window" << std::endl;
    std::terminate();
  }
  std::cout << 104 << std::endl;

  /* Create our opengl context and attach it to our window */
  maincontext = SDL_GL_CreateContext(mainwindow);
  std::cout << 105 << std::endl;
  bool success = SDL_GL_MakeCurrent(mainwindow, maincontext);
  if (!success) /* Die if creation failed */ {
    auto error = SDL_GetError();
    std::cout << "SDL_GL_MakeCurrent failed: " << error << std::endl;
    std::terminate();
  }
  //operatorwindow = mainwindow;
  //operatorcontext = maincontext;

  /* Create our opengl context and attach it to our window */
  operatorcontext = SDL_GL_CreateContext(operatorwindow);
  std::cout << 105 << std::endl;
  success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);
  if (!success) /* Die if creation failed */ {
    auto error = SDL_GetError();
    std::cout << "SDL_GL_MakeCurrent failed: " << error << std::endl;
    std::terminate();
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForOpenGL(operatorwindow, operatorcontext);
  success = ImGui_ImplOpenGL3_Init();
  if (!success) /* Die if creation failed */ {
    std::cout << "ImGui_ImplOpenGL3_Init failed" << std::endl;
    std::terminate();
  }
 
  {
    auto credentials = grpc::InsecureChannelCredentials();
    auto thalamus_channel = grpc::CreateChannel("localhost:50050", credentials);
    auto task_controller_channel = grpc::CreateChannel("localhost:50051", credentials);
    auto thalamus_stub = thalamus_grpc::Thalamus::NewStub(thalamus_channel);
    auto task_controller_stub = task_controller_grpc::TaskController::NewStub(task_controller_channel);

    gl_example(512, 512, *task_controller_stub, *thalamus_stub);
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DestroyContext(maincontext);
  SDL_DestroyWindow(mainwindow);
  SDL_GL_DestroyContext(operatorcontext);
  SDL_DestroyWindow(operatorwindow);
  SDL_Quit();
  std::cout << "Quit" << std::endl;
  return 0;
}

