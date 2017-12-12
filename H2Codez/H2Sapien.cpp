#include "stdafx.h"
#include "H2Sapien.h"
#include "H2ToolsCommon.h"
#include "Patches.h"
#include "resource.h"
#include <Shellapi.h>
#include <iostream>
#include <fstream>

typedef HWND(__fastcall *create_main_window)(HMENU thisptr, int __unused, HWND hWndParent, HMENU hMenu, LPCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle, HMENU oldmenu, LPVOID lpParam);
create_main_window create_main_window_orginal;

//bool __fastcall load_main_window(int thisptr,int unused, int a2, int a3, int a4, int *a5)
typedef bool(__fastcall *load_main_window)(int thisptr, int unused, int a2, int a3, int a4, int *a5);
load_main_window load_main_window_orginal;

typedef int (__thiscall *main_window_input)(void *thisptr, int a2, UINT uMsg, int hMenu, LPARAM lParam, int a6, int a7);
main_window_input main_window_input_orginal;

bool run_script(char *script_text);

char command_script[0x100];
INT_PTR CALLBACK SapienRunCommandProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CLOSE) {
		EndDialog(hwndDlg, 0);
		return true;
	}
	if (uMsg == WM_COMMAND && LOWORD(wParam)) {
		if (GetDlgItemText(hwndDlg, SAPIEN_COMMAND, command_script, sizeof(command_script)))
			run_script(command_script);
		EndDialog(hwndDlg, 0);
		return true;
	}
	return false;
}

void **script_epilog(void *a1, int return_data)
{
	auto script_epilog_impl = reinterpret_cast<void**(__cdecl *)(void *a1, int return_data)>(0x52CC70);
	return script_epilog_impl(a1, return_data);
}

int __fastcall main_window_input_hook(void *thisptr, BYTE _, int a2, UINT uMsg, int hMenu, LPARAM lParam, int a6, int a7)
{
	if (uMsg == WM_COMMAND) {
		switch (hMenu) {
			case SAPIEN_FILE_NEWINSTANCE:
			{
				return H2CommonPatches::newInstance();
			}
			case SAPIEN_OPEN_RUN_COMMAND_DIALOG:
			{
				DialogBoxParam(g_hModule, MAKEINTRESOURCE(SAPIEN_COMMAND_DIALOG), 0, SapienRunCommandProc, 0);
				return 1;
			}
			case SAPIEN_SCRIPT_DOC:
			{
				H2CommonPatches::print_help_to_doc();
				return 1;
			}
		}
	}
	return main_window_input_orginal(thisptr, a2, uMsg, hMenu, lParam, a6, a7);
}

bool __fastcall load_main_window_hook(int thisptr, int unused, int a2, int a3, int a4, int *a5)
{
	int menu_ptr = thisptr + 12;
	HMENU menu = CAST_PTR(HMENU,menu_ptr);
	menu = LoadMenu(g_hModule, MAKEINTRESOURCE(SAPIEN_MENU));
	return load_main_window_orginal(thisptr, 0, a2, a3, a4, a5);
}

HWND __fastcall create_main_window_hook(HMENU thisptr, int __unused, HWND hWndParent, HMENU hMenu, LPCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle, HMENU oldmenu, LPVOID lpParam)
{
	HMENU new_menu = LoadMenu(g_hModule, MAKEINTRESOURCE(SAPIEN_MENU));
	return create_main_window_orginal(thisptr, 0, hWndParent, hMenu, lpWindowName, dwStyle, dwExStyle, new_menu, lpParam);
}

bool run_script(char *script_text)
{
	bool return_data;
	int run_command_console = 0x4EC020;
	int esp_backup;

	__asm {
		mov esp_backup, esp
		push 1
		mov	esi, script_text
		call run_command_console
		mov return_data, al
		mov esp, esp_backup
	}
	return return_data;
}

std::string baggage_name;

errno_t __cdecl fopen_s_baggage_hook(FILE **File, const char *Filename, const char *Mode)
{
	baggage_name = H2CommonPatches::get_temp_name("baggage.txt");
	return fopen_s(File, baggage_name.c_str(), "w");
}

