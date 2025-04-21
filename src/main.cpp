#include <iostream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>

#include <chrono>

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
#include "include/encode/SkPngEncoder.h"

#include <grpcpp/grpcpp.h>
#include <thalamus.pb.h>
#include <thalamus.grpc.pb.h>
#include <task_controller.pb.h>
#include <task_controller.grpc.pb.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"

//#ifdef __clang__
//#pragma clang diagnostic pop
//#endif

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace std::chrono_literals;

std::chrono::steady_clock::time_point first_start;

void draw(SkCanvas* canvas) {
	const SkScalar scale = 256.0f;
	const SkScalar R = 0.45f * scale;
	const SkScalar TAU = 6.2831853f;
	SkPath path;
	path.moveTo(R, 0.0f);
	for (int i = 1; i < 7; ++i) {
    	SkScalar theta = 3 * i * TAU / 7;
    	path.lineTo(R * cos(theta), R * sin(theta));
	}
	path.close();
	SkPaint p;
	p.setAntiAlias(true);
	canvas->clear(SK_ColorWHITE);
	//canvas->translate(0.5f * scale, 0.5f * scale);
	canvas->rotate(90e-3 * std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - first_start).count());
	canvas->drawPath(path, p);
}

void raster(int width, int height,
        	void (*draw)(SkCanvas*),
        	const char* path) {
	sk_sp<SkSurface> rasterSurface(SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height)));
	SkCanvas* rasterCanvas = rasterSurface->getCanvas();
	draw(rasterCanvas);
	sk_sp<SkImage> img(rasterSurface->makeImageSnapshot());
	if (!img) { return; }
	sk_sp<SkData> png = SkPngEncoder::Encode(nullptr, img.get(), {});
	if (!png) { return; }
	SkFILEWStream out(path);
	(void)out.write(png->data(), png->size());
}

SDL_Window *mainwindow; /* Our window handle */
SDL_Window * operatorwindow; /* Our window handle */
SDL_GLContext maincontext; /* Our opengl context handle */
SDL_GLContext operatorcontext; /* Our opengl context handle */

struct Window {
  SDL_Window* sdl_window;
  SDL_GLContext sdl_context;
  sk_sp<const GrGLInterface> gl_face = nullptr;
  sk_sp<GrDirectContext> context = nullptr;
  sk_sp<SkSurface> surface = nullptr;
  SkSurfaceProps props;
  SkCanvas* gpuCanvas = nullptr;
  GrGLint buffer;
  int width, height;
  sk_sp<SkSurface> fb_surface = nullptr;
  SkCanvas* fb_gpuCanvas = nullptr;
  SkSurfaceProps fb_props;

  void rebuild(int width, int height, int buffer = -1) {
    this->width = width;
    this->height = height;
    bool success = SDL_GL_MakeCurrent(sdl_window, sdl_context);
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

    if (buffer >= 0) {
      GrGLFramebufferInfo info;
      info.fFBOID = (GrGLuint)buffer;
      info.fFormat = GR_GL_RGB8;

      GrBackendRenderTarget target = GrBackendRenderTargets::MakeGL(256, 256, 0, 8, info);

      fb_surface = SkSurfaces::WrapBackendRenderTarget(context.get(), target,
        kBottomLeft_GrSurfaceOrigin,
        kRGB_888x_SkColorType, nullptr, &fb_props);

      fb_gpuCanvas = fb_surface->getCanvas();
    }
   }
};

