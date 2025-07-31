#pragma once
#include <string>

#include "ImGuiFileDialogConfig.h"
#define IMGUI_PATH_BUTTON_ALIGN_LEFT
void drawFileDialogUI(bool& showDialog, std::string& selectedFile);
