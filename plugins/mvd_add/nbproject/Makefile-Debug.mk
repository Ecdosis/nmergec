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
	${OBJECTDIR}/benchmark.o \
	${OBJECTDIR}/mvd_add.o \
	${OBJECTDIR}/_ext/688439529/plugin_log.o \
	${OBJECTDIR}/suffixtree.o \
	${OBJECTDIR}/test.o \
	${OBJECTDIR}/path.o \
	${OBJECTDIR}/node.o


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
LDLIBSOPTIONS=-licuuc -lmvd

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_add.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_add.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -dynamiclib -install_name libmvd_add.${CND_DLIB_EXT} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_add.${CND_DLIB_EXT} -fPIC ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/benchmark.o: benchmark.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -I../../mvd/include -Iinclude -I../shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/benchmark.o benchmark.c

${OBJECTDIR}/mvd_add.o: mvd_add.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -I../../mvd/include -Iinclude -I../shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/mvd_add.o mvd_add.c

${OBJECTDIR}/_ext/688439529/plugin_log.o: ../shared/src/plugin_log.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/688439529
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -I../../mvd/include -Iinclude -I../shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/688439529/plugin_log.o ../shared/src/plugin_log.c

${OBJECTDIR}/suffixtree.o: suffixtree.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -I../../mvd/include -Iinclude -I../shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/suffixtree.o suffixtree.c

${OBJECTDIR}/test.o: test.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -I../../mvd/include -Iinclude -I../shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/test.o test.c

${OBJECTDIR}/path.o: path.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -I../../mvd/include -Iinclude -I../shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/path.o path.c

${OBJECTDIR}/node.o: node.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -I../../mvd/include -Iinclude -I../shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/node.o node.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_add.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
