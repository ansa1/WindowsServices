#include "stdafx.h"

int _tmain(int argc, TCHAR *argv[])
{
  if (argc != 1) {
    printf("Step 1.\n");
    if (wcscmp(argv[1], L"--server") == 0) {
      Server();
    }
    if (wcscmp(argv[1], L"--client") == 0) {
      Client(argc, argv);
    }
    if (wcscmp(argv[1], L"--service") == 0) {
      Service(argc, argv);
    }
    if (wcscmp(argv[1], L"--parent") == 0) {
      printf("Step 2.\n");
      parent(--argc, ++argv);
      _getch();
    }
  }
}