int __cdecl fclose_baggage_hook(FILE *File)
{
	int ret_data = fclose(File);
	ShellExecuteA(NULL, NULL, baggage_name.c_str(), NULL, NULL, SW_SHOW);
	return ret_data;
}

// this is a massive hack but whatever
bool false_hack = false;
std::string get_value_as_string(void *var_ptr, hs_type type)
{
	std::string value_as_string;
	if (!var_ptr)
		var_ptr = &false_hack;

	switch (type) {
	case hs_type::boolean:
		value_as_string = *reinterpret_cast<bool*>(var_ptr) ? "True" : "False";
		return value_as_string;
	case hs_type::funtion_name:
		value_as_string = "FIXME: read function name";
		return value_as_string;
	case hs_type::long_int:
		value_as_string = std::to_string(*reinterpret_cast<long*>(var_ptr));
		return value_as_string;
	case hs_type::nothing:
		value_as_string = "<void>";
		return value_as_string;
	case hs_type::passthrough:
		value_as_string = "FIXME: read passthrough";
		return value_as_string;
	case hs_type::real:
		value_as_string = std::to_string(*reinterpret_cast<float*>(var_ptr));
		return value_as_string;
	case hs_type::script:
		value_as_string = "FIXME: read script";
		return value_as_string;
	case hs_type::short_int:
		value_as_string = std::to_string(*reinterpret_cast<short*>(var_ptr));
		return value_as_string;
	case hs_type::special_form:
		value_as_string = "FIXME: read special_form";
		return value_as_string;
	}
}

void **__cdecl status_func_impl(int a1, void *a2, char a3)
{
	ofstream output;
	std::string temp_file_name = H2CommonPatches::get_temp_name("status.txt");
	hs_global_variable **global_table = reinterpret_cast<hs_global_variable **>(0x9ECE28);

	output.open(temp_file_name, ios::out);
	if (output)
	{
		for (USHORT current_global_id = 0; current_global_id < 706; current_global_id++)
		{
			hs_global_variable *current_var = global_table[current_global_id];
			std::string value_as_string = get_value_as_string(current_var->variable_ptr, current_var->type);
			output << current_var->name << "   :    " << value_as_string << std::endl;
		}
	}
	output.close();
	ShellExecuteA(NULL, NULL, temp_file_name.c_str(), NULL, NULL, SW_SHOW);
	return script_epilog(a2, 0);
}

hs_command status_cmd("status", hs_type::nothing, CAST_PTR(func_check,0x581EB0), status_func_impl);

void H2SapienPatches::Init()
{
#pragma region Patches
	// stop the default menu overwriting our custom one
	NopFill(0x47AD09, 0x15);
	// Stop sapien from getting a mutex on the main directory
	NopFill(0x409D3D, 0xD);

	// replace bultin IO since it seems to crash
	PatchCall(0x00477AE8, fopen_s_baggage_hook);
	PatchCall(0x00477D4F, fclose_baggage_hook);

	PatchCall(0x00477C05, fwprintf); // scenario name
	PatchCall(0x00477C54, fwprintf);
	PatchCall(0x00477D25, fwprintf); // ==block sizes==
	PatchCall(0x00477D45, fwprintf);

	// %ws should really be used for wchar_t strings instead of %s
	WritePointer(0x477C4F, L"%ws\n\n");
	WritePointer(0x477D40, L"%ws\n");
#pragma endregion

#pragma region Hooks

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	create_main_window_orginal =CAST_PTR(create_main_window,0x469030);
	DetourAttach(&(PVOID&)create_main_window_orginal, create_main_window_hook);

	load_main_window_orginal = CAST_PTR(load_main_window,0x47ACE0);
	DetourAttach(&(PVOID&)load_main_window_orginal, load_main_window_hook);

	main_window_input_orginal = CAST_PTR(main_window_input, 0x475B60);
	DetourAttach(&(PVOID&)main_window_input_orginal, main_window_input_hook);

	DetourTransactionCommit();
#pragma endregion

	hs_command **command_table = reinterpret_cast<hs_command **>(0x9E9E90);
	command_table[0x200] = &status_cmd;
}