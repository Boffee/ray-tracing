# the exercise is only about path tracing
HEADERS += $$PWD/include/charts/qcustomplot.h
SOURCES += $$PWD/src/area.cpp \
           $$PWD/src/charts/qcustomplot.cpp \
           $$PWD/src/directional.cpp \
           $$PWD/src/naive.cpp \
           $$PWD/src/ex2/hemisampling*.cpp \
           $$PWD/src/ex2/diffuse*.cpp \
           $$PWD/src/ex2/phong*.cpp \
           $$PWD/src/variance.cpp

# SOURCES += $$PWD/src/ex2/*/diffuse.cpp
# SOURCES += $$PWD/src/ex2/*/phong.cpp
