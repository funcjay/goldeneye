#include "stdio.h"
#include "stdlib.h"
#include <string>
#include <vector>
#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#include "triangleapi.h"
#include "r_studioint.h"
#include "com_model.h"
#include "UserMessages.h"

// imgui
#include "PlatformHeaders.h"
#include "SDL2/SDL.h"
#include <gl/GL.h>

#include "imgui.h"
#include "backends/imgui_impl_opengl2.h"
#include "backends/imgui_impl_sdl.h"


SDL_Window* mainWindow;

// OpenGL
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	std::string path = gEngfuncs.pfnGetGameDirectory() + (std::string) "/" + (std::string)filename;
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(path.c_str(), &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;
}

// textures
GLuint healthbar, health;
int healthbarW, healthbarH, healthW, healthH;

// garbage data
int garbage;

void LoadTextures()
{
	// health
	LoadTextureFromFile("gfx/hud/healthbar.tga", &healthbar, &healthbarW, &healthbarH);
	LoadTextureFromFile("gfx/hud/health.tga", &health, &healthW, &healthH);
}

bool CimguiHud::Init()
{
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);

	mainWindow = SDL_GetWindowFromID(1);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplOpenGL2_Init();
	ImGui_ImplSDL2_InitForOpenGL(mainWindow, ImGui::GetCurrentContext());

	LoadTextures();

	return true;
}

void CimguiHud::Reset() {}

bool CimguiHud::VidInit() { return true; }

#define MAX_HP	100.0f
void DrawHealth(ImGuiIO& io)
{
	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_HEALTH | HIDEHUD_ALL)) != 0 || gEngfuncs.IsSpectateOnly() != 0)
		return;

	if (!gHUD.HasSuit())
		return;

	// draw the casing
	ImGui::SetCursorPosX(8.0f);
	ImGui::SetCursorPosY(io.DisplaySize.y - ((float)healthbarH + 7.0f));
	ImGui::Image((void*)(intptr_t)healthbar, ImVec2((float)healthbarW, (float)healthbarH));

	float scaledHealth = (float)gHUD.m_Health.m_iHealth / MAX_HP;

	// draw the bar
	ImGui::SetCursorPosX(96.0f);
	ImGui::SetCursorPosY(io.DisplaySize.y - ((float)healthbarH + 7.0f) + 84.0f);
	ImGui::Image((void*)(intptr_t)health, ImVec2((float)healthW * scaledHealth, (float)healthH), ImVec2((float)(MAX_HP - gHUD.m_Health.m_iHealth) / MAX_HP, 0.0f), ImVec2(1.0f, 1.0f));
}

bool CimguiHud::Draw(float flTime)
{
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_ALL) != 0 || gEngfuncs.IsSpectateOnly() != 0)
		return true;

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplSDL2_NewFrame(mainWindow);
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	// draw here
	// set size and pos
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));

	// start
	ImGui::Begin("hud", null, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

	// draw things
	DrawHealth(io);

	// end
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	return true;
}
