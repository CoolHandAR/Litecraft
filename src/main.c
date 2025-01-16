
#include <Windows.h>

extern int Core_entry();


#ifdef DEBUG
int main()
{
	int error = Core_entry();

	return error;
}
#else
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	int error = Core_entry();

	return error;
}
#endif


