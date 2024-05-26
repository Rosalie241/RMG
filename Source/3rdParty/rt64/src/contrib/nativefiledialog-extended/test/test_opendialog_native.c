#define NFD_NATIVE
#include <nfd.h>

#include <stdio.h>
#include <stdlib.h>

/* this test should compile on all supported platforms */

int main(void) {
    // initialize NFD
    // either call NFD_Init at the start of your program and NFD_Quit at the end of your program,
    // or before/after every time you want to show a file dialog.
    NFD_Init();

    nfdchar_t* outPath;

    // prepare filters for the dialog
#ifdef _WIN32
    nfdfilteritem_t filterItem[2] = {{L"Source code", L"c,cpp,cc"}, {L"Headers", L"h,hpp"}};
#else
    nfdfilteritem_t filterItem[2] = {{"Source code", "c,cpp,cc"}, {"Headers", "h,hpp"}};
#endif

    // show the dialog
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 2, NULL);
    if (result == NFD_OKAY) {
        puts("Success!");
#ifdef _WIN32
#ifdef _MSC_VER
        _putws(outPath);
#else
        fputws(outPath, stdin);
#endif
#else
        puts(outPath);
#endif
        // remember to free the memory (since NFD_OKAY is returned)
        NFD_FreePath(outPath);
    } else if (result == NFD_CANCEL) {
        puts("User pressed cancel.");
    } else {
        printf("Error: %s\n", NFD_GetError());
    }

    // Quit NFD
    NFD_Quit();

    return 0;
}
