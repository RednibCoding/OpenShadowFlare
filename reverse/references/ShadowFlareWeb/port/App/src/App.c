#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "App.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static SDL_Window *g_window = NULL;
static SDL_Renderer *g_renderer = NULL;
static AppRenderCallback g_callback = NULL;
static void *g_callbackUserData = NULL;
static int g_running = 0;

#define APP_TARGET_TICK_MS 16
static Uint32 g_lastTickMs = 0;

int App_Init(const char *title)
{
    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return 0;

    g_window = SDL_CreateWindow(title ? title : "ShadowFlare",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                APP_WIDTH, APP_HEIGHT, SDL_WINDOW_SHOWN);
    if (!g_window)
    {
        SDL_Quit();
        return 0;
    }

#ifdef __EMSCRIPTEN__
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    if (!g_renderer)
        g_renderer = SDL_CreateRenderer(g_window, -1, 0);
#else
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer)
        g_renderer = SDL_CreateRenderer(g_window, -1, 0);
#endif
    if (!g_renderer)
    {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
        SDL_Quit();
        return 0;
    }

    g_running = 1;
    return 1;
}

void App_SetRenderCallback(AppRenderCallback callback, void *userData)
{
    g_callback = callback;
    g_callbackUserData = userData;
}

SDL_Renderer *App_GetRenderer(void)
{
    return g_renderer;
}

static void AppTick(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
            g_running = 0;
    }

    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_renderer);

    if (g_callback)
        g_callback(g_callbackUserData);

    SDL_RenderPresent(g_renderer);

#ifndef __EMSCRIPTEN__
    Uint32 now = SDL_GetTicks();
    Uint32 elapsed = now - g_lastTickMs;
    if (elapsed < APP_TARGET_TICK_MS)
        SDL_Delay(APP_TARGET_TICK_MS - elapsed);
    g_lastTickMs = SDL_GetTicks();
#endif

#ifdef __EMSCRIPTEN__
    if (!g_running)
        emscripten_cancel_main_loop();
#endif
}

void App_Run(void)
{
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(AppTick, 60, 1);
#else
    g_lastTickMs = SDL_GetTicks();
    while (g_running)
        AppTick();
#endif
}

void App_Shutdown(void)
{
    if (g_renderer)
    {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window)
    {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    SDL_Quit();
}
