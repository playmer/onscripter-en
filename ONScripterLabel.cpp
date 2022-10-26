/* -*- C++ -*-
 *
 *  ONScripterLabel.cpp - Execution block parser of ONScripter-EN
 *
 *  Copyright (c) 2001-2011 Ogapee. All rights reserved.
 *  (original ONScripter, of which this is a fork).
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  Copyright (c) 2007-2011 "Uncle" Mion Sonozaki
 *
 *  UncleMion@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>
 *  or write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Modified by Haeleth, autumn 2006, to remove unnecessary diagnostics and support OS X/Linux packaging better.

// Modified by Mion, March 2008, to update from
// Ogapee's 20080121 release source code.

// Modified by Mion, April 2009, to update from
// Ogapee's 20090331 release source code.

// Modified by Mion, November 2009, to update from
// Ogapee's 20091115 release source code.

#include <algorithm>

#include "ONScripter.h"
#include "graphics_cpu.h"
#include "graphics_resize.h"
#include <cstdio>

#ifdef MACOSX
#include "cocoa_alertbox.h"
#include "cocoa_directories.h"

#ifdef USE_PPC_GFX
#include <sys/types.h>
#include <sys/sysctl.h>
#endif // USE_PPC_GFX

#endif // MACOSX


#ifdef WIN32
#include <windows.h>
#include "SDL_syswm.h"
#include "winres.h"
typedef HRESULT (WINAPI *GETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPTSTR);
#endif
#ifdef LINUX
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#endif
#if !defined(WIN32) && !defined(MACOSX)
#include "resources.h"
#endif

extern void initSJIS2UTF16();
extern "C" void waveCallback( int channel );

#define DEFAULT_AUDIOBUF 2048

#define FONT_FILE "default.ttf"
#define REGISTRY_FILE "registry.txt"
#define DLL_FILE "dll.txt"
//haeleth change to use English-language font name
#define DEFAULT_ENV_FONT "MS Gothic"

#define SAVEFILE_VERSION_MAJOR 2
#define SAVEFILE_VERSION_MINOR 6


#undef min

void ONScripter::WarpMouse(int x, int y)
{
  printf("x: %d, y: %d\n", x, y);
  int windowResolutionX, windowResolutionY;
  SDL_GetRendererOutputSize(mRenderer, &windowResolutionX, &windowResolutionY);

  float scaleHeight = windowResolutionY / (float)screen_surface->h;
  float scaleWidth  = windowResolutionX / (float)screen_surface->w;
  float scale = std::min(scaleHeight, scaleWidth);
  
  SDL_Rect dstRect = {};
  dstRect.w = scale * screen_surface->w;
  dstRect.h = scale * screen_surface->h;
  dstRect.x = (windowResolutionX - dstRect.w)/2;
  dstRect.y = (windowResolutionY - dstRect.h)/2;

  x = (x * scale) + dstRect.x;
  y = (y * scale) + dstRect.y;

  SDL_WarpMouseInWindow(mWindow, x, y);
}

bool ONScripter::ToggleFullscreen(SDL_Window* Window) {
    bool IsFullscreen = SDL_GetWindowFlags(Window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
    return 0 == SDL_SetWindowFullscreen(Window, IsFullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

SDL_Surface* ONScripter::SetVideoMode(int width, int height, int bpp, bool fullscreen)
{
  SDL_SetWindowSize(mWindow, width, height);
  SDL_SetWindowFullscreen(mWindow, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

  mGLRenderer.Resize(width, height);

  return screen_surface;
}

void ONScripter::UpdateScreen(SDL_Rect dst_rect)
{
    // Original
  SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
  SDL_RenderClear(mRenderer);

  bool doIt = false;
  if (doIt)
    SDL_SaveBMP(accumulation_surface, "Test.bmp");

  auto texture = SDL_CreateTextureFromSurface(mRenderer, accumulation_surface);
  
  Uint32 format;
  int imageResolutionX, imageResolutionY, access;
  SDL_QueryTexture(texture, &format, &access, &imageResolutionX, &imageResolutionY);

  int windowResolutionX, windowResolutionY;
  SDL_GetRendererOutputSize(mRenderer, &windowResolutionX, &windowResolutionY);

  float scaleHeight = windowResolutionY / (float)imageResolutionY;
  float scaleWidth  = windowResolutionX / (float)imageResolutionX;
  float scale = std::min(scaleHeight, scaleWidth);
  
  SDL_Rect dstRect;
  dstRect.w = scale * imageResolutionX;
  dstRect.h = scale * imageResolutionY;
  dstRect.x = (windowResolutionX - dstRect.w)/2;
  dstRect.y = (windowResolutionY - dstRect.h)/2;

  SDL_RenderCopy(mRenderer, texture, NULL /*&dst_rect*/, &dstRect);
  SDL_RenderPresent(mRenderer);
  SDL_DestroyTexture(texture);

    // GL
  mGLRenderer.UpdateScreen();
}

typedef int (ONScripter::*FuncList)();
static struct FuncLUT{
    char command[30];
    FuncList method;
} func_lut[] = {
    {"yesnobox",   &ONScripter::yesnoboxCommand},
    {"wavestop",   &ONScripter::wavestopCommand},
    {"waveloop",   &ONScripter::waveCommand},
    {"wave",   &ONScripter::waveCommand},
    {"waittimer",   &ONScripter::waittimerCommand},
    {"wait",   &ONScripter::waitCommand},
    {"vsp2",   &ONScripter::vspCommand},
    {"vsp",   &ONScripter::vspCommand},
    {"voicevol",   &ONScripter::voicevolCommand},
    {"trap",   &ONScripter::trapCommand},
    {"transbtn",  &ONScripter::transbtnCommand},
    {"textspeeddefault",   &ONScripter::textspeeddefaultCommand},
    {"textspeed",   &ONScripter::textspeedCommand},
    {"textshow",   &ONScripter::textshowCommand},
    {"texton",   &ONScripter::textonCommand},
    {"textoff",   &ONScripter::textoffCommand},
    {"texthide",   &ONScripter::texthideCommand},
    {"textexbtn",   &ONScripter::textexbtnCommand},
    {"textclear",   &ONScripter::textclearCommand},
    {"textbtnwait",   &ONScripter::btnwaitCommand},
    {"textbtnstart",   &ONScripter::textbtnstartCommand},
    {"textbtnoff",   &ONScripter::textbtnoffCommand},
    {"texec",   &ONScripter::texecCommand},
    {"tateyoko",   &ONScripter::tateyokoCommand},
    {"tal", &ONScripter::talCommand},
    {"tablegoto",   &ONScripter::tablegotoCommand},
    {"systemcall",   &ONScripter::systemcallCommand},
    {"strsph",   &ONScripter::strspCommand},
    {"strsp",   &ONScripter::strspCommand},
    {"stop",   &ONScripter::stopCommand},
    {"sp_rgb_gradation",   &ONScripter::sp_rgb_gradationCommand},
    {"spstr",   &ONScripter::spstrCommand},
    {"spreload",   &ONScripter::spreloadCommand},
    {"splitstring",   &ONScripter::splitCommand},
    {"split",   &ONScripter::splitCommand},
    {"spclclk",   &ONScripter::spclclkCommand},
    {"spbtn",   &ONScripter::spbtnCommand},
    {"skipoff",   &ONScripter::skipoffCommand},
    {"shell",   &ONScripter::shellCommand},
    {"sevol",   &ONScripter::sevolCommand},
    {"setwindow3",   &ONScripter::setwindow3Command},
    {"setwindow2",   &ONScripter::setwindow2Command},
    {"setwindow",   &ONScripter::setwindowCommand},
    {"seteffectspeed",   &ONScripter::seteffectspeedCommand},
    {"setcursor",   &ONScripter::setcursorCommand},
    {"selnum",   &ONScripter::selectCommand},
    {"selgosub",   &ONScripter::selectCommand},
    {"selectbtnwait", &ONScripter::btnwaitCommand},
    {"select",   &ONScripter::selectCommand},
    {"savetime",   &ONScripter::savetimeCommand},
    {"savescreenshot2",   &ONScripter::savescreenshotCommand},
    {"savescreenshot",   &ONScripter::savescreenshotCommand},
    {"saveon",   &ONScripter::saveonCommand},
    {"saveoff",   &ONScripter::saveoffCommand},
    {"savegame2",   &ONScripter::savegameCommand},
    {"savegame",   &ONScripter::savegameCommand},
    {"savefileexist",   &ONScripter::savefileexistCommand},
    {"r_trap",   &ONScripter::trapCommand},
    {"rnd",   &ONScripter::rndCommand},
    {"rnd2",   &ONScripter::rndCommand},
    {"rmode",   &ONScripter::rmodeCommand},
    {"resettimer",   &ONScripter::resettimerCommand},
    {"resetmenu", &ONScripter::resetmenuCommand},
    {"reset",   &ONScripter::resetCommand},
    {"repaint",   &ONScripter::repaintCommand},
    {"quakey",   &ONScripter::quakeCommand},
    {"quakex",   &ONScripter::quakeCommand},
    {"quake",   &ONScripter::quakeCommand},
    {"puttext",   &ONScripter::puttextCommand},
    {"prnumclear",   &ONScripter::prnumclearCommand},
    {"prnum",   &ONScripter::prnumCommand},
    {"print",   &ONScripter::printCommand},
    {"language", &ONScripter::languageCommand},
    {"playstop",   &ONScripter::playstopCommand},
    {"playonce",   &ONScripter::playCommand},
    {"play",   &ONScripter::playCommand},
    {"okcancelbox",   &ONScripter::yesnoboxCommand},
    {"ofscpy", &ONScripter::ofscopyCommand},
    {"ofscopy", &ONScripter::ofscopyCommand},
    {"nega", &ONScripter::negaCommand},
    {"msp2", &ONScripter::mspCommand},
    {"msp", &ONScripter::mspCommand},
    {"mpegplay", &ONScripter::movieCommand},
    {"mp3vol", &ONScripter::mp3volCommand},
    {"mp3stop", &ONScripter::mp3stopCommand},
    {"mp3save", &ONScripter::mp3Command},
    {"mp3loop", &ONScripter::mp3Command},
    {"mp3fadeout", &ONScripter::mp3fadeoutCommand},
    {"mp3fadein", &ONScripter::mp3fadeinCommand},
    {"mp3", &ONScripter::mp3Command},
    {"movie", &ONScripter::movieCommand},
    {"movemousecursor", &ONScripter::movemousecursorCommand},
    {"mousemode", &ONScripter::mousemodeCommand},
    {"monocro", &ONScripter::monocroCommand},
    {"minimizewindow", &ONScripter::minimizewindowCommand},
    {"mesbox", &ONScripter::mesboxCommand},
    {"menu_window", &ONScripter::menu_windowCommand},
    {"menu_waveon", &ONScripter::menu_waveonCommand},
    {"menu_waveoff", &ONScripter::menu_waveoffCommand},
    {"menu_full", &ONScripter::menu_fullCommand},
    {"menu_click_page", &ONScripter::menu_click_pageCommand},
    {"menu_click_def", &ONScripter::menu_click_defCommand},
    {"menu_automode", &ONScripter::menu_automodeCommand},
    {"lsph2sub", &ONScripter::lsp2Command},
    {"lsph2add", &ONScripter::lsp2Command},
    {"lsph2", &ONScripter::lsp2Command},
    {"lsph", &ONScripter::lspCommand},
    {"lsp2sub", &ONScripter::lsp2Command},
    {"lsp2add", &ONScripter::lsp2Command},
    {"lsp2", &ONScripter::lsp2Command},
    {"lsp", &ONScripter::lspCommand},
    {"lr_trap",   &ONScripter::trapCommand},
    {"lrclick",   &ONScripter::clickCommand},
    {"loopbgmstop", &ONScripter::loopbgmstopCommand},
    {"loopbgm", &ONScripter::loopbgmCommand},
    {"lookbackflush", &ONScripter::lookbackflushCommand},
    {"lookbackbutton",      &ONScripter::lookbackbuttonCommand},
    {"logsp2", &ONScripter::logspCommand},
    {"logsp", &ONScripter::logspCommand},
    {"locate", &ONScripter::locateCommand},
    {"loadgame", &ONScripter::loadgameCommand},
    {"linkcolor", &ONScripter::linkcolorCommand},
    {"ld", &ONScripter::ldCommand},
    {"layermessage", &ONScripter::layermessageCommand},
    {"jumpf", &ONScripter::jumpfCommand},
    {"jumpb", &ONScripter::jumpbCommand},
    {"isfull", &ONScripter::isfullCommand},
    {"isskip", &ONScripter::isskipCommand},
    {"ispage", &ONScripter::ispageCommand},
    {"isdown", &ONScripter::isdownCommand},
    {"insertmenu", &ONScripter::insertmenuCommand},
    {"input", &ONScripter::inputCommand},
    {"indent", &ONScripter::indentCommand},
    {"humanorder", &ONScripter::humanorderCommand},
    {"getzxc", &ONScripter::getzxcCommand},
    {"getvoicevol", &ONScripter::getvoicevolCommand},
    {"getversion", &ONScripter::getversionCommand},
    {"gettimer", &ONScripter::gettimerCommand},
    {"gettextbtnstr", &ONScripter::gettextbtnstrCommand},
    {"gettext", &ONScripter::gettextCommand},
    {"gettaglog", &ONScripter::gettaglogCommand},
    {"gettag", &ONScripter::gettagCommand},
    {"gettab", &ONScripter::gettabCommand},
    {"getspsize", &ONScripter::getspsizeCommand},
    {"getspmode", &ONScripter::getspmodeCommand},
    {"getskipoff", &ONScripter::getskipoffCommand},
    {"getsevol", &ONScripter::getsevolCommand},
    {"getscreenshot", &ONScripter::getscreenshotCommand},
    {"getsavestr", &ONScripter::getsavestrCommand},
    {"getret", &ONScripter::getretCommand},
    {"getreg", &ONScripter::getregCommand},
    {"getpageup", &ONScripter::getpageupCommand},
    {"getpage", &ONScripter::getpageCommand},
    {"getnextline", &ONScripter::getcursorposCommand},
    {"getmp3vol", &ONScripter::getmp3volCommand},
    {"getmousepos", &ONScripter::getmouseposCommand},
    {"getmouseover", &ONScripter::getmouseoverCommand},
    {"getmclick", &ONScripter::getmclickCommand},
    {"getlogtext", &ONScripter::gettextCommand},
    {"getlog", &ONScripter::getlogCommand},
    {"getinsert", &ONScripter::getinsertCommand},
    {"getfunction", &ONScripter::getfunctionCommand},
    {"getenter", &ONScripter::getenterCommand},
    {"getcursorpos2", &ONScripter::getcursorposCommand},
    {"getcursorpos", &ONScripter::getcursorposCommand},
    {"getcursor", &ONScripter::getcursorCommand},
    {"getcselstr", &ONScripter::getcselstrCommand},
    {"getcselnum", &ONScripter::getcselnumCommand},
    {"getbtntimer", &ONScripter::gettimerCommand},
    {"getbgmvol", &ONScripter::getmp3volCommand},
    {"game", &ONScripter::gameCommand},
    {"flushout", &ONScripter::flushoutCommand},
    {"fileexist", &ONScripter::fileexistCommand},
    {"existspbtn", &ONScripter::spbtnCommand},
    {"exec_dll", &ONScripter::exec_dllCommand},
    {"exbtn_d", &ONScripter::exbtnCommand},
    {"exbtn", &ONScripter::exbtnCommand},
    {"erasetextwindow", &ONScripter::erasetextwindowCommand},
    {"erasetextbtn", &ONScripter::erasetextbtnCommand},
    {"effectskip", &ONScripter::effectskipCommand},
    {"end", &ONScripter::endCommand},
    {"dwavestop", &ONScripter::dwavestopCommand},
    {"dwaveplayloop", &ONScripter::dwaveCommand},
    {"dwaveplay", &ONScripter::dwaveCommand},
    {"dwaveloop", &ONScripter::dwaveCommand},
    {"dwaveload", &ONScripter::dwaveCommand},
    {"dwave", &ONScripter::dwaveCommand},
    {"drawtext", &ONScripter::drawtextCommand},
    {"drawsp3", &ONScripter::drawsp3Command},
    {"drawsp2", &ONScripter::drawsp2Command},
    {"drawsp", &ONScripter::drawspCommand},
    {"drawfill", &ONScripter::drawfillCommand},
    {"drawclear", &ONScripter::drawclearCommand},
    {"drawbg2", &ONScripter::drawbg2Command},
    {"drawbg", &ONScripter::drawbgCommand},
    {"draw", &ONScripter::drawCommand},
    {"deletescreenshot", &ONScripter::deletescreenshotCommand},
    {"delay", &ONScripter::delayCommand},
    {"definereset", &ONScripter::defineresetCommand},
    {"csp2", &ONScripter::cspCommand},
    {"csp", &ONScripter::cspCommand},
    {"cselgoto", &ONScripter::cselgotoCommand},
    {"cselbtn", &ONScripter::cselbtnCommand},
    {"csel", &ONScripter::selectCommand},
    {"click", &ONScripter::clickCommand},
    {"cl", &ONScripter::clCommand},
    {"chvol", &ONScripter::chvolCommand},
    {"checkpage", &ONScripter::checkpageCommand},
    {"checkkey", &ONScripter::checkkeyCommand},
    {"cellcheckspbtn", &ONScripter::spbtnCommand},
    {"cellcheckexbtn", &ONScripter::exbtnCommand},
    {"cell", &ONScripter::cellCommand},
    {"caption", &ONScripter::captionCommand},
    {"btnwait2", &ONScripter::btnwaitCommand},
    {"btnwait", &ONScripter::btnwaitCommand},
    {"btntime2", &ONScripter::btntimeCommand},
    {"btntime", &ONScripter::btntimeCommand},
    {"btndown",  &ONScripter::btndownCommand},
    {"btndef",  &ONScripter::btndefCommand},
    {"btnarea",  &ONScripter::btnareaCommand},
    {"btn",     &ONScripter::btnCommand},
    {"br",      &ONScripter::brCommand},
    {"blt",      &ONScripter::bltCommand},
    {"bgmvol", &ONScripter::mp3volCommand},
    {"bgmstop", &ONScripter::mp3stopCommand},
    {"bgmonce", &ONScripter::mp3Command},
    {"bgmfadeout", &ONScripter::mp3fadeoutCommand},
    {"bgmfadein", &ONScripter::mp3fadeinCommand},
    {"bgmdownmode", &ONScripter::bgmdownmodeCommand},
    {"bgm", &ONScripter::mp3Command},
    {"bgcpy",      &ONScripter::bgcopyCommand},
    {"bgcopy",      &ONScripter::bgcopyCommand},
    {"bg",      &ONScripter::bgCommand},
    {"barclear",      &ONScripter::barclearCommand},
    {"bar",      &ONScripter::barCommand},
    {"avi",      &ONScripter::aviCommand},
    {"automode_time",      &ONScripter::automode_timeCommand},
    {"autoclick",      &ONScripter::autoclickCommand},
    {"amsp2",      &ONScripter::amspCommand},
    {"amsp",      &ONScripter::amspCommand},
    {"allsp2resume",      &ONScripter::allsp2resumeCommand},
    {"allspresume",      &ONScripter::allspresumeCommand},
    {"allsp2hide",      &ONScripter::allsp2hideCommand},
    {"allsphide",      &ONScripter::allsphideCommand},
    {"abssetcursor", &ONScripter::setcursorCommand},
    {"", NULL}
};

