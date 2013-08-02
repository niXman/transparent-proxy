QT -= core gui
TARGET = transparent-proxy
CONFIG += console
CONFIG -= app_bundle qt
TEMPLATE = app

QMAKE_CXXFLAGS += \
	-std=c++11

SOURCES += \
    main.cpp

LIBS += \
	-lboost_system \
	-lboost_thread \
	-lboost_program_options \
	-lpthread

HEADERS += \
    handler_invoker.hpp \
    handler_allocator.hpp \
    session.hpp \
    acceptor.hpp
