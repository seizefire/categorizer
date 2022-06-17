#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fileapi.h>
#include <windows.h>
#include <winerror.h>
#include <winreg.h>
#include <sys/stat.h>

const int ini_attributes = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN;

char* ini_path;
char* tags;
struct stat st;

HWND window;
HWND label;
HWND input;
HWND button;

int check_ini_path(){
	if(stat(ini_path, &st) == 0){
		if(st.st_mode & S_IFREG == 0){
			MessageBox(NULL, "An error occurred when trying to open desktop.ini\n  =>  desktop.ini does not point to a file", "Filesystem Error", MB_ICONERROR | MB_OK);
			return 1;
		}
	}else{
		FILE* file = fopen(ini_path, "w+");
		if(file == NULL){
			char message[256] = "An error occured when trying to open desktop.ini\n  =>  ";
			strcat(message, strerror(errno));
			MessageBox(NULL, message, "Filesystem Error", MB_OK | MB_ICONERROR);
			return 1;
		}
		fclose(file);
		SetFileAttributesA(ini_path, GetFileAttributesA(ini_path) | ini_attributes);
	}
	return 0;
}

char* strtrim(char* text){
	int left_bound = 0;
	int right_bound = strlen(text) - 1;
	while(left_bound <= right_bound && (text[left_bound] == 32 || text[left_bound] == 9 || text[left_bound] == 10 || text[left_bound] == 13 || text[left_bound] == 0)){
		left_bound++;
	}
	if(left_bound > right_bound){
		return "";
	}
	while(right_bound > left_bound && (text[right_bound] == 32 || text[right_bound] == 9 || text[right_bound] == 10 || text[right_bound] == 13 || text[right_bound] == 0)){
		right_bound--;
	}
	int str_length = right_bound - left_bound + 2;
	char* new_str = malloc(str_length);
	memset(new_str, 0, str_length);
	memcpy(new_str, text + left_bound, str_length - 1);
	return new_str;
}

