######################################
#
######################################
#source file
#auto find all *.c and *.cpp files, and compile to *.o file
SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS      := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE))) $(CXXFLAGS) 
  
#target change name to what you want
TARGET  := main
  
#compile and lib parameter
#编译参数
CC              := mipsel-openwrt-linux-gcc
LIBS          := /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/ipkg-install/usr/lib  -L. -lasound -lm -lmp3lame -lshine
LDFLAGS   := -L
DEFINES   := -I 
INCLUDE   := /home/zyc/Documents/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/alsa-lib-1.0.28/include
CFLAGS     := -g -Wall -O3 $(DEFINES) $(INCLUDE)
CXXFLAGS := $(CFLAGS) -DHAVE_CONFIG_H


#i think you should do anything here
#下面的基本上不需要做任何改动了
.PHONY : everything objs clean veryclean rebuild

everything : $(TARGET)

all : $(TARGET)
  
objs : $(OBJS)
  
rebuild: veryclean everything
                
clean :
	rm -fr *.o
    
veryclean : clean
	rm -fr $(TARGET)
  
$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

