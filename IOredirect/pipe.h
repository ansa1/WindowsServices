#pragma once

#include "stdafx.h"

int parent(int argc, TCHAR *argv[]);

int Server(VOID);
int Client(int argc, TCHAR *argv[]);

VOID Service(int arg, TCHAR *argv[]);

bool MyInit();
void MyDone();