void gl_example(int width, int height, void (*draw)(SkCanvas*), const char* path) {
	std::cout << 1 << std::endl;
	std::cout << 2 << std::endl;
	std::cout << 3 << std::endl;

  auto& io = ImGui::GetIO();

  Window main_window{mainwindow, maincontext};
  Window& operator_window = main_window;
  //Window operator_window(operatorwindow, operatorcontext);
  std::vector<double> scales(8);
  std::string a;
  main_window.rebuild(width, height);
  //operator_window.rebuild(width / 2, height / 2);

  bool success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);
  GrGLuint FramebufferName = 0;
  auto glerr = operator_window.gl_face->fFunctions.fGetError();
  operator_window.gl_face->fFunctions.fGenFramebuffers(1, &FramebufferName);
  glerr = operator_window.gl_face->fFunctions.fGetError();
  operator_window.gl_face->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, FramebufferName);
  glerr = operator_window.gl_face->fFunctions.fGetError();
  // The texture we're going to render to
  GrGLuint renderedTexture;
  operator_window.gl_face->fFunctions.fGenTextures(1, &renderedTexture);
  glerr = operator_window.gl_face->fFunctions.fGetError();

  // "Bind" the newly created texture : all future texture functions will modify this texture
  operator_window.gl_face->fFunctions.fBindTexture(GR_GL_TEXTURE_2D, renderedTexture);
  glerr = operator_window.gl_face->fFunctions.fGetError();

  // Give an empty image to OpenGL ( the last "0" )
  operator_window.gl_face->fFunctions.fTexImage2D(GR_GL_TEXTURE_2D, 0, GR_GL_RGB, 256, 256, 0, GR_GL_RGB, GR_GL_UNSIGNED_BYTE, 0);
  glerr = operator_window.gl_face->fFunctions.fGetError();

  // Poor filtering. Needed !
  operator_window.gl_face->fFunctions.fTexParameteri(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_MAG_FILTER, GR_GL_NEAREST);
  glerr = operator_window.gl_face->fFunctions.fGetError();
  operator_window.gl_face->fFunctions.fTexParameteri(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_MIN_FILTER, GR_GL_NEAREST);
  glerr = operator_window.gl_face->fFunctions.fGetError();

  operator_window.gl_face->fFunctions.fFramebufferTexture2D(GR_GL_FRAMEBUFFER, GR_GL_COLOR_ATTACHMENT0, GR_GL_TEXTURE_2D, renderedTexture, 0);
  glerr = operator_window.gl_face->fFunctions.fGetError();
  GrGLenum DrawBuffers[1] = { GR_GL_COLOR_ATTACHMENT0 };
  operator_window.gl_face->fFunctions.fDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
  glerr = operator_window.gl_face->fFunctions.fGetError();
  auto buffer_status = operator_window.gl_face->fFunctions.fCheckFramebufferStatus(GR_GL_FRAMEBUFFER);
  glerr = operator_window.gl_face->fFunctions.fGetError();

  main_window.rebuild(width, height, FramebufferName);

  auto draw_ui = [&] {
    bool success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);

    operator_window.gl_face->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, FramebufferName);
    auto glerr = operator_window.gl_face->fFunctions.fGetError();
    auto scale = std::min(256.0 / main_window.width, 256.0 / main_window.height);
    operator_window.gl_face->fFunctions.fViewport(0, 0, operator_window.width*scale, operator_window.height * scale);
    glerr = operator_window.gl_face->fFunctions.fGetError();

    operator_window.fb_gpuCanvas->resetMatrix();
    operator_window.fb_gpuCanvas->scale(scale, scale);
    draw(operator_window.fb_gpuCanvas);
    operator_window.context->flush();
    operator_window.gl_face->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, 0);

    operator_window.gl_face->fFunctions.fViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glerr = operator_window.gl_face->fFunctions.fGetError();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Operator View");                          // Create a window called "Hello, world!" and append into it.
    ImGui::Image(renderedTexture, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
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

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
      SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(operatorwindow);
  };

	std::cout << 1 << std::endl;
	SDL_Event event;
	bool running = true;
	//gl_face->fFunctions.fViewport(0, 0, 512, 512);
	first_start = std::chrono::steady_clock::now();
	while (running) {
  	auto start = std::chrono::steady_clock::now();
  	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
  	while (running && elapsed < 16ms) {
    	if (SDL_WaitEventTimeout(&event, 16 - elapsed.count())) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        //draw_ui();

      	switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
          if (event.window.windowID == SDL_GetWindowID(mainwindow)) {
            main_window.rebuild(event.window.data1, event.window.data2, FramebufferName);
          }
          else if (event.window.windowID == SDL_GetWindowID(operatorwindow)) {
            operator_window.rebuild(event.window.data1, event.window.data2, FramebufferName);
          }
          break;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      	case SDL_EVENT_QUIT:
        	std::cout << "quit" << std::endl;
        	running = false;
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
    	elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    	//std::cout << "DRAW " << << std::endl;
  	}
  	//std::cout << "DRAW" << std::endl;

    bool success = SDL_GL_MakeCurrent(mainwindow, maincontext);
    main_window.gl_face->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, 0);
    auto glerr = main_window.gl_face->fFunctions.fGetError();
    main_window.gl_face->fFunctions.fViewport(0, 0, main_window.width, main_window.height);

    main_window.gpuCanvas->resetMatrix();
    draw(main_window.gpuCanvas);

    //SDL_GL_SwapWindow(mainwindow);

    draw_ui();
    //success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);
    //operator_window.gl_face->fFunctions.fViewport(0, 0, operator_window.width, operator_window.height);
    //
    //operator_window.gpuCanvas->resetMatrix();
    //auto scale = std::min(double(operator_window.width) / main_window.width, double(operator_window.height) / main_window.height);
    //operator_window.gpuCanvas->scale(scale, scale);
    //draw(operator_window.gpuCanvas);
    //operator_window.context->flush();
    //
    //SDL_GL_SwapWindow(operatorwindow);
	}
	//sk_sp<SkImage> img(gpuSurface->makeImageSnapshot());
	//if (!img) { return; }
	//// Must pass non-null context so the pixels can be read back and encoded.
	//sk_sp<SkData> png = SkPngEncoder::Encode(context.get(), img.get(), {});
	//if (!png) { return; }
	//SkFILEWStream out(path);
	//(void)out.write(png->data(), png->size());
}