int edit_desktop_ini(){
	// check the ini file
	if(check_ini_path() > 0){
		return 0;
	}
	// read the file
	FILE* file = fopen(ini_path, "r");
	long size = st.st_size;
	char* contents = malloc(size + 1);
	memset(contents, 0, size + 1);
	fread(contents, sizeof(char), size, file);
	fclose(file);
	// look for header
	char* header = strstr(contents, "[{F29F85E0-4FF9-1068-AB91-08002B27B3D9}]");
	int buffer_length = strlen(contents) + strlen(tags) + 128;
	char* buffer = malloc(buffer_length);
	memset(buffer, 0, buffer_length);
	// handle if file does already have tag
	char tagged = 0;
	if(header != NULL){
		char* property = strstr(header, "\nProp5=");
		if(property != NULL){
			int pre_length = (property + 7) - header;
			char* post = strstr(property + 1, "\n");
			memcpy(buffer, contents, pre_length);
			strcat(buffer, "31,");
			strcat(buffer, tags);
			if(post != NULL){
				strcat(buffer, post);
			}
		}else{
			memcpy(buffer, contents, header + 40 - contents);
			strcat(buffer, "\n");
			strcat(buffer, "Prop5=31,");
			strcat(buffer, tags);
			strcat(buffer, header + 40);
		}
		tagged = 1;
	}
	// handle if file does not already have tag
	if(tagged == 0){
		memcpy(buffer, "[{F29F85E0-4FF9-1068-AB91-08002B27B3D9}]", 40);
		strcat(buffer, "\n");
		strcat(buffer, "Prop5=31,");
		strcat(buffer, tags);
		strcat(buffer, "\n");
		strcat(buffer, contents);
	}
	// remove hidden and system attributes
	SetFileAttributesA(ini_path, (GetFileAttributesA(ini_path) | ini_attributes) ^ ini_attributes);
	// try opening the file
	file = fopen(ini_path, "w+");
	if(file == NULL){
		char message[256] = "An error occured when trying to write to desktop.ini\n  =>  ";
		strcat(message, strerror(errno));
		MessageBox(NULL, message, "Filesystem Error", MB_OK | MB_ICONERROR);
		return 0;
	}
	// write the data
	fwrite(buffer, sizeof(char), strlen(buffer), file);
	// close the file
	fclose(file);
	// return hidden and system attributes
	SetFileAttributesA(ini_path, GetFileAttributesA(ini_path) | ini_attributes);
	// we're done here
	free(contents);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	switch(msg){
		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_CTLCOLORSTATIC:
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (long)CreateSolidBrush(0xffffff);
		case WM_COMMAND:
			switch(wParam){
				case BN_CLICKED:
					char* buffer = malloc(256);
					memset(buffer, 0, 256);
					GetWindowTextA(input, buffer, 255);
					tags = strtrim(buffer);
					free(buffer);
					if(strlen(tags) == 0){
						MessageBox(NULL, "Please enter the tags you want to apply onto the folder", "Error", MB_ICONERROR | MB_OK);
						break;
					}
					edit_desktop_ini();
					DestroyWindow(hwnd);
					break;
			}
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
void registry_error(char* generic, char* detail, int error){
	char* buffer = malloc(256);
	memset(buffer, 0, 256);
	memcpy(buffer, generic, strlen(generic));
	strcat(buffer, "\n  =>  ");
	strcat(buffer, detail);
	strcat(buffer, "\n  =>  ");
	if(error == ERROR_ACCESS_DENIED){
		strcat(buffer, "Access denied. Try running this program in Administrator Mode.");
	}else{	
		LPTSTR string = NULL;
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&string, 0, NULL);
		strcat(buffer, string);
		LocalFree(string);
	}
	MessageBox(NULL, buffer, "Registry Error", MB_ICONERROR | MB_OK);
	free(buffer);
}
void register_program(){
	HKEY base_key;
	LSTATUS status = RegCreateKeyExA(HKEY_CLASSES_ROOT, "Directory\\shell\\Seizefire's Categorizer", 0, NULL, 0, KEY_SET_VALUE | KEY_CREATE_SUB_KEY | KEY_WOW64_64KEY, NULL, &base_key, NULL);
	if(status == 0){
		status = RegSetValueExA(base_key, NULL, 0, REG_SZ, "Categorize...", 14);
		if(status == 0){
			HKEY command_key;
			status = RegCreateKeyExA(base_key, "command", 0, NULL, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &command_key, NULL);
			if(status == 0){
				char* assembly_location = malloc(512);
				memset(assembly_location, 0, 512);
				GetModuleFileNameA(NULL, assembly_location, 512);
				char* command = malloc(512);
				memset(command, 0, 512);
				command[0] = '"';
				strcat(command, assembly_location);
				free(assembly_location);
				strcat(command, "\" \"%1\"");
				status = RegSetValueExA(command_key, NULL, 0, REG_SZ, command, strlen(command) + 1);
				free(command);
				if(status == 0){
					MessageBox(NULL, "Program registered successfully!\nA \"Categorize...\" option should show up when right-clicking a folder.\n\nIf the option doesn't show up, try restarting File Explorer via Task Manager", "Success", MB_ICONINFORMATION | MB_OK);
				}else{
					registry_error("An error occured when trying to edit the registry", "Failed to change values in command key in HKEY_CLASSES_ROOT", status);
				}
				RegCloseKey(command_key);
			}else{
				registry_error("An error occured when trying to edit the registry", "Failed to open command key in HKEY_CLASSES_ROOT", status);
			}
		}else{
			registry_error("An error occured when trying to edit the registry", "Failed to change values in base key in HKEY_CLASSES_ROOT", status);
		}
		RegCloseKey(base_key);
	}else{
		registry_error("An error occured when trying to edit the registry", "Failed to open base key in HKEY_CLASSES_ROOT", status);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// check argument count
	if(__argc < 2){
		int response = MessageBox(NULL, "This program should be run from the File Explorer Context Menu\n\nPress \"OK\" to register this program where it is right now (make sure this program is in a good spot)\nPress \"Cancel\" if you'd like to move this program, or just don't want to register\n\nThis menu can be accessed at any time, even after registering", "Initial Setup", MB_ICONINFORMATION | MB_OKCANCEL);
		if(response == IDOK){
			register_program();
		}
		return 0;
	}
	// copy path into variable
	int path_length = strlen(__argv[1]);
	ini_path = malloc(_MAX_PATH);
	memset(ini_path, 0, _MAX_PATH);
	memcpy(ini_path, __argv[1], path_length);
	// check if directory exists
	struct stat st = {0};
	if(stat(ini_path, &st) != 0 || st.st_mode & S_IFDIR == 0){
		MessageBox(NULL, "An error occurred when trying to open desktop.ini\n  =>  The given path does not point to a directory", "Filesystem Error", MB_ICONERROR | MB_OK);
		return 0;
	}
	// append \desktop.ini
	memcpy(ini_path + path_length, "\\desktop.ini", 12);
	// create file if it doesn't exist
	check_ini_path();
	// setting up the window class
	WNDCLASSEX wincl;
	wincl.cbSize = sizeof(WNDCLASSEX);
	wincl.style = CS_HREDRAW | CS_VREDRAW;
	wincl.lpfnWndProc = WndProc;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hInstance = hInstance;
	wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wincl.lpszMenuName = NULL;
	wincl.lpszClassName = "coolWindowClass";
	wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	// registering the window class
	if(!RegisterClassEx(&wincl)){
		MessageBox(NULL, "An error occurred when trying to create the window\n  =>  Unable to register window class", "Window Error", MB_ICONERROR | MB_OK);
		return 0;
	}
	// creating the window
	window = CreateWindowEx(WS_EX_APPWINDOW, "coolWindowClass", "Categorize", WS_OVERLAPPEDWINDOW ^ WS_SIZEBOX ^ WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 240, 120, NULL, NULL, hInstance, NULL);
	if(window == NULL){
		MessageBox(NULL, "An error occurred when trying to create the window\n  =>  Unable to create window", "Window Error", MB_ICONERROR | MB_OK);
		return 0;
	}
	// creating the text field label
	label = CreateWindow("STATIC", "Enter your tags below", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, 10, 200, 16, window, NULL, hInstance, NULL);
	if(label == NULL){
		MessageBox(NULL, "An error occurred when trying to create the window\n  =>  Unable to create label", "Window Error", MB_ICONERROR | MB_OK);
		return 0;
	}
	// create the OK button
	button = CreateWindow("BUTTON", "OK", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 60, 200, 20, window, NULL, hInstance, NULL);
	if(button == NULL){
		MessageBox(NULL, "An error occurred when trying to create the window\n  =>  Unable to create button", "Window Error", MB_ICONERROR | MB_OK);
		return 0;
	}
	// create the text field
	input = CreateWindow("EDIT", "", WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT, 20, 30, 200, 20, window, NULL, hInstance, NULL);
	if(input == NULL){
		MessageBox(NULL, "An error occurred when trying to create the window\n  =>  Unable to create input", "Window Error", MB_ICONERROR | MB_OK);
		return 0;
	}
	// show the window
	ShowWindow(window, nCmdShow);
	UpdateWindow(window);
	// handle messages
	MSG msg;
	while(GetMessage(&msg, NULL, 0, 0) > 0){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	// final return code
	return msg.wParam;
}