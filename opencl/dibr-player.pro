TEMPLATE = app
CONFIG += console
CONFIG -= qt
CONFIG += debug

SOURCES += main.cpp

HEADERS += \
    ocl_common.h \
    img_filters.h \
    options.h

OTHER_FILES += \
    program1.cl \
    convolution.cl

INCLUDEPATH += /usr/local/include /usr/include/nvidia-319-updates
LIBS +=-L/usr/include/nvidia-319-updates/ -lOpenCL -L/usr/local/lib  -lrt -lm -lmagic

INCLUDEPATH += /usr/local/include /usr/include/opencv /usr/include/opencv2 ../processing_library
LIBS += -L/usr/local/lib `pkg-config --libs opencv` -lrt -lm