int main(int argc, char* argv[]) {
  std::cout << 100 << std::endl;
  //SDL_GLContext maincontext; /* Our opengl context handle */
  std::cout << 100 << std::endl;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) /* Initialize SDL's Video subsystem */ {
	std::cout << "Unable to initialize SDL" << std::endl;
	std::terminate();
	return 1;
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
	  return 1;
  }
  std::cout << 104 << std::endl;

  //operatorwindow = SDL_CreateWindow("OPERATOR",
  //  256, 256, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  //std::cout << 103 << std::endl;
  //if (!operatorwindow) /* Die if creation failed */ {
  //  std::cout << "Unable to create window" << std::endl;
  //  std::terminate();
  //  return 1;
  //}
  //std::cout << 104 << std::endl;

  /* Create our opengl context and attach it to our window */
  maincontext = SDL_GL_CreateContext(mainwindow);
  std::cout << 105 << std::endl;
  bool success = SDL_GL_MakeCurrent(mainwindow, maincontext);
  if (!success) /* Die if creation failed */ {
    auto error = SDL_GetError();
    std::cout << "SDL_GL_MakeCurrent failed: " << error << std::endl;
    std::terminate();
    return 1;
  }
  operatorwindow = mainwindow;
  operatorcontext = maincontext;

  /* Create our opengl context and attach it to our window */
  //operatorcontext = SDL_GL_CreateContext(operatorwindow);
  //std::cout << 105 << std::endl;
  //success = SDL_GL_MakeCurrent(operatorwindow, operatorcontext);
  //if (!success) /* Die if creation failed */ {
  //  auto error = SDL_GetError();
  //  std::cout << "SDL_GL_MakeCurrent failed: " << error << std::endl;
  //  std::terminate();
  //  return 1;
  //}

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForOpenGL(operatorwindow, operatorcontext);
  success = ImGui_ImplOpenGL3_Init();
  if (!success) /* Die if creation failed */ {
    std::cout << "ImGui_ImplOpenGL3_Init failed" << std::endl;
    std::terminate();
    return 1;
  }
 
  {
    auto credentials = grpc::InsecureChannelCredentials();
    auto thalamus_channel = grpc::CreateChannel("localhost:50050", credentials);
    auto task_controller_channel = grpc::CreateChannel("localhost:50051", credentials);
    auto thalamus_stub = thalamus_grpc::Thalamus::NewStub(thalamus_channel);
    auto task_controller_stub = task_controller_grpc::TaskController::NewStub(task_controller_channel);

    gl_example(512, 512, draw, "skia.png");
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DestroyContext(maincontext);
  SDL_DestroyWindow(mainwindow);
  //SDL_GL_DestroyContext(operatorcontext);
  //SDL_DestroyWindow(operatorwindow);
  SDL_Quit();
  std::cout << "Quit" << std::endl;
  return 0;
}