static struct FuncHash{
    int start;
    int end;
    FuncHash()
    : start(-1), end(-2) {}
} func_hash['z'-'a'+1];

static void SDL_Quit_Wrapper()
{
    SDL_Quit();
}


static char const* Source(GLenum source)
{
    switch (source)
    {
    case GL_DEBUG_SOURCE_API: return "DEBUG_SOURCE_API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "DEBUG_SOURCE_WINDOW_SYSTEM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER: return "DEBUG_SOURCE_SHADER_COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY: return "DEBUG_SOURCE_THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION: return "DEBUG_SOURCE_APPLICATION";
    case GL_DEBUG_SOURCE_OTHER: return "DEBUG_SOURCE_OTHER";
    default: return "unknown";
    }
}

static char const* Severity(GLenum severity)
{
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH: return "DEBUG_SEVERITY_HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM: return "DEBUG_SEVERITY_MEDIUM";
    case GL_DEBUG_SEVERITY_LOW: return "DEBUG_SEVERITY_LOW";
    case GL_DEBUG_SEVERITY_NOTIFICATION: return "DEBUG_SEVERITY_NOTIFICATION";
    default: return "unknown";
    }
}


static char const* Type(GLenum type)
{
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR: return "DEBUG_TYPE_ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEBUG_TYPE_DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "DEBUG_TYPE_UNDEFINED_BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY: return "DEBUG_TYPE_PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE: return "DEBUG_TYPE_PERFORMANCE";
    case GL_DEBUG_TYPE_MARKER: return "DEBUG_TYPE_MARKER";
    case GL_DEBUG_TYPE_PUSH_GROUP: return "DEBUG_TYPE_PUSH_GROUP";
    case GL_DEBUG_TYPE_POP_GROUP: return "DEBUG_TYPE_POP_GROUP";
    case GL_DEBUG_TYPE_OTHER: return "DEBUG_TYPE_OTHER";
    default: return "unknown";
    }
}


static void APIENTRY messageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    if (GL_DEBUG_SEVERITY_NOTIFICATION == severity)
    {
        return;
    }

    printf("GL DEBUG CALLBACK:\n    Source = %s\n    type = %s\n    severity = %s\n    message = %s\n",
        Source(source),
        Type(type),
        Severity(severity),
        message);
}


void GLRenderer::RenderToTexture::Create(SDL_Surface* aSurface)
{
    CreateInternal(aSurface->w, aSurface->h, aSurface);
}


void GLRenderer::RenderToTexture::Create(int aWidth, int aHeight)
{

}

void GLRenderer::RenderToTexture::CreateInternal(int aWidth, int aHeight, SDL_Surface* aSurface)
{
    //////////////// Framebuffer
    glGenFramebuffers(1, &mFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);

    //////////////// Color
    glGenTextures(1, &mColor);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, mColor);

    // Give an empty image to OpenGL ( the last "0" )
    if (nullptr == aSurface)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, aWidth, aHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }
    else
    {
        SDL_Surface* newSurface = SDL_CreateRGBSurface(0, aWidth, aHeight, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
        SDL_BlitSurface(aSurface, 0, newSurface, 0); // Blit onto a purely RGB Surface
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, aWidth, aHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, newSurface->pixels);
    }

    // Poor filtering. Needed !
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    //////////////// Depth
    glGenRenderbuffers(1, &mDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, mDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, aWidth, aHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepth);


    //////////////// Framebuffer Setup
    // Set "renderedTexture" as our colour attachement #0
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mColor, 0);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
}

void GLRenderer::Initialize(int aWidth, int aHeight)
{
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.3 + GLSL 130
    const char* glsl_version = "#version 330";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    mSecondWindow = SDL_CreateWindow("GL Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, aWidth, aHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);
    if (mSecondWindow == nullptr)
    {
        puts(SDL_GetError());
    }
    mContext = SDL_GL_CreateContext(mSecondWindow);
    SDL_GL_MakeCurrent(mSecondWindow, mContext);

    glbinding::initialize([](const char* name) 
        { 
            auto fn = reinterpret_cast<glbinding::ProcAddress>(SDL_GL_GetProcAddress(name));;
            return fn;
        }, true);

    glEnable(GL_DEBUG_OUTPUT);

    // FIXME: This doesn't work on Apple when I tested it, need to look into this more on 
    // other platforms, and maybe only enable it in dev builds.
#if defined(_WIN32)
    glDebugMessageCallback(messageCallback, nullptr);
#endif


    accumulation_surface.Create(aWidth, aHeight);
    backup_surface.Create(aWidth, aHeight);
    screen_surface.Create(aWidth, aHeight);
    effect_dst_surface.Create(aWidth, aHeight);
    effect_src_surface.Create(aWidth, aHeight);
    effect_tmp_surface.Create(aWidth, aHeight);
    screenshot_surface.Create(aWidth, aHeight);
    image_surface.Create(aWidth, aHeight);
}

void GLRenderer::Resize(int aWidth, int aHeight)
{
    SDL_SetWindowSize(mSecondWindow, aWidth, aHeight);

    DestroyTextures();
    accumulation_surface.Create(aWidth, aHeight);
    backup_surface.Create(aWidth, aHeight);
    screen_surface.Create(aWidth, aHeight);
    effect_dst_surface.Create(aWidth, aHeight);
    effect_src_surface.Create(aWidth, aHeight);
    effect_tmp_surface.Create(aWidth, aHeight);
    screenshot_surface.Create(aWidth, aHeight);
    image_surface.Create(aWidth, aHeight);
}

void GLRenderer::DestroyTextures()
{
    accumulation_surface.Destroy();
    backup_surface.Destroy();
    screen_surface.Destroy();
    effect_dst_surface.Destroy();
    effect_src_surface.Destroy();
    effect_tmp_surface.Destroy();
    screenshot_surface.Destroy();
    image_surface.Destroy();
}

void GLRenderer::RenderToTexture::Destroy()
{
    glDeleteTextures(1, &mDepth);
    glDeleteBuffers(1, &mDepth);
    glDeleteTextures(1, &mColor);
    glDeleteBuffers(1, &mColor);
    glDeleteFramebuffers(1, &mFramebuffer);
}

void GLRenderer::UpdateScreen()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(mSecondWindow);
}


void ONScripter::InitializeWindowAndRenderer()
{
    mGLRenderer.Initialize(800, 600);
    SDL_CreateWindowAndRenderer(800, 600, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI, &mWindow, &mRenderer);
}


void ONScripter::SetWindowIcon(SDL_Surface* icon)
{
    SDL_SetWindowIcon(mWindow, icon);
    SDL_SetWindowIcon(mGLRenderer.mSecondWindow, icon);
}

