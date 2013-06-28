#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-MacOSX
CND_DLIB_EXT=dylib
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/b64.o \
	${OBJECTDIR}/src/char_buf.o \
	${OBJECTDIR}/src/chunk_state.o \
	${OBJECTDIR}/src/dyn_string.o \
	${OBJECTDIR}/src/memwatch.o \
	${OBJECTDIR}/src/mvdfile.o \
	${OBJECTDIR}/src/mvdtool.o \
	${OBJECTDIR}/src/operation.o \
	${OBJECTDIR}/src/plugin.o \
	${OBJECTDIR}/src/plugin_list.o \
	${OBJECTDIR}/src/test.o \
	${OBJECTDIR}/src/zip/adler32.o \
	${OBJECTDIR}/src/zip/compress.o \
	${OBJECTDIR}/src/zip/crc32.o \
	${OBJECTDIR}/src/zip/deflate.o \
	${OBJECTDIR}/src/zip/inffast.o \
	${OBJECTDIR}/src/zip/inflate.o \
	${OBJECTDIR}/src/zip/inftrees.o \
	${OBJECTDIR}/src/zip/trees.o \
	${OBJECTDIR}/src/zip/zip.o \
	${OBJECTDIR}/src/zip/zutil.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-ldl -licuuc -lmvd -Lplugins/mvd_add/dist/Debug/GNU-MacOSX -lmvd_add -Lmvd/dist/Debug/GNU-MacOSX -lmvd -Lplugins/mvd_list/dist/Debug/GNU-MacOSX -lmvd_list

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec: plugins/mvd_add/dist/Debug/GNU-MacOSX/libmvd_add.dylib

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec: mvd/dist/Debug/GNU-MacOSX/libmvd.dylib

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec: plugins/mvd_list/dist/Debug/GNU-MacOSX/libmvd_list.dylib

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/src/b64.o: src/b64.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/b64.o src/b64.c

${OBJECTDIR}/src/char_buf.o: src/char_buf.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/char_buf.o src/char_buf.c

${OBJECTDIR}/src/chunk_state.o: src/chunk_state.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/chunk_state.o src/chunk_state.c

${OBJECTDIR}/src/dyn_string.o: src/dyn_string.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/dyn_string.o src/dyn_string.c

${OBJECTDIR}/src/memwatch.o: src/memwatch.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/memwatch.o src/memwatch.c

${OBJECTDIR}/src/mvdfile.o: src/mvdfile.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvdfile.o src/mvdfile.c

${OBJECTDIR}/src/mvdtool.o: src/mvdtool.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvdtool.o src/mvdtool.c

${OBJECTDIR}/src/operation.o: src/operation.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/operation.o src/operation.c

${OBJECTDIR}/src/plugin.o: src/plugin.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/plugin.o src/plugin.c

${OBJECTDIR}/src/plugin_list.o: src/plugin_list.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/plugin_list.o src/plugin_list.c

${OBJECTDIR}/src/test.o: src/test.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/test.o src/test.c

${OBJECTDIR}/src/zip/adler32.o: src/zip/adler32.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/adler32.o src/zip/adler32.c

${OBJECTDIR}/src/zip/compress.o: src/zip/compress.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/compress.o src/zip/compress.c

${OBJECTDIR}/src/zip/crc32.o: src/zip/crc32.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/crc32.o src/zip/crc32.c

${OBJECTDIR}/src/zip/deflate.o: src/zip/deflate.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/deflate.o src/zip/deflate.c

${OBJECTDIR}/src/zip/inffast.o: src/zip/inffast.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/inffast.o src/zip/inffast.c

${OBJECTDIR}/src/zip/inflate.o: src/zip/inflate.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/inflate.o src/zip/inflate.c

${OBJECTDIR}/src/zip/inftrees.o: src/zip/inftrees.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/inftrees.o src/zip/inftrees.c

${OBJECTDIR}/src/zip/trees.o: src/zip/trees.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/trees.o src/zip/trees.c

${OBJECTDIR}/src/zip/zip.o: src/zip/zip.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/zip.o src/zip/zip.c

${OBJECTDIR}/src/zip/zutil.o: src/zip/zutil.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -Iinclude/zip -I/usr/local/include -Imvd/include -Iplugins/shared/include -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/zutil.o src/zip/zutil.c

# Subprojects
.build-subprojects:
	cd plugins/mvd_add && ${MAKE}  -f Makefile CONF=Debug
	cd mvd && ${MAKE}  -f Makefile CONF=Debug
	cd plugins/mvd_list && ${MAKE}  -f Makefile CONF=Debug
	cd mvd && ${MAKE}  -f Makefile CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec

# Subprojects
.clean-subprojects:
	cd plugins/mvd_add && ${MAKE}  -f Makefile CONF=Debug clean
	cd mvd && ${MAKE}  -f Makefile CONF=Debug clean
	cd plugins/mvd_list && ${MAKE}  -f Makefile CONF=Debug clean
	cd mvd && ${MAKE}  -f Makefile CONF=Debug clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
