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
	${OBJECTDIR}/src/utils.o \
	${OBJECTDIR}/src/dyn_array.o \
	${OBJECTDIR}/src/zip/zip.o \
	${OBJECTDIR}/src/mvd/mvdfile.o \
	${OBJECTDIR}/src/char_buf.o \
	${OBJECTDIR}/src/mvd/group.o \
	${OBJECTDIR}/src/plugin.o \
	${OBJECTDIR}/src/benchmark.o \
	${OBJECTDIR}/src/zip/deflate.o \
	${OBJECTDIR}/src/mvd/pair.o \
	${OBJECTDIR}/src/dyn_string.o \
	${OBJECTDIR}/src/zip/crc32.o \
	${OBJECTDIR}/src/b64.o \
	${OBJECTDIR}/src/mvd/mvd.o \
	${OBJECTDIR}/src/hashmap.o \
	${OBJECTDIR}/src/zip/compress.o \
	${OBJECTDIR}/src/mvdtool.o \
	${OBJECTDIR}/src/mvd/serialiser.o \
	${OBJECTDIR}/src/operation.o \
	${OBJECTDIR}/src/memwatch.o \
	${OBJECTDIR}/src/zip/inflate.o \
	${OBJECTDIR}/src/plugin_list.o \
	${OBJECTDIR}/src/test.o \
	${OBJECTDIR}/src/link_node.o \
	${OBJECTDIR}/src/zip/zutil.o \
	${OBJECTDIR}/src/hsieh.o \
	${OBJECTDIR}/src/bitset.o \
	${OBJECTDIR}/src/zip/inffast.o \
	${OBJECTDIR}/src/zip/inftrees.o \
	${OBJECTDIR}/src/mvd/chunk_state.o \
	${OBJECTDIR}/src/zip/adler32.o \
	${OBJECTDIR}/src/zip/trees.o


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
LDLIBSOPTIONS=-ldl

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/src/utils.o: src/utils.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/utils.o src/utils.c

${OBJECTDIR}/src/dyn_array.o: src/dyn_array.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/dyn_array.o src/dyn_array.c

${OBJECTDIR}/src/zip/zip.o: src/zip/zip.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/zip.o src/zip/zip.c

${OBJECTDIR}/src/mvd/mvdfile.o: src/mvd/mvdfile.c 
	${MKDIR} -p ${OBJECTDIR}/src/mvd
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvd/mvdfile.o src/mvd/mvdfile.c

${OBJECTDIR}/src/char_buf.o: src/char_buf.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/char_buf.o src/char_buf.c

${OBJECTDIR}/src/mvd/group.o: src/mvd/group.c 
	${MKDIR} -p ${OBJECTDIR}/src/mvd
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvd/group.o src/mvd/group.c

${OBJECTDIR}/src/plugin.o: src/plugin.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/plugin.o src/plugin.c

${OBJECTDIR}/src/benchmark.o: src/benchmark.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/benchmark.o src/benchmark.c

${OBJECTDIR}/src/zip/deflate.o: src/zip/deflate.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/deflate.o src/zip/deflate.c

${OBJECTDIR}/src/mvd/pair.o: src/mvd/pair.c 
	${MKDIR} -p ${OBJECTDIR}/src/mvd
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvd/pair.o src/mvd/pair.c

${OBJECTDIR}/src/dyn_string.o: src/dyn_string.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/dyn_string.o src/dyn_string.c

${OBJECTDIR}/src/zip/crc32.o: src/zip/crc32.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/crc32.o src/zip/crc32.c

${OBJECTDIR}/src/b64.o: src/b64.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/b64.o src/b64.c

${OBJECTDIR}/src/mvd/mvd.o: src/mvd/mvd.c 
	${MKDIR} -p ${OBJECTDIR}/src/mvd
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvd/mvd.o src/mvd/mvd.c

${OBJECTDIR}/src/hashmap.o: src/hashmap.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/hashmap.o src/hashmap.c

${OBJECTDIR}/src/zip/compress.o: src/zip/compress.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/compress.o src/zip/compress.c

${OBJECTDIR}/src/mvdtool.o: src/mvdtool.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvdtool.o src/mvdtool.c

${OBJECTDIR}/src/mvd/serialiser.o: src/mvd/serialiser.c 
	${MKDIR} -p ${OBJECTDIR}/src/mvd
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvd/serialiser.o src/mvd/serialiser.c

${OBJECTDIR}/src/operation.o: src/operation.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/operation.o src/operation.c

${OBJECTDIR}/src/memwatch.o: src/memwatch.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/memwatch.o src/memwatch.c

${OBJECTDIR}/src/zip/inflate.o: src/zip/inflate.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/inflate.o src/zip/inflate.c

${OBJECTDIR}/src/plugin_list.o: src/plugin_list.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/plugin_list.o src/plugin_list.c

${OBJECTDIR}/src/test.o: src/test.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/test.o src/test.c

${OBJECTDIR}/src/link_node.o: src/link_node.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/link_node.o src/link_node.c

${OBJECTDIR}/src/zip/zutil.o: src/zip/zutil.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/zutil.o src/zip/zutil.c

${OBJECTDIR}/src/hsieh.o: src/hsieh.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/hsieh.o src/hsieh.c

${OBJECTDIR}/src/bitset.o: src/bitset.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/bitset.o src/bitset.c

${OBJECTDIR}/src/zip/inffast.o: src/zip/inffast.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/inffast.o src/zip/inffast.c

${OBJECTDIR}/src/zip/inftrees.o: src/zip/inftrees.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/inftrees.o src/zip/inftrees.c

${OBJECTDIR}/src/mvd/chunk_state.o: src/mvd/chunk_state.c 
	${MKDIR} -p ${OBJECTDIR}/src/mvd
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvd/chunk_state.o src/mvd/chunk_state.c

${OBJECTDIR}/src/zip/adler32.o: src/zip/adler32.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/adler32.o src/zip/adler32.c

${OBJECTDIR}/src/zip/trees.o: src/zip/trees.c 
	${MKDIR} -p ${OBJECTDIR}/src/zip
	${RM} $@.d
	$(COMPILE.c) -g -DMVD_TEST -Iinclude -Iinclude/zip -Isrc/mvd -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/zip/trees.o src/zip/trees.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