void ONScripter::initSDL()
{
    /* ---------------------------------------- */
    /* Initialize SDL */

    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0 ){
        errorAndExit("Couldn't initialize SDL", SDL_GetError(), "Init Error", true);
        return; //dummy
    }
    atexit(SDL_Quit_Wrapper); // work-around for OS/2

    
    for(int i = 0; i < SDL_NumJoysticks(); i++ ) 
    {
      if (SDL_IsGameController(i))
      { 
        // We don't care which controller, we're taking input from all of them.
        SDL_GameControllerOpen(i);
      }
    }

    //SDL_AudioInit("directsound");

#ifdef ONSCRIPTER_CDAUDIO
    if( cdaudio_flag && SDL_InitSubSystem( SDL_INIT_CDROM ) < 0 ){
        errorAndExit("Couldn't initialize CD-ROM", SDL_GetError(), "Init Error", true);
        return; //dummy
    }
#endif

#if 0
    if(SDL_InitSubSystem( SDL_INIT_JOYSTICK ) == 0 && SDL_JoystickOpen(0) != NULL)
        printf( "Initialize JOYSTICK\n");
#endif
    
    InitializeWindowAndRenderer();

    /* ---------------------------------------- */
    /* Initialize SDL */
    if ( TTF_Init() < 0 ){
        errorAndExit("can't initialize SDL TTF", NULL, "Init Error", true);
        return; //dummy
    }

//insani added app icon
    SDL_Surface* icon = LoadSurfaceFromFile("icon.png");

    //use icon.png preferably, but try embedded resources if not found
    //(cmd-line option --use-app-icons to prefer app resources over icon.png)
    //(Mac apps can set use-app-icons in a engine.config file within the
    //bundle, to have it always use the bundle icns)
#ifndef MACOSX
    if (!icon || use_app_icons) {
#ifdef WIN32
      // FIXME:
        //use the (first) Windows icon resource
        //HICON wicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ONSCRICON));
        //if (wicon) {
        //    SDL_SysWMinfo info;
        //    SDL_VERSION(&info.version);
        //    SDL_GetWMInfo(&info);
        //    SendMessage(info.window, WM_SETICON, ICON_BIG, (LPARAM)wicon);
        //}
#else
        //backport from ponscripter
        //const InternalResource* internal_icon = getResource("icon.png");
        //if (internal_icon) {
        //    if (icon) SDL_FreeSurface(icon);
        //    SDL_RWops* rwicon = SDL_RWFromConstMem(internal_icon->buffer,
        //                                           internal_icon->size);
        //    icon = IMG_Load_RW(rwicon, 0);
        //    use_app_icons = false;
        //}
#endif //WIN32
    }
#endif //!MACOSX
    // If an icon was found (and desired), use it.
    if (icon && !use_app_icons) {
#if defined(MACOSX) || defined(WIN32)
#if defined(MACOSX)
        //resize the (usually 32x32) icon to 128x128
        SDL_Surface *tmp2 = SDL_CreateRGBSurface(SDL_SWSURFACE, 128, 128,
                                                 32, 0x00ff0000, 0x0000ff00,
                                                 0x000000ff, 0xff000000);
#elif defined(WIN32)
        //resize the icon to 32x32
        SDL_Surface *tmp2 = SDL_CreateRGBSurface(SDL_SWSURFACE, 32, 32,
                                                 32, 0x00ff0000, 0x0000ff00,
                                                 0x000000ff, 0xff000000);
#endif //MACOSX, WIN32
        SDL_Surface *tmp = SDL_ConvertSurface( icon, tmp2->format, SDL_SWSURFACE );
        if ((tmp->w == tmp2->w) && (tmp->h == tmp2->h)) {
            //already the right size, just use converted surface as-is
            SDL_FreeSurface(tmp2);
            tmp2 = icon;
            icon = tmp;
            SDL_FreeSurface(tmp2);
        } else {
            //resize converted surface
            ons_gfx::resizeSurface(tmp, tmp2);
            SDL_FreeSurface(tmp);
            tmp = icon;
            icon = tmp2;
            SDL_FreeSurface(tmp);
        }
#endif //MACOSX || WIN32
        SetWindowIcon(icon);
    }
    if (icon)
        SDL_FreeSurface(icon);

#ifdef BPP16
    screen_bpp = 16;
#else
    screen_bpp = 32;
#endif

#if defined(PDA) && defined(PDA_WIDTH)
    screen_ratio1 *= PDA_WIDTH;
    screen_ratio2 *= 320;
    screen_width   = screen_width  * PDA_WIDTH / 320;
    screen_height  = screen_height * PDA_WIDTH / 320;
#elif defined(PDA) && defined(PDA_AUTOSIZE)
    SDL_Rect **modes;
    modes = SDL_ListModes(NULL, 0);
    if (modes == (SDL_Rect **)0){
        errorAndExit("No Video mode available.", NULL, "Init Error", true);
        return; //dummy
    }
    else if (modes == (SDL_Rect **)-1){
        // no restriction
    }
 	else{
        int width;
        if (modes[0]->w * 3 > modes[0]->h * 4)
            width = (modes[0]->h / 3) * 4;
        else
            width = (modes[0]->w / 4) * 4;
        screen_ratio1 *= width;
        screen_ratio2 *= 320;
        screen_width   = screen_width  * width / 320;
        screen_height  = screen_height * width / 320;
    }
#endif

#ifdef RCA_SCALE
    scr_stretch_x = 1.0;
    scr_stretch_y = 1.0;
#endif
    if (scaled_flag) {
        SDL_DisplayMode DM;
        SDL_GetDesktopDisplayMode(0, &DM);
        int native_width = DM.w;
        int native_height = DM.h;
        
        // Resize up to fill screen
#ifndef RCA_SCALE
        float scr_stretch_x, scr_stretch_y;
#endif
        scr_stretch_x = (float)native_width / (float)screen_width;
        scr_stretch_y = (float)native_height / (float)screen_height;
#ifdef RCA_SCALE
        if (widescreen_flag) {
            if (scr_stretch_x > scr_stretch_y) {
                screen_ratio1 = native_height;
                screen_ratio2 = script_height;
                scr_stretch_x /= scr_stretch_y;
                scr_stretch_y = 1.0;
            } else { 
                screen_ratio1 = native_width;
                screen_ratio2 = script_width;
                scr_stretch_y /= scr_stretch_x;
                scr_stretch_x = 1.0;
            }
            screen_width  = StretchPosX(script_width);
            screen_height = StretchPosY(script_height);
        } else
#endif
        {
            // Constrain aspect to same as game
            if (scr_stretch_x > scr_stretch_y) {
                screen_ratio1 = native_height;
                screen_ratio2 = script_height;
            } else { 
                screen_ratio1 = native_width;
                screen_ratio2 = script_width;
            }
            scr_stretch_x = scr_stretch_y = 1.0;
            screen_width  = ExpandPos(script_width);
            screen_height = ExpandPos(script_height);
        }
    }
#ifdef RCA_SCALE
    else if (widescreen_flag) {
        const SDL_VideoInfo* info = SDL_GetVideoInfo();
        int native_width = info->current_w;
        int native_height = info->current_h;
        
        // Resize to screen aspect ratio
        const float screen_asp = (float)screen_width / (float)screen_height;
        const float native_asp = (float)native_width / (float)native_height;
        const float aspquot = native_asp / screen_asp;
        if (aspquot >1.01) {
            // Widescreen; make gamearea wider
            scr_stretch_x = (float)screen_height * native_asp / (float)screen_width;
            screen_width = screen_height * native_asp;
        } else if (aspquot < 0.99) {
            scr_stretch_y = (float)screen_width / native_asp / (float)screen_height;
            screen_height = screen_width / native_asp;
        }
    }
#endif
    screen_surface = AnimationInfo::allocSurface( screen_width, screen_height );
    screen_surface = SetVideoMode( screen_width, screen_height, screen_bpp, fullscreen_mode );

    /* ---------------------------------------- */
    /* Check if VGA screen is available. */
#if defined(PDA) && (PDA_WIDTH==640)
    if ( screen_surface == NULL ){
        screen_ratio1 /= 2;
        screen_width  /= 2;
        screen_height /= 2;
        screen_surface = SetVideoMode( screen_width, screen_height, screen_bpp, fullscreen_mode );
    }
#endif

    if ( screen_surface == NULL ) {
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN,
                 "Couldn't set %dx%dx%d video mode",
                 screen_width, screen_height, screen_bpp);
        errorAndExit(script_h.errbuf, SDL_GetError(), "Init Error", true);
        return; //dummy
    }
    //printf("Display: %d x %d (%d bpp)\n", screen_width, screen_height, screen_bpp);
    dirty_rect.setDimension(screen_width, screen_height);

    initSJIS2UTF16();

    setStr(&wm_title_string, gDEFAULT_WM_TITLE.c_str());
    setStr(&wm_icon_string, DEFAULT_WM_ICON);
    SDL_SetWindowTitle(mWindow, wm_title_string);
    //FIXME: SDL_SetWindowIcon(mWindow, wm_icon_string);

#ifdef WIN32
    //check the audio driver setting
    const char* audiodriver = SDL_GetAudioDriver(0);
    //fprintf(stderr,"audio driver: %s\n", audiodriver);
    audiobuffer_size = 8192; //minimum buffer size for waveout
    if ((audiobuffer_size < 8192) &&
        (strcmp(audiodriver, "waveout") == 0)){
        audiobuffer_size = 8192; //minimum buffer size for waveout
    }
#endif
    openAudio();
}

void ONScripter::openAudio(/*int freq, Uint16 format, int channels*/)
{
    //if ( Mix_OpenAudio( freq, format, channels, audiobuffer_size ) < 0 ){
    //    errorAndCont("Couldn't open audio device!", SDL_GetError(), "Init Error", true);
    //    audio_open_flag = false;
    //}
    //else{
    //    int freq;
    //    Uint16 format;
    //    int channels;
    //
    //    Mix_QuerySpec( &freq, &format, &channels);
    //    //printf("Audio: %d Hz %d bit %s\n", freq,
    //    //       (format&0xFF),
    //    //       (channels > 1) ? "stereo" : "mono");
    //    audio_format.format = format;
    //    audio_format.freq = freq;
    //    audio_format.channels = channels;
    //
        audio_open_flag = true;
    //
    //    Mix_AllocateChannels( ONS_MIX_CHANNELS+ONS_MIX_EXTRA_CHANNELS );
    //    Mix_ChannelFinished( waveCallback );
    //}
}

int ONScripter::ExpandPos(int val) {
    return float(val * screen_ratio1) / screen_ratio2 + 0.5;
}

int ONScripter::ContractPos(int val) {
    return float(val * screen_ratio2) / screen_ratio1 + 0.5;
}
#ifdef RCA_SCALE
int ONScripter::StretchPosX(int val) {
    return float(val * screen_ratio1) * scr_stretch_x / screen_ratio2 + 0.5;
}
int ONScripter::StretchPosY(int val) {
    return float(val * screen_ratio1) * scr_stretch_y / screen_ratio2 + 0.5;
}
#endif

