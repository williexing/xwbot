TEMPLATE = lib
# CONFIG += console
CONFIG -= qt

SOURCES += main.c \
    syntFilter.c \
    StateSearchW.c \
    StateConstructW.c \
    packing.c \
    lsf.c \
    LPCencode.c \
    LPCdecode.c \
    iLBC_test.c \
    iLBC_encode.c \
    iLBC_decode.c \
    iCBSearch.c \
    iCBConstruct.c \
    hpOutput.c \
    hpInput.c \
    helpfun.c \
    getCBvec.c \
    gainquant.c \
    FrameClassify.c \
    filter.c \
    enhancer.c \
    doCPLC.c \
    createCB.c \
    constants.c \
    anaFilter.c

OTHER_FILES += \
    rfc3951.txt \
    extract-cfile.awk

HEADERS += \
    StateSearchW.h \
    StateConstructW.h \
    packing.h \
    lsf.h \
    LPCencode.h \
    LPCdecode.h \
    iLBC_encode.h \
    iLBC_define.h \
    iLBC_decode.h \
    iCBSearch.h \
    iCBConstruct.h \
    hpOutput.h \
    hpInput.h \
    helpfun.h \
    getCBvec.h \
    gainquant.h \
    FrameClassify.h \
    filter.h \
    enhancer.h \
    doCPLC.h \
    createCB.h \
    constants.h \
    anaFilter.h \
    syntFilter.h

