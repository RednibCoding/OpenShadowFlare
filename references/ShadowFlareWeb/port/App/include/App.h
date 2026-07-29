#ifndef SFDE_APP_H
#define SFDE_APP_H


#define APP_WIDTH 640
#define APP_HEIGHT 480

typedef struct SDL_Renderer SDL_Renderer;

typedef void (*AppRenderCallback)(void *userData);

int App_Init(const char *title);

void App_SetRenderCallback(AppRenderCallback callback, void *userData);

SDL_Renderer *App_GetRenderer(void);

void App_Run(void);

void App_Shutdown(void);

#endif