ONScripter::ONScripter()
//Using an initialization list to make sure pointers start out NULL
  : default_font(NULL)
  , registry_file(NULL)
  , dll_file(NULL)
  , getret_str(NULL)
  , key_exe_file(NULL)
  , trap_dest(NULL)
  , wm_title_string(NULL)
  , wm_icon_string(NULL)
  , accumulation_surface(NULL)
  , backup_surface(NULL)
  , effect_dst_surface(NULL)
  , effect_src_surface(NULL)
  , effect_tmp_surface(NULL)
  , screenshot_surface(NULL)
  , image_surface(NULL)
  , tmp_image_buf(NULL)
  , current_button_link(NULL)
  , shelter_button_link(NULL)
  , sprite_info(NULL)
  , sprite2_info(NULL)
  , font_file(NULL)
  , root_glyph_cache(NULL)
  , string_buffer_breaks(NULL)
  , string_buffer_margins(NULL)
  , sin_table(NULL)
  , cos_table(NULL)
  , whirl_table(NULL)
  , breakup_cells(NULL)
  , breakup_cellforms(NULL)
  , breakup_mask(NULL)
  , shelter_select_link(NULL)
  , default_cdrom_drive(NULL)
  , wave_file_name(NULL)
  , seqmusic_file_name(NULL)
  #ifdef ONSCRIPTER_CDAUDIO
  , cdrom_info(NULL)
  #endif
  , music_file_name(NULL)
  , music_buffer(NULL)
  , mp3_sample(NULL)
  //, music_info(NULL)
  , music_cmd(NULL)
  , seqmusic_cmd(NULL)
  , async_movie(NULL)
  , movie_buffer(NULL)
  , async_movie_surface(NULL)
  , surround_rects(NULL)
  , text_font(NULL)
  , cached_page(NULL)
  , system_menu_title(NULL)
{
    //first initialize *everything* (static) to base values
#if TARGET_OS_WIN32
    mSoundBackEnd = SoLoud::Soloud::BACKENDS::WASAPI;
#elif TARGET_OS_MAC
    mSoundBackEnd = SoLoud::Soloud::BACKENDS::COREAUDIO;
#else
    mSoundBackEnd = SoLoud::Soloud::BACKENDS::MINIAUDIO;
#endif

    resetFlags();
    resetFlagsSub();

    //init envdata variables
    fullscreen_mode = false;
    volume_on_flag = true;
    text_speed_no = 1;
    cdaudio_on_flag = false;
    automode_time = 1000;

    //init onscripter-specific variables
    skip_past_newline = false;
    cdaudio_flag = false;
    preferred_automode_time_set = false;
    preferred_automode_time = automode_time;
    enable_wheeldown_advance_flag = false;
    disable_rescale_flag = false;
    edit_flag = false;
#ifdef RCA_SCALE
    widescreen_flag = false;
#endif
    scaled_flag = false;
    nomovieupscale_flag = false;
    window_mode = false;
    use_app_icons = false;
    cdrom_drive_number = 0;
    audiobuffer_size = DEFAULT_AUDIOBUF;
    match_bgm_audio_flag = false;
#ifdef WIN32
    current_user_appdata = false;
#endif

    //init various internal variables
    audio_open_flag = false;
    getret_int = 0;
    variable_edit_index = variable_edit_num = variable_edit_sign = 0;
    tmp_image_buf_length = mean_size_of_loaded_images = 0;
    num_loaded_images = 1; //avoid possible div by zero
    effect_counter = effect_timer_resolution = 0;
    effect_start_time = effect_start_time_old = 0;
    effect_duration = 1; //avoid possible div by zero
    effect_tmp = 0;
    skip_effect = in_effect_blank = false;
    effectspeed = EFFECTSPEED_NORMAL;
    shortcut_mouse_line = -1;
    skip_mode = SKIP_NONE;
    music_buffer_length = 0;
    mp3fade_start = 0;
    wm_edit_string[0] = '\0';
#ifdef PNG_AUTODETECT_NSCRIPTER_MASKS
    png_mask_type = PNG_MASK_AUTODETECT;
#elif defined PNG_FORCE_NSCRIPTER_MASKS
    png_mask_type = PNG_MASK_USE_NSCRIPTER;
#else
    png_mask_type = PNG_MASK_USE_ALPHA;
#endif
    
    //init arrays
    int i=0;
    for (i=0 ; i<MAX_PARAM_NUM ; i++) bar_info[i] = prnum_info[i] = NULL;
    last_textpos_xy[0] = last_textpos_xy[1] = 0;
    loop_bgm_name[0] = loop_bgm_name[1] = NULL;
    for ( i=0 ; i<ONS_MIX_CHANNELS ; i++ ) {
        channelvolumes[i] = DEFAULT_VOLUME;
        channel_preloaded[i] = false;
    }
    //for (i=0 ; i<ONS_MIX_CHANNELS+ONS_MIX_EXTRA_CHANNELS ; i++)
    //    wave_sample[i] = NULL;

    fileversion = SAVEFILE_VERSION_MAJOR*100 + SAVEFILE_VERSION_MINOR;

    internal_timer = SDL_GetTicks();

    //setting this to let script_h call error message popup routines
    script_h.setOns(this);
    
#if defined (USE_X86_GFX) && !defined(MACOSX)
    // determine what functions the cpu supports (Mion)
    {
        using namespace ons_gfx;
        unsigned int func, eax, ebx, ecx, edx;
        func = CPUF_NONE;
        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) != 0) {
            printf("System info: Intel CPU, with functions: ");
            if (edx & bit_MMX) {
                func |= CPUF_X86_MMX;
                printf("MMX ");
            }
            if (edx & bit_SSE) {
                func |= CPUF_X86_SSE;
                printf("SSE ");
            }
            if (edx & bit_SSE2) {
                func |= CPUF_X86_SSE2;
                printf("SSE2 ");
            }
            printf("\n");
        }
        setCpufuncs(func);
    }
#elif defined (USE_X86_GFX) && defined(MACOSX)
    // x86 CPU on Mac OS X all support SSE2
    ons_gfx::setCpufuncs(ons_gfx::CPUF_X86_SSE2);
    printf("System info: Intel CPU with SSE2 functionality\n");
#elif defined(USE_PPC_GFX) && defined(MACOSX)
    // Determine if this PPC CPU supports AltiVec (Roto)
    {
        using namespace ons_gfx;
        unsigned int func = CPUF_NONE;
        int altivec_present = 0;
    
        size_t length = sizeof(altivec_present);
        int error = sysctlbyname("hw.optional.altivec", &altivec_present, &length, NULL, 0);
        if(error) {
            setCpufuncs(CPUF_NONE);
            return;
        }
        if(altivec_present) {
            func |= CPUF_PPC_ALTIVEC;
            printf("System info: PowerPC CPU, supports altivec\n");
        } else {
            printf("System info: PowerPC CPU, DOES NOT support altivec\n");
        }
        setCpufuncs(func);
    }
#else
    disableCpuGfx();
#endif

    //since we've made it this far, let's init some dynamic variables
    setStr( &registry_file, REGISTRY_FILE );
    setStr( &dll_file, DLL_FILE );
    readColor( &linkcolor[0], "#FFFF22" ); // yellow - link color
    readColor( &linkcolor[1], "#88FF88" ); // cyan - mouseover link color
    sprite_info  = new AnimationInfo[MAX_SPRITE_NUM];
    sprite2_info = new AnimationInfo[MAX_SPRITE2_NUM];

    for (i=0 ; i<MAX_SPRITE2_NUM ; i++)
        sprite2_info[i].affine_flag = true;
    for (i=0 ; i<NUM_GLYPH_CACHE ; i++){
        if (i != NUM_GLYPH_CACHE-1) glyph_cache[i].next = &glyph_cache[i+1];
    }
    glyph_cache[NUM_GLYPH_CACHE-1].next = NULL;
    root_glyph_cache = &glyph_cache[0];

    // External Players
    music_cmd = getenv("PLAYER_CMD");
    seqmusic_cmd  = getenv("MUSIC_CMD");
}

ONScripter::~ONScripter()
{
    reset();

    delete[] sprite_info;
    delete[] sprite2_info;

    if (default_font) delete[] default_font;
    if (font_file) delete[] font_file;
}

void ONScripter::enableCDAudio(){
    cdaudio_flag = true;
}

void ONScripter::setCDNumber(int cdrom_drive_number)
{
    this->cdrom_drive_number = cdrom_drive_number;
}

void ONScripter::setAudiodriver(const char *driver)
{
    char buf[128];
    if (driver && driver[0] != '\0')
        snprintf(buf, 128, "SDL_AUDIODRIVER=%s", driver);
    else
        strncpy(buf, "SDL_AUDIODRIVER=", 128);
    //SDL_putenv(buf);
}

void ONScripter::setAudioBufferSize(int kbyte_size)
{
    if ( (kbyte_size == 1) || (kbyte_size == 2) || (kbyte_size == 4) ||
         (kbyte_size == 8) || (kbyte_size == 16) ) {
        //only allow powers of 2 as buffer sizes
        audiobuffer_size = kbyte_size * 1024;
        fprintf(stderr, "Using audiobuffer of %d bytes\n", audiobuffer_size);
    } else {
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN, "Invalid audiobuffer size %dk"
                 " - using prior size of %d bytes", kbyte_size, audiobuffer_size);
        errorAndCont(script_h.errbuf, NULL, "Config Issue", true);
    }
}

void ONScripter::setMatchBgmAudio(bool flag)
{
    match_bgm_audio_flag = flag;
}

void ONScripter::setFontFile(const char *filename)
{
    setStr(&default_font, filename);
}

void ONScripter::setRegistryFile(const char *filename)
{
    setStr(&registry_file, filename);
}

void ONScripter::setDLLFile(const char *filename)
{
    setStr(&dll_file, filename);
}

void ONScripter::setFileVersion(const char *ver)
{
    int verno = atoi(ver);
    if ((verno >= 199) && (verno <= fileversion))
        fileversion = verno;
}

void ONScripter::setFullscreenMode()
{
    fullscreen_mode = true;
}

void ONScripter::setWindowMode()
{
    window_mode = true;
}

#ifndef NO_LAYER_EFFECTS
void ONScripter::setNoLayers()
{
    use_layers = false;
}
#endif

#ifdef WIN32
void ONScripter::setUserAppData()
{
    current_user_appdata = true;
}
#endif

void ONScripter::setUseAppIcons()
{
    use_app_icons = true;
}

void ONScripter::setIgnoreTextgosubNewline()
{
    script_h.ignore_textgosub_newline = true;
}

void ONScripter::setSkipPastNewline()
{
    skip_past_newline = true;
}

void ONScripter::setPreferredWidth(const char *widthstr)
{
    int width = atoi(widthstr);
    //minimum preferred window width of 160 (gets ridiculous if smaller)
    if (width > 160)
        preferred_width = width;
    else if (width > 0)
        preferred_width = 160;
}

void ONScripter::enableButtonShortCut()
{
    force_button_shortcut_flag = true;
}

void ONScripter::setPreferredAutomodeTime(const char *timestr)
{
    long time = atoi(timestr);
    printf("setting preferred automode time to %ld\n", time);
    preferred_automode_time_set = true;
    automode_time = preferred_automode_time = time;
}

void ONScripter::enableWheelDownAdvance()
{
    enable_wheeldown_advance_flag = true;
}

void ONScripter::disableCpuGfx()
{
    using namespace ons_gfx;
    setCpufuncs(CPUF_NONE);
}

void ONScripter::disableRescale()
{
    disable_rescale_flag = true;
}

void ONScripter::enableEdit()
{
    edit_flag = true;
}

void ONScripter::setKeyEXE(const char *filename)
{
    setStr(&key_exe_file, filename);
}

#ifdef RCA_SCALE
void ONScripter::setWidescreen()
{
    widescreen_flag = true;
}
#endif

void ONScripter::setNoScaled()
{
    scaled_flag = false;
}
void ONScripter::setScaled()
{
    scaled_flag = true;
}

void ONScripter::setNoMovieUpscale()
{
    nomovieupscale_flag = true;
}

void ONScripter::setGameIdentifier(const char *gameid)
{
    setStr(&cmdline_game_id, gameid);
}

