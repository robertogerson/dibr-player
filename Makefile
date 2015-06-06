UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
  SDL_INCLUDES  = -I/usr/include/SDL -I/usr/local/include/SDL
  SDL_LIBS      = -lSDL -lSDL_image
  GL_LIBS       = -lGL -lGLU
  VLC_LIBS      = -lvlc
  
  INCLUDES      = $(SDL_INCLUDES) -Iinclude -I/usr/include -I/usr/local/include:x

else
  SDL_INCLUDES  = -I./deps/SDL/include -I./deps/SDL/include/SDL \
                  -I./deps/SDL_image/include
  SDL_LIBS      = -L./deps/SDL/bin -L./deps/SDL/lib -lmingw32 -lSDL -lSDLmain \
                  -L./deps/SDL_image/bin -lSDL_image
  GL_LIBS       = -lopengl32 -lglu32
  VLC_INCLUDES  = -I./deps/vlc/include
  VLC_LIBS      = -L./deps/vlc -lvlc
  
  INCLUDES      = $(SDL_INCLUDES) $(VLC_INCLUDES) -Iinclude -I/usr/include \
                  -I/usr/local/include 
endif

LIBS = $(SDL_LIBS) $(GL_LIBS) $(VLC_LIBS)

CXXFLAGS =	-O2 -g -Wall -fmessage-length=0 $(INCLUDES) 

OBJS =	        dibr-player.c
#OBJS =	        dibr-player-image-only.c
#OBJS =	        dibr-player-f.cpp

TARGET =        dibr-player

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(INCLUDES) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(TARGET)

