IF(DEBUG)
IF (ANDROID)
SET(DEBUG_CFLAGS "-g -gdwarf-2 -DANDROID -DDEBUG")
ELSEIF (UNIX)
SET(DEBUG_CFLAGS "-g -gdwarf-2 -O0 -DDEBUG -fno-stack-protector ")
ELSEIF(WIN32)
SET(DEBUG_CFLAGS "/Wall /D__LE__ /DDEBUG /Qstd=c99")
ENDIF (ANDROID)
ELSE(DEBUG)
SET(DEBUG_CFLAGS "-pg")
ENDIF(DEBUG)

IF(WIN32)
SET(X_ARCH win32)
ELSEIF(ANDROID)
SET(X_ARCH android)
ELSEIF(IOS)
SET(X_ARCH ios)
ENDIF(WIN32)

SET(GLOBAL_COMMON_LIBS ssl crypto ogg xbus xparser xmppbot xmppbot xwjni _xvspeaker _x_ilbc speex
speexdsp _x_speex auth_plain bind commands features procsys
stream sasl tls iq presence message core xwcrypt discovery evtdomain xstun ibshell 
jingle sessions _x_hid _x_theora _xqvfb _xqvhid)