int ONScripter::init()
{
    if (archive_path.get_num_paths() == 0) {
    
        //default archive_path is current directory ".", followed by parent ".."
        DirPaths default_path = DirPaths(".");
        default_path.add("..");
#ifdef MACOSX
        // On Mac OS X, store archives etc in the application bundle by default,
        // but also check the application root directory and current directory.
        if (isBundled()) {
            archive_path.add(bundleResPath());

            // Now add the application path.
            char *path = bundleAppPath();
            if (path) {
                archive_path.add(path);
                // add the next directory up as a fallback.
                char tmp[strlen(path) + 4];
                sprintf(tmp, "%s%c%s", path, DELIMITER, "..");
                archive_path.add(tmp);
            } else {
                //if we couldn't find the application path, we still need
                //something - use current dir and parent (default)
                archive_path.add(default_path);
            }
        }
        else {
            // Not in a bundle: just use current dir and parent as normal.
            archive_path.add(default_path);
        }
#else
        // On Linux, the path is unpredictable and should be set by
        // using "-r PATH" or "--root PATH" in a launcher script.
        // On other platforms it's the same place as the executable.
        archive_path.add(default_path);
        //printf("init:archive_paths: \"%s\"\n", archive_path->get_all_paths());
#endif
    }
    
    if (key_exe_file){
        createKeyTable( key_exe_file );
        script_h.setKeyTable( key_table );
    }
    
    if ( open() ) return -1;

    if ( script_h.save_path == NULL ){
        char* gameid = script_h.game_identifier;
        char gamename[20];
        if (!gameid) {
            gameid=(char*)&gamename;
            snprintf(gameid, 20, "ONScripter-%x", script_h.game_hash);
        }
#ifdef WIN32
        // On Windows, store in [Profiles]/All Users/Application Data.
        // Permit saves to be per-user rather than shared if
        // option --current-user-appdata is specified
        HMODULE shdll = LoadLibrary("shfolder");
        if (shdll) {
            GETFOLDERPATH gfp = GETFOLDERPATH(GetProcAddress(shdll, "SHGetFolderPathA"));
            if (gfp) {
                char hpath[MAX_PATH];
#define CSIDL_COMMON_APPDATA 0x0023 // for [Profiles]/All Users/Application Data
#define CSIDL_APPDATA 0x001A // for [Profiles]/[User]/Application Data
                HRESULT res;
                if (current_user_appdata)
                    res = gfp(0, CSIDL_APPDATA, 0, 0, hpath);
                else
                    res = gfp(0, CSIDL_COMMON_APPDATA, 0, 0, hpath);
                if (res != S_FALSE && res != E_FAIL && res != E_INVALIDARG) {
                    script_h.save_path = new char[strlen(hpath) + strlen(gameid) + 3];
                    sprintf(script_h.save_path, "%s%c%s%c",
                            hpath, DELIMITER, gameid, DELIMITER);
                    CreateDirectory(script_h.save_path, 0);
                }
            }
            FreeLibrary(shdll);
        }
        if (script_h.save_path == NULL) {
            // Error; assume ancient Windows. In this case it's safe
            // to use the archive path!
            setSavePath(archive_path.get_path(0));
        }
#elif defined MACOSX
        // On Mac OS X, place in ~/Library/Application Support/<gameid>/
        char *path;
        ONSCocoa::getGameAppSupportPath(&path, gameid);
        setSavePath(path);
        delete[] path;
#elif defined LINUX
        // On Linux (and similar *nixen), place in ~/.gameid
        passwd* pwd = getpwuid(getuid());
        if (pwd) {
            script_h.save_path = new char[strlen(pwd->pw_dir) + strlen(gameid) + 4];
            sprintf(script_h.save_path, "%s%c.%s%c", 
                    pwd->pw_dir, DELIMITER, gameid, DELIMITER);
            mkdir(script_h.save_path, 0755);
        }
        else setSavePath(archive_path.get_path(0));
#else
        // Fall back on default ONScripter behaviour if we don't have
        // any better ideas.
        setSavePath(archive_path.get_path(0));
#endif
    }
    if ( script_h.game_identifier ) {
        delete[] script_h.game_identifier; 
        script_h.game_identifier = NULL; 
    }

    if (strcmp(script_h.save_path, archive_path.get_path(0)) != 0) {
        // insert save_path onto the front of archive_path
        DirPaths new_path = DirPaths(script_h.save_path);
        new_path.add(archive_path);
        archive_path = new_path;
    }

#ifdef USE_LUA
    lua_handler.init(this, &script_h);
#endif

    //initialize cmd function table hash
    int idx = 0;
    while (func_lut[idx].method){
        int j = func_lut[idx].command[0]-'a';
        if (func_hash[j].start == -1) func_hash[j].start = idx;
        func_hash[j].end = idx;
        idx++;
    }

#ifdef WIN32
    if (debug_level > 0) {
        openDebugFolders();
    }
#endif

    initSDL();
    
    //mEventQueueMutex = SDL_CreateMutex();
    //mMusicMutex = SDL_CreateMutex();


    //printf("%s\n", SDL_GetCurrentAudioDriver());

    image_surface = SDL_CreateRGBSurface( SDL_SWSURFACE, 1, 1, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 );
    accumulation_surface = AnimationInfo::allocSurface( screen_width, screen_height );
    backup_surface       = AnimationInfo::allocSurface( screen_width, screen_height );
    effect_src_surface   = AnimationInfo::allocSurface( screen_width, screen_height );
    effect_dst_surface   = AnimationInfo::allocSurface( screen_width, screen_height );
    effect_tmp_surface   = AnimationInfo::allocSurface( screen_width, screen_height );
    SDL_SetSurfaceBlendMode( accumulation_surface, SDL_BLENDMODE_NONE );
    SDL_SetSurfaceBlendMode( backup_surface, SDL_BLENDMODE_NONE );
    SDL_SetSurfaceBlendMode( effect_src_surface, SDL_BLENDMODE_NONE );
    SDL_SetSurfaceBlendMode( effect_dst_surface, SDL_BLENDMODE_NONE );
    SDL_SetSurfaceBlendMode( effect_tmp_surface, SDL_BLENDMODE_NONE );

    num_loaded_images = 10; // to suppress temporal increase at the start-up

    text_info.num_of_cells = 1;
    text_info.allocImage( screen_width, screen_height );
    text_info.fill(0, 0, 0, 0);

    // ----------------------------------------
    // Initialize font
    delete[] font_file;
    if ( default_font ){
        font_file = new char[ strlen(default_font) + 1 ];
        sprintf( font_file, "%s", default_font );
    }
    else{
        FILE *fp;
        font_file = new char[ archive_path.max_path_len() + strlen(FONT_FILE) + 1 ];
        for (int i=0; i<(archive_path.get_num_paths()); i++) {
            // look through archive_path(s) for the font file
            sprintf( font_file, "%s%s", archive_path.get_path(i), FONT_FILE );
            //printf("font file: %s\n", font_file);
            fp = std::fopen(font_file, "rb");
            if (fp != NULL) {
                fclose(fp);
                break;
            }
        }
        //sprintf( font_file, "%s%s", archive_path->get_path(0), FONT_FILE );
        setStr(&default_font, FONT_FILE);
    }

    // ----------------------------------------
    // Sound related variables
#ifdef ONSCRIPTER_CDAUDIO
    this->cdaudio_flag = cdaudio_flag;
    if ( cdaudio_flag ){
        if ( cdrom_drive_number >= 0 && cdrom_drive_number < SDL_CDNumDrives() )
            cdrom_info = SDL_CDOpen( cdrom_drive_number );
        if ( !cdrom_info ){
            fprintf(stderr, "Couldn't open default CD-ROM: %s\n", SDL_GetError());
        }
        else if ( cdrom_info && !CD_INDRIVE( SDL_CDStatus( cdrom_info ) ) ) {
            fprintf( stderr, "no CD-ROM in the drive\n" );
            SDL_CDClose( cdrom_info );
            cdrom_info = NULL;
        }
    }
#endif
    // ----------------------------------------
    // Initialize misc variables

    internal_timer = SDL_GetTicks();

    loadEnvData();

    ScriptHandler::LanguageScript cur_pref = script_h.preferred_script;
    defineresetCommand();
    readToken();

    if ( sentence_font.openFont( font_file, screen_ratio1, screen_ratio2 ) == NULL ){
#if defined(MACOSX)
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN, "Could not find the font file '%s'.\n"
                 "Please ensure it is present with the game data.", default_font);
#else
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN, "Could not find the font file '%s'.", default_font);
#endif
        errorAndExit(script_h.errbuf, NULL, "Missing font file", true);
        return -1;
    }

    //Do a little check for whether the font supports Japanese glyphs,
    //if either system-menu or preferred text mode is Japanese
    if ( (script_h.system_menu_script == ScriptHandler::JAPANESE_SCRIPT) ||
         (cur_pref == ScriptHandler::JAPANESE_SCRIPT) ) {
        if (debug_level > 0)
            printf("Checking font for Japanese support\n");
        Uint16 test = 0x300c; // Unicode JP start-quote
        int error, minx1, maxx1, miny1, maxy1;
        int minx2, maxx2, miny2, maxy2;
        int minx3, maxx3, miny3, maxy3;
        error = TTF_GlyphMetrics((TTF_Font*)sentence_font.ttf_font, test,
                                 &minx1, &maxx1, &miny1, &maxy1, NULL);
        if (debug_level > 0)
            printf("JP start-quote glyph metrics: x=(%d,%d), y=(%d,%d)\n",
                   minx1, maxx1, miny1, maxy1);
        test = 0x300d; // Unicode JP end-quote
        error = TTF_GlyphMetrics((TTF_Font*)sentence_font.ttf_font, test,
                                     &minx2, &maxx2, &miny2, &maxy2, NULL);
        if (debug_level > 0)
            printf("JP end-quote glyph metrics: x=(%d,%d), y=(%d,%d)\n",
                   minx2, maxx2, miny2, maxy2);
        test = 0x3042; // Unicode Hiragana letter A
        error = TTF_GlyphMetrics((TTF_Font*)sentence_font.ttf_font, test,
                                     &minx3, &maxx3, &miny3, &maxy3, NULL);
        if (debug_level > 0)
            printf("JP hiragana A glyph metrics: x=(%d,%d), y=(%d,%d)\n",
                   minx3, maxx3, miny3, maxy3);
        if (error != 0) {
            // font doesn't have the glyph, so set to use English mode default
            setEnglishDefault();
            setEnglishMenu();
            setDefaultMenuLabels();
            printf("Font file doesn't support Japanese; reverting to English\n");
        } else if ((minx1 == minx2) && (maxx1 == maxx2) && (miny1 == miny2) && 
                   (maxy1 == maxy2) && (minx1 == minx3) && (maxx1 == maxx3) &&
                   (miny1 == miny3) && (maxy1 == maxy3)) {
            // font has equal metrics for quotes, so assume glyphs are both null
            setEnglishDefault();
            setEnglishMenu();
            setDefaultMenuLabels();
            printf("Font file has equivalent metrics for 3 Japanese glyphs - ");
            printf("assuming it doesn't support Japanese; reverting to English\n");
        } else if (debug_level > 0) {
            printf("Ok, the font appears to support Japanese\n");
        }
    }

    // preferred was set by command-line option if it was set, so
    // make it the default from now on; default to English otherwise
    if (script_h.default_script == ScriptHandler::NO_SCRIPT_PREF) {
        if (cur_pref == ScriptHandler::NO_SCRIPT_PREF)
            script_h.preferred_script = ScriptHandler::LATIN_SCRIPT;
        else
            script_h.preferred_script = cur_pref;
        script_h.default_script = script_h.preferred_script;
    }
    if (script_h.system_menu_script == ScriptHandler::NO_SCRIPT_PREF) {
        setEnglishMenu();
        setDefaultMenuLabels();
    }

    return 0;
}

void ONScripter::reset()
{
    resetFlags();

    if (sin_table) delete[] sin_table;
    if (cos_table) delete[] cos_table;
    sin_table = cos_table = NULL;
    if (whirl_table) delete[] whirl_table;
    whirl_table = NULL;

    if (breakup_cells) delete[] breakup_cells;
    if (breakup_mask) delete[] breakup_mask;
    if (breakup_cellforms) delete[] breakup_cellforms;

    ons_gfx::resetResizeBuffer();

    if (string_buffer_breaks) delete[] string_buffer_breaks;
    string_buffer_breaks = NULL;

    resetSentenceFont();

    setStr(&getret_str, NULL);
    getret_int = 0;

    if (async_movie) stopMovie(async_movie);
    async_movie = NULL;
    if (movie_buffer) delete[] movie_buffer;
    movie_buffer = NULL;
    if (surround_rects) delete[] surround_rects;
    surround_rects = NULL;

    resetSub();

    /* ---------------------------------------- */
    /* Load global variables if available */
    if ( loadFileIOBuf( "gloval.sav" ) == 0 ||
         loadFileIOBuf( "global.sav" ) == 0 )
        readVariables( script_h.global_variable_border, VARIABLE_RANGE );

}

void ONScripter::resetSub()
{
    int i;

    for ( i=0 ; i<script_h.global_variable_border ; i++ )
        script_h.getVariableData(i).reset(false);

    resetFlagsSub();

    skip_mode = (skip_mode & SKIP_TO_EOP) ? SKIP_TO_EOP : SKIP_NONE;
    setStr(&trap_dest, NULL);

    resetSentenceFont();

    deleteNestInfo();
    deleteButtonLink();
    deleteSelectLink();

    stopCommand();
    loopbgmstopCommand();
    for (int ch=0; ch<ONS_MIX_CHANNELS ; ch++)
        channel_preloaded[ch] = false; //reset; also ensures that all dwaves stop
    stopAllDWAVE();
    setStr(&loop_bgm_name[1], NULL);

    // ----------------------------------------
    // reset AnimationInfo
    btndef_info.reset();
    bg_info.reset();
    setStr( &bg_info.file_name, "black" );
    createBackground();
    for (i=0 ; i<3 ; i++) tachi_info[i].reset();
    for (i=0 ; i<MAX_SPRITE_NUM ; i++) sprite_info[i].reset();
    for (i=0 ; i<MAX_SPRITE2_NUM ; i++) sprite2_info[i].reset();
    barclearCommand();
    prnumclearCommand();
    for (i=0 ; i<2 ; i++) cursor_info[i].reset();
    for (i=0 ; i<4 ; i++) lookback_info[i].reset();

    //Mion: reset textbtn
    deleteTextButtonInfo();
    readColor( &linkcolor[0], "#FFFF22" ); // yellow - link color
    readColor( &linkcolor[1], "#88FF88" ); // cyan - mouseover link color

    dirty_rect.fill( screen_width, screen_height );
}

void ONScripter::resetFlags()
{
    automode_flag = false;
    autoclick_time = 0;
    btntime2_flag = false;
    btntime_value = 0;
    btnwait_time = 0;
    
    is_exbtn_enabled = false;

    system_menu_enter_flag = false;
    system_menu_mode = SYSTEM_NULL;
    key_pressed_flag = false;
    shift_pressed_status = 0;
    ctrl_pressed_status = 0;
#ifdef MACOSX
    apple_pressed_status = 0;
#endif
    display_mode = shelter_display_mode = DISPLAY_MODE_NORMAL;
    event_mode = shelter_event_mode = IDLE_EVENT_MODE;
    did_leavetext = false;
    in_effect_blank = false;
    skip_effect = false;
    effectskip_flag = true; //on by default

    current_over_button = 0;
    current_button_valid = false;
    variable_edit_mode = NOT_EDIT_MODE;

    new_line_skip_flag = false;
    text_on_flag = true;
    draw_cursor_flag = shelter_draw_cursor_flag = false;

    // ----------------------------------------
    // Sound related variables

    wave_play_loop_flag = false;
    seqmusic_play_loop_flag = false;
    music_play_loop_flag = false;
    cd_play_loop_flag = false;
    mp3save_flag = false;
    current_cd_track = -1;
    mp3fadeout_duration = mp3fadein_duration = 0;
    bgmdownmode_flag = false;

    movie_click_flag = movie_loop_flag = false;

    disableGetButtonFlag();
}

