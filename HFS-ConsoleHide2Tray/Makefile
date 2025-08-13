CC = g++
CFLAGS = -Wall -O2 -std=c++17
LDFLAGS = -mwindows -static -static-libgcc -static-libstdc++ -lcomctl32 -lgdi32 -lpsapi
RES = resources.o
SRC = HFS-ConsoleHide2Tray.cpp
OBJ = $(SRC:.cpp=.o)
EXE = HFS-ConsoleHide2Tray.exe
ICON = trayicon.ico
RC_FILE = TrayIcon.rc

all: $(EXE)

$(EXE): $(OBJ) $(RES)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

resources.o: $(RC_FILE) resource.h $(ICON)
	windres $(RC_FILE) -o resources.o

clean:
	del /F /Q $(OBJ) $(RES) $(EXE) 2> nul

.PHONY: all clean