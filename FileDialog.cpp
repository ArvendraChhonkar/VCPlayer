#include "FileDialog.h"

#include "FileDialog.h"
#include "ImGuiFileDialog.h" 


void drawFileDialogUI(bool& showDialog,std::string& selectedFile){
    if(showDialog){
        ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey","Open Video FIle",".mp4, .mkv, .avi, .mov");
        showDialog = false;
    }
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        if(ImGuiFileDialog::Instance()->IsOk()){
            selectedFile = ImGuiFileDialog::Instance()->GetFilePathName();

        }
        ImGuiFileDialog::Instance()->Close();
    }
    
}