void ONScripter::resetFlagsSub()
{
    int i=0;
    
    for ( i=0 ; i<3 ; i++ ) human_order[i] = 2-i; // "rcl"

    all_sprite_hide_flag = false;
    all_sprite2_hide_flag = false;

    refresh_window_text_mode = REFRESH_NORMAL_MODE | REFRESH_WINDOW_MODE |
                               REFRESH_TEXT_MODE;
    erase_text_window_mode = 1;

    monocro_flag = false;
    monocro_color[0] = monocro_color[1] = monocro_color[2] = 0;
    nega_mode = 0;
    clickstr_state = CLICK_NONE;
    trap_mode = TRAP_NONE;

    last_keypress = KEYPRESS_NULL;

    saveon_flag = true;
    internal_saveon_flag = true;

    textgosub_clickstr_state = CLICK_NONE;
    indent_offset = 0;
    line_enter_status = 0;
    page_enter_status = 0;
    last_textpos_xy[0] = last_textpos_xy[1] = 0;
    line_has_nonspace = false;

    for (i=0 ; i<SPRITE_NUM_LAST_LOADS ; i++) last_loaded_sprite[i] = -1;
    last_loaded_sprite_ind = 0;

    txtbtn_start_num = next_txtbtn_num = 1;
    in_txtbtn = false;
    txtbtn_show = false;
    txtbtn_visible = false;
}

void ONScripter::resetSentenceFont()
{
    sentence_font.reset();
    sentence_font.font_size_xy[0] = DEFAULT_FONT_SIZE;
    sentence_font.font_size_xy[1] = DEFAULT_FONT_SIZE;
    sentence_font.top_xy[0] = 21;
    sentence_font.top_xy[1] = 16;// + sentence_font.font_size;
    sentence_font.num_xy[0] = 23;
    sentence_font.num_xy[1] = 16;
    sentence_font.pitch_xy[0] = sentence_font.font_size_xy[0];
    sentence_font.pitch_xy[1] = 2 + sentence_font.font_size_xy[1];
    sentence_font.wait_time = 20;
    sentence_font.window_color[0] = sentence_font.window_color[1] = sentence_font.window_color[2] = 0x99;
    sentence_font.color[0] = sentence_font.color[1] = sentence_font.color[2] = 0xff;
    sentence_font_info.reset();
    sentence_font_info.pos.x = 0;
    sentence_font_info.pos.y = 0;
    sentence_font_info.pos.w = screen_width;
    sentence_font_info.pos.h = screen_height;
}

bool ONScripter::doErrorBox( const char *title, const char *errstr, bool is_simple, bool is_warning )
//returns true if we need to exit
{
    //The OS X dialog box routines are crashing when in fullscreen mode,
    //so let's switch to windowed mode just in case
    menu_windowCommand();

#if defined(MACOSX)
    if (is_simple && !is_warning)
        ONSCocoa::alertbox(title, errstr);
    else {
        if (ONSCocoa::scriptErrorBox(title, errstr, is_warning, ONSCocoa::ENC_SJIS) == SCRIPTERROR_IGNORE)
            return false;
    }

#elif defined(WIN32) && defined(USE_MESSAGEBOX)
    char errtitle[256];
    HWND pwin = NULL;
    SDL_SysWMinfo info;
    UINT mb_type = MB_OK;
    SDL_VERSION(&info.version);
    SDL_GetWMInfo(&info);

    if (SDL_GetWMInfo(&info) == 1) {
        pwin = info.window;
        snprintf(errtitle, 256, "%s", title);
    } else {
        snprintf(errtitle, 256, "ONScripter-EN: %s", title);
    }

    if (is_warning) {
        //Retry and Ignore both continue, Abort exits
        //would rather do an Ignore/Exit button set, oh well
        mb_type = MB_ABORTRETRYIGNORE|MB_DEFBUTTON3|MB_ICONWARNING;
    }
    else
        mb_type |= MB_ICONERROR;
    int res = MessageBox(pwin, errstr, errtitle, mb_type);
    if (is_warning)
        return (res == IDABORT); //should do exit if got Abort
#else
    //no errorbox support; at least send the info to stderr
    fprintf(stderr, " ***[Info] %s *** \n%s\n", title, errstr);
#endif

    // get affairs in order
    if (errorsave) {
        saveon_flag = internal_saveon_flag = true;
        //save current game state to save999.dat,
        //without exiting if I/O Error
        saveSaveFile( 999, NULL, true );
    }
#ifdef WIN32
    openDebugFolders();
#endif

    quit(true);

    return true; //should do exit
}

#ifdef WIN32
void ONScripter::openDebugFolders()
{
    // to make it easier to debug user issues on Windows, open
    // the current directory, save_path and ONScripter output folders
    // in Explorer
    HMODULE shdll = LoadLibrary("shell32");
    if (shdll) {
        char hpath[MAX_PATH];
        bool havefp = false;
        GETFOLDERPATH gfp = GETFOLDERPATH(GetProcAddress(shdll, "SHGetFolderPathA"));
        if (gfp) {
            HRESULT res = gfp(0, CSIDL_APPDATA, 0, 0, hpath); //now user-based
            if (res != S_FALSE && res != E_FAIL && res != E_INVALIDARG) {
                havefp = true;
                sprintf((char *)&hpath + strlen(hpath), "%c%s",
                        DELIMITER, "ONScripter-EN");
            }
        }
        typedef HINSTANCE (WINAPI *SHELLEXECUTE)(HWND, LPCSTR, LPCSTR,
                           LPCSTR, LPCSTR, int);
        SHELLEXECUTE shexec =
            SHELLEXECUTE(GetProcAddress(shdll, "ShellExecuteA"));
        if (shexec) {
            shexec(NULL, "open", "", NULL, NULL, SW_SHOWNORMAL);
            shexec(NULL, "open", script_h.save_path, NULL, NULL, SW_SHOWNORMAL);
            if (havefp)
                shexec(NULL, "open", hpath, NULL, NULL, SW_SHOWNORMAL);
        }
        FreeLibrary(shdll);
    }
}
#endif

bool intersectRects( SDL_Rect &result, SDL_Rect rect1, SDL_Rect rect2) {
    if ( (rect1.w == 0) || (rect1.h == 0) ) {
        result = rect1;
        return false;
    } else if ( (rect2.w == 0) || (rect2.h == 0) ) {
        result = rect2;
        return false;
    }
    if (rect1.x < rect2.x) {
        result = rect2;
        if ((rect1.x + rect1.w) < rect2.x) {
            result.w = 0;
            return false;
        } else if ((rect1.x + rect1.w) < (rect2.x + rect2.w)){
            result.w = rect1.x + rect1.w - rect2.x;
        }
    } else {
        result = rect1;
        if ((rect2.x + rect2.w) < rect1.x) {
            result.w = 0;
            return false;
        } else if ((rect2.x + rect2.w) < (rect1.x + rect1.w)){
            result.w = rect2.x + rect2.w - rect1.x;
        }
    }
    if (rect1.y < rect2.y) {
        result.y = rect2.y;
        if ((rect1.y + rect1.h) < rect2.y) {
            result.h = 0;
            return false;
        } else if ((rect1.y + rect1.h) < (rect2.y + rect2.h)){
            result.h = rect1.y + rect1.h - rect2.y;
        } else
            result.h = rect2.h;
    } else {
        result.y = rect1.y;
        if ((rect2.y + rect2.h) < rect1.y) {
            result.h = 0;
            return false;
        } else if ((rect2.y + rect2.h) < (rect1.y + rect1.h)){
            result.h = rect2.y + rect2.h - rect1.y;
        } else
            result.h = rect1.h;
    }

    return true;
}

void ONScripter::flush( int refresh_mode, SDL_Rect *rect, bool clear_dirty_flag, bool direct_flag )
{
    if ( direct_flag ){
        flushDirect( *rect, refresh_mode );
    }
    else{
        if ( rect ) dirty_rect.add( *rect );

        if (dirty_rect.bounding_box.w * dirty_rect.bounding_box.h > 0)
            flushDirect( dirty_rect.bounding_box, refresh_mode );
    }

    if ( clear_dirty_flag ) dirty_rect.clear();
}

void ONScripter::flushDirect( SDL_Rect &rect, int refresh_mode, bool updaterect )
{
    //printf("flush %d: %d %d %d %d\n", refresh_mode, rect.x, rect.y, rect.w, rect.h );

    if (surround_rects) {
        // playing a movie, need to avoid overpainting it
        SDL_Rect tmp_rects[4];
        for (int i=0; i<4; ++i) {
            if (intersectRects(tmp_rects[i], rect, surround_rects[i])) {
                refreshSurface( accumulation_surface, &tmp_rects[i], refresh_mode );
                SDL_BlitSurface( accumulation_surface, &tmp_rects[i], screen_surface, &tmp_rects[i] );
            }
        }
        //FIXME: 
        if (updaterect)
        {
          //SDL_UpdateRects(screen_surface, 4, tmp_rects);
          UpdateScreen(rect);
        }
    } else { 
        refreshSurface( accumulation_surface, &rect, refresh_mode );
        SDL_BlitSurface( accumulation_surface, &rect, screen_surface, &rect );
        //FIXME: 
        if (updaterect)
        {
          //SDL_UpdateRect(screen_surface, rect.x, rect.y, rect.w, rect.h);
          UpdateScreen(rect);
        }
    }
}

