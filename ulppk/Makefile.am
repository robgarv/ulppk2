AM_CFLAGS = -g -fPIC 
AM_LDFLAGS = -ldl -pthread
AM_YFLAGS = -d
lib_LTLIBRARIES=libulppk.la
libulppk_la_SOURCES = appenv.c \
btacc.c \
cmdargs.c \
crc16ccitt.c \
diagnostics.c \
dqacc.c \
inifileparser.y \
inifile.l \
ifile.c \
ioutils.c \
linearlist.c \
llacc.c \
mmapfile.c \
mmatom.c \
mmdeque.c \
mmfor.c \
mmpool.c \
mmrpt_deque.c \
msgcell.c \
msgdeque.c \
pathinfo.c \
process_control.c \
rpt_deque.c \
signalkit.c \
socketio.c \
socketserver.c \
statemachine.c \
sysconfig.c \
ttymodes.c \
trap.c \
ulppk_log.c \
ulppk-properties.c \
urlcoder.c

libulppk_la_LDFLAGS = -release @PACKAGE_VERSION@ -version-info @LIBVERSION@

pkginclude_HEADERS = appenv.h \
bool.h \
btacc.h \
cmdargs.h \
crc16ccitt.h \
diagnostics.h \
dqacc.h \
ifile.h \
ioutils.h \
linearlist.h \
llacc.h \
mmapfile.h \
mmatom.h \
mmdeque.h \
mmfor.h \
mmpool.h \
mmrpt_deque.h \
msgcell.h \
msgdeque.h \
pathinfo.h \
process_control.h \
rpt_deque.h \
signalkit.h \
socketio.h \
socketserver.h \
statemachine.h \
trap.h \
ttymodes.h \
sysconfig.h \
ulppk.h \
ulppk_log.h \
ulppk-properties.h \
urlcoder.h


