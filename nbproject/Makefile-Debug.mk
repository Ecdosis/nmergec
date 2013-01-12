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
	${OBJECTDIR}/src/empty_output_stream.o \
	${OBJECTDIR}/src/kmpsearch.o \
	${OBJECTDIR}/src/command.o \
	${OBJECTDIR}/src/mvdtool.o \
	${OBJECTDIR}/src/memwatch.o \
	${OBJECTDIR}/src/test.o \
	${OBJECTDIR}/src/output_stream.o \
	${OBJECTDIR}/src/mvd/chunk_state.o


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
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/nmergec ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/src/empty_output_stream.o: src/empty_output_stream.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -Iinclude -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/empty_output_stream.o src/empty_output_stream.c

${OBJECTDIR}/src/kmpsearch.o: src/kmpsearch.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -Iinclude -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/kmpsearch.o src/kmpsearch.c

${OBJECTDIR}/src/command.o: src/command.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -Iinclude -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/command.o src/command.c

${OBJECTDIR}/src/mvdtool.o: src/mvdtool.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -Iinclude -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvdtool.o src/mvdtool.c

${OBJECTDIR}/src/memwatch.o: src/memwatch.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -Iinclude -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/memwatch.o src/memwatch.c

${OBJECTDIR}/src/test.o: src/test.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -Iinclude -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/test.o src/test.c

${OBJECTDIR}/src/output_stream.o: src/output_stream.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -Iinclude -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/output_stream.o src/output_stream.c

${OBJECTDIR}/src/mvd/chunk_state.o: src/mvd/chunk_state.c 
	${MKDIR} -p ${OBJECTDIR}/src/mvd
	${RM} $@.d
	$(COMPILE.c) -g -Iinclude -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvd/chunk_state.o src/mvd/chunk_state.c

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