void ONScripter::mouseOverCheck( int x, int y )
{
    int c = -1;

    last_mouse_state.x = x;
    last_mouse_state.y = y;

    /* ---------------------------------------- */
    /* Check button */
    int button = 0;
    bool found = false;
    ButtonLink *p_button_link = root_button_link.next;
    ButtonLink *cur_button_link = NULL;
    while( p_button_link ){
        c++;
        cur_button_link = p_button_link;
        while (cur_button_link) {
            if ( x >= cur_button_link->select_rect.x &&
                 x < cur_button_link->select_rect.x + cur_button_link->select_rect.w &&
                 y >= cur_button_link->select_rect.y &&
                 y < cur_button_link->select_rect.y + cur_button_link->select_rect.h &&
                 ( cur_button_link->button_type != ButtonLink::TEXT_BUTTON ||
                   ( txtbtn_visible && txtbtn_show ) )){
                bool in_button = true;
                if (transbtn_flag){
                    in_button = false;
                    AnimationInfo *anim = NULL;
                    if ( cur_button_link->button_type == ButtonLink::SPRITE_BUTTON ||
                         cur_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON )
                        anim = &sprite_info[ cur_button_link->sprite_no ];
                    else
                        anim = cur_button_link->anim[0];
                    int alpha = anim->getPixelAlpha(x - cur_button_link->select_rect.x,
                                                    y - cur_button_link->select_rect.y);
                    if (alpha > TRANSBTN_CUTOFF)
                        in_button = true;
                }
                if (in_button){
                    button = cur_button_link->no;
                    found = true;
                    break;
                }
            }
            cur_button_link = cur_button_link->same;
        }
        if (found) break;
        p_button_link = p_button_link->next;
    }

    if ( (current_button_valid != found) ||
         (current_over_button != button) ) {
//printf("mouseOverCheck: refresh (current=%d, button=%d)", current_over_button, button);
//if (current_button_valid) printf(" valid");
//printf (" cur_link=%d, p_link=%d\n", (unsigned int)current_button_link, (unsigned int)p_button_link);
        DirtyRect dirty = dirty_rect;
        dirty_rect.clear();

        SDL_Rect check_src_rect = {0, 0, 0, 0};
        SDL_Rect check_dst_rect = {0, 0, 0, 0};
        if (current_button_valid){
            cur_button_link = current_button_link;
            while (cur_button_link) {
                cur_button_link->show_flag = 0;
                check_src_rect = cur_button_link->image_rect;
                if ( cur_button_link->button_type == ButtonLink::SPRITE_BUTTON ||
                     cur_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON ){
                    sprite_info[ cur_button_link->sprite_no ].visible = true;
                    sprite_info[ cur_button_link->sprite_no ].setCell(0);
                }
                else if ( cur_button_link->button_type == ButtonLink::TMP_SPRITE_BUTTON ){
                    cur_button_link->show_flag = 1;
                    cur_button_link->anim[0]->visible = true;
                    cur_button_link->anim[0]->setCell(0);
                }
                else if ( cur_button_link->button_type == ButtonLink::TEXT_BUTTON ){
                    if (txtbtn_visible) {
                        cur_button_link->show_flag = 1;
                        cur_button_link->anim[0]->visible = true;
                        cur_button_link->anim[0]->setCell(0);
                    }
                }
                else if ( cur_button_link->anim[1] != NULL ){
                    cur_button_link->show_flag = 2;
                }
                dirty_rect.add( cur_button_link->image_rect );
                if ( is_exbtn_enabled && exbtn_d_button_link.exbtn_ctl ){
                    decodeExbtnControl( exbtn_d_button_link.exbtn_ctl, &check_src_rect, &check_dst_rect );
                }

                cur_button_link = cur_button_link->same;
            }
        } else {
            if ( is_exbtn_enabled && exbtn_d_button_link.exbtn_ctl ){
                decodeExbtnControl( exbtn_d_button_link.exbtn_ctl, &check_src_rect, &check_dst_rect );
            }
        }

        if ( p_button_link ){
            if ( system_menu_mode != SYSTEM_NULL ){
                if ( menuselectvoice_file_name[MENUSELECTVOICE_OVER] )
                    playSound(menuselectvoice_file_name[MENUSELECTVOICE_OVER],
                              SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }
            else{
                if ( selectvoice_file_name[SELECTVOICE_OVER] )
                    playSound(selectvoice_file_name[SELECTVOICE_OVER],
                              SOUND_WAVE|SOUND_OGG, false, MIX_WAVE_CHANNEL);
            }
            cur_button_link = p_button_link;
            while (cur_button_link) {
                check_dst_rect = cur_button_link->image_rect;
                if ( cur_button_link->button_type == ButtonLink::SPRITE_BUTTON ||
                     cur_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON ){
                    sprite_info[ cur_button_link->sprite_no ].setCell(1);
                    sprite_info[ cur_button_link->sprite_no ].visible = true;
                    if ((cur_button_link == p_button_link) && is_exbtn_enabled &&
                        (cur_button_link->button_type == ButtonLink::EX_SPRITE_BUTTON)){
                        decodeExbtnControl( cur_button_link->exbtn_ctl, &check_src_rect, &check_dst_rect );
                    }
                }
                else if ( cur_button_link->button_type == ButtonLink::TMP_SPRITE_BUTTON){
                    cur_button_link->show_flag = 1;
                    cur_button_link->anim[0]->visible = true;
                    cur_button_link->anim[0]->setCell(1);
                }
                else if ( cur_button_link->button_type == ButtonLink::TEXT_BUTTON &&
                          txtbtn_show && txtbtn_visible ){
                    cur_button_link->show_flag = 1;
                    cur_button_link->anim[0]->visible = true;
                    cur_button_link->anim[0]->setCell(1);
                    if ((cur_button_link == p_button_link) &&
                        is_exbtn_enabled && cur_button_link->exbtn_ctl ){
                        decodeExbtnControl( cur_button_link->exbtn_ctl, &check_src_rect, &check_dst_rect );
                    }
                }
                else if ( cur_button_link->button_type == ButtonLink::NORMAL_BUTTON ||
                          cur_button_link->button_type == ButtonLink::LOOKBACK_BUTTON ){
                    cur_button_link->show_flag = 1;
                }
                dirty_rect.add( cur_button_link->image_rect );
                cur_button_link = cur_button_link->same;
            }
            if (c>=0)
                shortcut_mouse_line = c;
        }
        current_button_link = p_button_link;

        flush( refreshMode() );
        dirty_rect = dirty;
    }
    current_button_valid = found;
    current_over_button = button;
}

void ONScripter::executeLabel()
{
    int last_token_line = -1;

  executeLabelTop:

    while ( current_line<current_label_info.num_of_lines ){
        if ((debug_level > 0) && (last_token_line != current_line) &&
            (script_h.getStringBuffer()[0] != 0x0a)) {
            printf("\n*****  executeLabel %s:%d/%d:mode=%s *****\n",
                   current_label_info.name,
                   current_line,
                   current_label_info.num_of_lines,
                   (display_mode == 0 ? "normal" : (display_mode == 1 ? "text" : "updated")));
            fflush(stdout);
        }
        last_token_line = current_line;

        if ( script_h.getStringBuffer()[0] == '~' ){
            last_tilde.next_script = script_h.getNext();
            readToken();
            continue;
        }
        if ( break_flag && !script_h.isName("next") ){
            if ( script_h.getStringBuffer()[0] == 0x0a )
                current_line++;

            if ((script_h.getStringBuffer()[0] != ':') &&
                (script_h.getStringBuffer()[0] != ';') &&
                (script_h.getStringBuffer()[0] != 0x0a))
                script_h.skipToken();

            readToken();
            continue;
        }

        if ( kidokuskip_flag && (skip_mode & SKIP_NORMAL) &&
             kidokumode_flag && !script_h.isKidoku() )
            skip_mode &= ~SKIP_NORMAL;

        // FIXME:
        ////check for quit event before running each command, for safety
        ////(this won't prevent all window lockups, but should give some
        //// greater chance of the user being able to quit when one happens)
        //if ( SDL_PumpEvents(), SDL_PeepEvents( NULL, 1, SDL_PEEKEVENT, SDL_QUITMASK) )
        //    endCommand();

        int ret = ScriptParser::parseLine();
        if ( ret == RET_NOMATCH ) ret = this->parseLine();

        if ( ret & (RET_SKIP_LINE | RET_EOL) ){
            if (ret & RET_SKIP_LINE) script_h.skipLine();
            if (++current_line >= current_label_info.num_of_lines) break;
        }

        if ( ret & RET_EOT ) processEOT();
        
        if (!(ret & RET_NO_READ)) readToken();
    }

    current_label_info = script_h.lookupLabelNext( current_label_info.name );
    current_line = 0; last_token_line = -1;

    if ( current_label_info.start_address != NULL ){
        script_h.setCurrent( current_label_info.label_header );
        readToken();
        goto executeLabelTop;
    }

    fprintf( stderr, " ***** End *****\n");
    endCommand();
}

void ONScripter::runScript()
{
    readToken();

    int ret = ScriptParser::parseLine();
    if ( ret == RET_NOMATCH ) ret = this->parseLine();
}

int ONScripter::parseLine( )
{
    int ret;
    char *cmd = script_h.getStringBuffer();
    if (cmd[0] == '_'){
        int c=0;
        while (cmd[c+1] != 0) {
            cmd[c] = cmd[c+1];
            c++;
        }
        cmd[c] = '\0';
    }
    const char *s_buf = script_h.getStringBuffer();
    if ( !script_h.isText() && !script_h.isPretext() ){
        snprintf(script_h.current_cmd, 64, "%s", s_buf);
        //Check against builtin cmds
        if (cmd[0] >= 'a' && cmd[0] <= 'z'){
            FuncHash &fh = func_hash[cmd[0]-'a'];
            for (int i=fh.start ; i<=fh.end ; i++){
                if ( !strcmp( func_lut[i].command, cmd ) ){
                    return (this->*func_lut[i].method)();
                }
            }
        }

        script_h.current_cmd_type = ScriptHandler::CMD_BUILTIN;
        if ( *s_buf == 0x0a ){
            script_h.current_cmd_type = ScriptHandler::CMD_NONE;
            return RET_CONTINUE | RET_EOL;
        }
        else if ((s_buf[0] == 'v') && (s_buf[1] >= '0') && (s_buf[1] <= '9')){
            strcpy(script_h.current_cmd, "vNUM");
            return vCommand();
        }
        else if ((s_buf[0] == 'd') && (s_buf[1] == 'v') &&
                 (s_buf[2] >= '0') && (s_buf[2] <= '9')){
            strcpy(script_h.current_cmd, "dvNUM");
            return dvCommand();
        }
        else if ((s_buf[0] == 'm') && (s_buf[1] == 'v') &&
                 (s_buf[2] >= '0') && (s_buf[2] <= '9')){
            strcpy(script_h.current_cmd, "mvNUM");
            return mvCommand();
        }

        script_h.current_cmd_type = ScriptHandler::CMD_UNKNOWN;
        snprintf(script_h.errbuf, MAX_ERRBUF_LEN, "command [%s] is not supported yet!!", s_buf );
        errorAndCont(script_h.errbuf);

        script_h.skipToken();

        return RET_CONTINUE;
    }

    /* Text/Pretext */

    if ( current_mode == DEFINE_MODE )
        errorAndExit( "text cannot be displayed while in the define section." );
    ret = textCommand();
    //Mion: moved all text processing into textCommand & its subfunctions

    return ret;
}

void ONScripter::readToken()
{
    bool pretext_check = false;
    if (pretextgosub_label && 
        (!pagetag_flag || (page_enter_status == 0)))
        pretext_check = true;

    script_h.readToken(pretext_check);
    string_buffer_offset = 0;

    if (script_h.isText() && (linepage_mode > 0) &&
        (script_h.getNext()[0] == 0x0a)){
        // ugly work around
        unsigned int len = strlen(script_h.getStringBuffer());
        //Mion: text buffers don't end with newline anymore...
        // checked next_script for the newline
        if (script_h.getStringBuffer()[len-1] == '_'){
            script_h.trimStringBuffer(1);
        } else if ((script_h.getStringBuffer()[len-1] != '@') &&
                   (script_h.getStringBuffer()[len-1] != '\\')){
            if (linepage_mode == 1){
                script_h.addStringBuffer('\\');
            }
            else {
                // insert a clickwait-or-newpage
                script_h.addStringBuffer('\\');
                script_h.addStringBuffer('@');
            }
        }
    }
}

/* ---------------------------------------- */
void ONScripter::processTextButtonInfo()
{
    TextButtonInfoLink *info = text_button_info.next;

    if (info) txtbtn_show = true;
    while (info) {
        ButtonLink *firstbtn = NULL;
        char *text = info->prtext;
        char *text2;
        Fontinfo f_info = sentence_font;
        //f_info.clear();
        f_info.xy[0] = info->xy[0];
        f_info.xy[1] = info->xy[1];
        setColor(f_info.off_color, linkcolor[0]);
        setColor(f_info.on_color, linkcolor[1]);
        do {
            text2 = strchr(text, 0x0a);
            if (text2) {
                *text2 = '\0';
            }
            ButtonLink *txtbtn = getSelectableSentence(text, &f_info, true, false, false);
            //printf("made txtbtn: %d '%s'\n", info->no, text);
            txtbtn->button_type = ButtonLink::TEXT_BUTTON;
            txtbtn->no = info->no;
            if (!txtbtn_visible)
                txtbtn->show_flag = 0;
            if (firstbtn)
                firstbtn->connect(txtbtn);
            else
                firstbtn = txtbtn;
            f_info.xy[0] = info->xy[0];
            f_info.xy[1] = info->xy[1];
            f_info.newLine();
            if (text2) {
                *text2 = 0x0a;
                text2++;
            }
            text = text2;
        } while (text2);
        root_button_link.insert(firstbtn);
        info->button = firstbtn;
        info = info->next;
    }
}

void ONScripter::deleteTextButtonInfo()
{
    TextButtonInfoLink *i1 = text_button_info.next;

    while( i1 ){
        TextButtonInfoLink *i2 = i1;
        // need to hide textbtn links
        ButtonLink *cur_button_link = i2->button;
        while (cur_button_link) {
            cur_button_link->show_flag = 0;
            cur_button_link = cur_button_link->same;
        }
        i1 = i1->next;
        delete i2;
    }
    text_button_info.next = NULL;
    txtbtn_visible = false;
    next_txtbtn_num = txtbtn_start_num;
}

void ONScripter::deleteButtonLink()
{
    ButtonLink *b1 = root_button_link.next;

    while( b1 ){
        ButtonLink *b2 = b1->same;
        while ( b2 ) {
            ButtonLink *b3 = b2;
            b2 = b2->same;
            delete b3;
        }
        b2 = b1;
        b1 = b1->next;
        if ( b2->button_type == ButtonLink::TEXT_BUTTON ) {
            // Need to delete ref to button from text_button_info
            TextButtonInfoLink *i1 = text_button_info.next;
            while (i1) {
                if (i1->button == b2)
                    i1->button = NULL;
                i1 = i1->next;
            }
        }
        delete b2;
    }
    root_button_link.next = NULL;
    current_button_link = NULL;
    current_button_valid = false;

    if ( exbtn_d_button_link.exbtn_ctl ) delete[] exbtn_d_button_link.exbtn_ctl;
    exbtn_d_button_link.exbtn_ctl = NULL;
    is_exbtn_enabled = false;
}

void ONScripter::refreshMouseOverButton()
{
    int mx, my;
    current_over_button = -1;
    current_button_valid = false;
    current_button_link = NULL;
    SDL_GetMouseState( &mx, &my );
    mouseOverCheck( mx, my );
}

/* ---------------------------------------- */
/* Delete select link */
void ONScripter::deleteSelectLink()
{
    SelectLink *link, *last_select_link = root_select_link.next;

    while ( last_select_link ){
        link = last_select_link;
        last_select_link = last_select_link->next;
        delete link;
    }
    root_select_link.next = NULL;
}

void ONScripter::clearCurrentPage()
{
    sentence_font.clear();

    int num = (sentence_font.num_xy[0]*2+1)*sentence_font.num_xy[1];
    if (sentence_font.getTateyokoMode() == Fontinfo::TATE_MODE)
        num = (sentence_font.num_xy[1]*2+1)*sentence_font.num_xy[1];

// TEST for ados backlog cutoff problem
    num *= 2;

    if ( current_page->text &&
         current_page->max_text != num ){
        delete[] current_page->text;
        current_page->text = NULL;
    }
    if ( !current_page->text ){
        current_page->text = new char[num];
        current_page->max_text = num;
    }
    current_page->text_count = 0;

    if (current_page->tag){
        delete[] current_page->tag;
        current_page->tag = NULL;
    }

    num_chars_in_sentence = 0;
    internal_saveon_flag = true;

    text_info.fill( 0, 0, 0, 0 );
    cached_page = current_page;

    deleteTextButtonInfo();
}

void ONScripter::displayTextWindow( SDL_Surface *surface, SDL_Rect &clip )
{
    if ( current_font->is_transparent ){

        SDL_Rect rect = {0, 0, screen_width, screen_height};
        if ( current_font == &sentence_font )
            rect = sentence_font_info.pos;

        if ( AnimationInfo::doClipping( &rect, &clip ) ) return;

        if ( rect.x + rect.w > surface->w ) rect.w = surface->w - rect.x;
        if ( rect.y + rect.h > surface->h ) rect.h = surface->h - rect.y;

        SDL_LockSurface( surface );
        ONSBuf *buf = (ONSBuf *)surface->pixels + rect.y * surface->w + rect.x;

        SDL_PixelFormat *fmt = surface->format;
        int color[3];
        color[0] = current_font->window_color[0] + 1;
        color[1] = current_font->window_color[1] + 1;
        color[2] = current_font->window_color[2] + 1;

        for ( int i=rect.y ; i<rect.y + rect.h ; i++ ){
            for ( int j=rect.x ; j<rect.x + rect.w ; j++, buf++ ){
                *buf = (((*buf & fmt->Rmask) >> fmt->Rshift) * color[0] >> 8) << fmt->Rshift |
                    (((*buf & fmt->Gmask) >> fmt->Gshift) * color[1] >> 8) << fmt->Gshift |
                    (((*buf & fmt->Bmask) >> fmt->Bshift) * color[2] >> 8) << fmt->Bshift;
            }
            buf += surface->w - rect.w;
        }

        SDL_UnlockSurface( surface );
    }
    else if ( sentence_font_info.image_surface ){
        drawTaggedSurface( surface, &sentence_font_info, clip );
    }
}

void ONScripter::newPage( bool next_flag )
{
    /* ---------------------------------------- */
    /* Set forward the text buffer */
    if ( current_page->text_count != 0 ){
        current_page = current_page->next;
        if ( start_page == current_page )
            start_page = start_page->next;
    }

    if ( next_flag ){
        indent_offset = 0;
        page_enter_status = 0;
    }
    
    clearCurrentPage();
    txtbtn_visible = false;
    txtbtn_show = false;

    flush( refreshMode(), &sentence_font_info.pos );
}

AnimationInfo* ONScripter::getSentence( char *buffer, Fontinfo *info, int num_cells, bool flush_flag, bool nofile_flag, bool skip_whitespace )
{
    //Mion: moved from getSelectableSentence and modified
    int current_text_xy[2];
    current_text_xy[0] = info->xy[0];
    current_text_xy[1] = info->xy[1];

    AnimationInfo *anim = new AnimationInfo();

    anim->trans_mode = AnimationInfo::TRANS_STRING;
    anim->is_single_line = false;
    anim->num_of_cells = num_cells;
    anim->color_list = new uchar3[ num_cells ];
    for (int i=0 ; i<3 ; i++){
        if (nofile_flag)
            anim->color_list[0][i] = info->nofile_color[i];
        else
            anim->color_list[0][i] = info->off_color[i];
        if (num_cells > 1)
            anim->color_list[1][i] = info->on_color[i];
    }
    anim->skip_whitespace = skip_whitespace;
    setStr( &anim->file_name, buffer );
    anim->orig_pos.x = info->x();
    anim->orig_pos.y = info->y();
    UpdateAnimPosXY(anim);
    anim->visible = true;

    setupAnimationInfo( anim, info );

    info->newLine();
    if (info->getTateyokoMode() == Fontinfo::YOKO_MODE)
        info->xy[0] = current_text_xy[0];
    else
        info->xy[1] = current_text_xy[1];

    dirty_rect.add( anim->pos );

    return anim;
}

struct ONScripter::ButtonLink *ONScripter::getSelectableSentence( char *buffer, Fontinfo *info, bool flush_flag, bool nofile_flag, bool skip_whitespace )
{
    ButtonLink *button_link = new ButtonLink();
    button_link->button_type = ButtonLink::TMP_SPRITE_BUTTON;
    button_link->show_flag = 1;

    AnimationInfo *anim = getSentence(buffer, info, 2, flush_flag,
                                      nofile_flag, skip_whitespace);
    button_link->anim[0] = anim;
    button_link->select_rect = button_link->image_rect = anim->pos;

    return button_link;
}

void ONScripter::decodeExbtnControl( const char *ctl_str, SDL_Rect *check_src_rect, SDL_Rect *check_dst_rect )
{
    char sound_name[256];
    int i, sprite_no, sprite_no2, cell_no;

    while( char com = *ctl_str++ ){
        if (com == 'C' || com == 'c'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            sprite_no2 = sprite_no;
            cell_no = -1;
            if ( *ctl_str == '-' ){
                ctl_str++;
                sprite_no2 = getNumberFromBuffer( &ctl_str );
            }
            for (i=sprite_no ; i<=sprite_no2 ; i++)
                refreshSprite( i, false, cell_no, NULL, NULL );
        }
        else if (com == 'P' || com == 'p'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            if ( *ctl_str == ',' ){
                ctl_str++;
                cell_no = getNumberFromBuffer( &ctl_str );
            }
            else
                cell_no = 0;
            refreshSprite( sprite_no, true, cell_no, check_src_rect, check_dst_rect );
        }
        else if (com == 'S' || com == 's'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            if      (sprite_no < 0) sprite_no = 0;
            else if (sprite_no >= ONS_MIX_CHANNELS) sprite_no = ONS_MIX_CHANNELS-1;
            if ( *ctl_str != ',' ) continue;
            ctl_str++;
            if ( *ctl_str != '(' ) continue;
            ctl_str++;
            char *buf = sound_name;
            while (*ctl_str != ')' && *ctl_str != '\0' ) *buf++ = *ctl_str++;
            *buf++ = '\0';
            playSound(sound_name, SOUND_WAVE|SOUND_OGG, false, sprite_no);
            if ( *ctl_str == ')' ) ctl_str++;
        }
        else if (com == 'M' || com == 'm'){
            sprite_no = getNumberFromBuffer( &ctl_str );
            SDL_Rect rect = sprite_info[ sprite_no ].pos;
            if ( *ctl_str != ',' ) continue;
            ctl_str++; // skip ','
            sprite_info[ sprite_no ].orig_pos.x = getNumberFromBuffer( &ctl_str );
            if ( *ctl_str != ',' ) {
                UpdateAnimPosXY(&sprite_info[ sprite_no ]);
                continue;
            }
            ctl_str++; // skip ','
            sprite_info[ sprite_no ].orig_pos.y = getNumberFromBuffer( &ctl_str );
            UpdateAnimPosXY(&sprite_info[ sprite_no ]);
            dirty_rect.add( rect );
            sprite_info[ sprite_no ].visible = true;
            dirty_rect.add( sprite_info[ sprite_no ].pos );
        }
    }
}

void ONScripter::loadCursor( int no, const char *str, int x, int y, bool abs_flag )
{
    cursor_info[ no ].setImageName( str );
    cursor_info[ no ].is_cursor = true;
    cursor_info[ no ].orig_pos.x = x;
    cursor_info[ no ].orig_pos.y = y;
    UpdateAnimPosXY(&cursor_info[ no ]);

    parseTaggedString( &cursor_info[ no ] );
    setupAnimationInfo( &cursor_info[ no ] );
    if ( filelog_flag )
        script_h.findAndAddLog( script_h.log_info[ScriptHandler::FILE_LOG], cursor_info[ no ].file_name, true ); // a trick for save file
    cursor_info[ no ].abs_flag = abs_flag;
    if ( cursor_info[ no ].image_surface )
        cursor_info[ no ].visible = true;
    else
        cursor_info[ no ].remove();
}

void ONScripter::saveAll(bool no_error)
{
    // only save the game state if save_path is set
    if (script_h.save_path != NULL) {
        saveEnvData();
        saveGlovalData(no_error);
        if ( filelog_flag )  writeLog( script_h.log_info[ScriptHandler::FILE_LOG] );
        if ( labellog_flag ) writeLog( script_h.log_info[ScriptHandler::LABEL_LOG] );
        if ( kidokuskip_flag ) script_h.saveKidokuData(no_error);
    }
}

void ONScripter::loadEnvData()
{
    volume_on_flag = true;
    text_speed_no = 1;
    skip_mode &= ~SKIP_TO_EOP;
    setStr( &default_env_font, NULL );
    cdaudio_on_flag = true;
    setStr( &default_cdrom_drive, NULL );
    kidokumode_flag = true;
    use_default_volume = true;
    bgmdownmode_flag = false;
    setStr( &savedir, NULL );
    automode_time = 1000;

    if (loadFileIOBuf( "envdata" ) == 0){
        use_default_volume = false;
        if (readInt() == 1 && window_mode == false) menu_fullCommand();
        if (readInt() == 0) volume_on_flag = false;
        text_speed_no = readInt();
        if (text_speed_no < 0 || text_speed_no > 2) {
            //catch corrupted text_speed_no
            snprintf(script_h.errbuf, MAX_ERRBUF_LEN,
                    "envdata has invalid text speed setting %d, changing to default",
                    text_speed_no);
            errorAndCont(script_h.errbuf, NULL, "Env Issue", true);
            text_speed_no = 1;
        }
        if (readInt() == 1) skip_mode |= SKIP_TO_EOP;
        readStr( &default_env_font );
        if (default_env_font == NULL)
            setStr(&default_env_font, DEFAULT_ENV_FONT);
        if (readInt() == 0) cdaudio_on_flag = false;
        readStr( &default_cdrom_drive );
        //read and validate sound volume settings
        voice_volume = DEFAULT_VOLUME - readInt();
        if ( (voice_volume < 0) || (voice_volume > DEFAULT_VOLUME) )
            voice_volume = DEFAULT_VOLUME;
        se_volume = DEFAULT_VOLUME - readInt();
        if ( (se_volume < 0) || (se_volume > DEFAULT_VOLUME) )
            se_volume = DEFAULT_VOLUME;
        music_volume = DEFAULT_VOLUME - readInt();
        if ( (music_volume < 0) || (music_volume > DEFAULT_VOLUME) )
            music_volume = DEFAULT_VOLUME;
        if (readInt() == 0) kidokumode_flag = false;
        if (readInt() == 1) {
            bgmdownmode_flag = true;
            
            //SDL_LockMutex(mMusicMutex);
            //music_struct.voice_sample = &wave_sample[0];
            //SDL_UnlockMutex(mMusicMutex);
        }
        readStr( &savedir );
        if (savedir)
            script_h.setSavedir(savedir);
        else
            setStr( &savedir, "" ); //prevents changing savedir
        automode_time = readInt();
    }
    else{
        setStr( &default_env_font, DEFAULT_ENV_FONT );
        voice_volume = se_volume = music_volume = DEFAULT_VOLUME;
    }
    // set the volumes of channels
    channelvolumes[0] = voice_volume;
    for ( int i=1 ; i<ONS_MIX_CHANNELS ; i++ )
        channelvolumes[i] = se_volume;

    //use preferred automode_time, if set
    if (preferred_automode_time_set)
        automode_time = preferred_automode_time;
}

void ONScripter::saveEnvData()
{
    file_io_buf_ptr = 0;
    bool output_flag = false;
    for (int i=0 ; i<2 ; i++){
        writeInt( fullscreen_mode?1:0, output_flag );
        writeInt( volume_on_flag?1:0, output_flag );
        writeInt( text_speed_no, output_flag );
        writeInt( (skip_mode & SKIP_TO_EOP)?1:0, output_flag );
        writeStr( default_env_font, output_flag );
        writeInt( cdaudio_on_flag?1:0, output_flag );
        writeStr( default_cdrom_drive, output_flag );
        writeInt( DEFAULT_VOLUME - voice_volume, output_flag );
        writeInt( DEFAULT_VOLUME - se_volume, output_flag );
        writeInt( DEFAULT_VOLUME - music_volume, output_flag );
        writeInt( kidokumode_flag?1:0, output_flag );
        writeInt( bgmdownmode_flag?1:0, output_flag );
        writeStr( savedir, output_flag );
        writeInt( automode_time, output_flag );

        if (i==1) break;
        allocFileIOBuf();
        output_flag = true;
    }

    saveFileIOBuf( "envdata" );
}

int ONScripter::refreshMode()
{
    if (display_mode & DISPLAY_MODE_TEXT)
        return refresh_window_text_mode;

    return REFRESH_NORMAL_MODE;
}

void ONScripter::quit(bool no_error)
{
    saveAll(no_error);

    if (async_movie) stopMovie(async_movie);
    async_movie = NULL;
    
//#ifdef ONSCRIPTER_CDAUDIO
//    if ( cdrom_info ){
//        SDL_CDStop( cdrom_info );
//        SDL_CDClose( cdrom_info );
//    }
//#endif
//    if ( seqmusic_info ){
//        Mix_HaltMusic();
//        Mix_FreeMusic( seqmusic_info );
//    }
//    if ( music_info ){
//        Mix_HaltMusic();
//        Mix_FreeMusic( music_info );
//    }
}

void ONScripter::disableGetButtonFlag()
{
    btndown_flag = false;
    transbtn_flag = false;

    getzxc_flag = false;
    gettab_flag = false;
    getpageup_flag = false;
    getpagedown_flag = false;
    getinsert_flag = false;
    getfunction_flag = false;
    getenter_flag = false;
    getcursor_flag = false;
    spclclk_flag = false;
    getmclick_flag = false;
    getskipoff_flag = false;
    getmouseover_flag = false;
    getmouseover_min = getmouseover_max = 0;
    btnarea_flag = false;
    btnarea_pos = 0;
}

int ONScripter::getNumberFromBuffer( const char **buf )
{
    int ret = 0;
    while ( **buf >= '0' && **buf <= '9' )
        ret = ret*10 + *(*buf)++ - '0';

    return ret;
}

