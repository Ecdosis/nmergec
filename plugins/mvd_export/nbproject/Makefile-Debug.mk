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
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/688439529/memwatch.o \
	${OBJECTDIR}/dom.o \
	${OBJECTDIR}/mvd_export.o \
	${OBJECTDIR}/mvd_json.o


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
LDLIBSOPTIONS=-licuuc ../../mvd/dist/Test/GNU-Linux-x86/mvd

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_export.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_export.${CND_DLIB_EXT}: ../../mvd/dist/Test/GNU-Linux-x86/mvd

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_export.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_export.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/_ext/688439529/memwatch.o: ../shared/src/memwatch.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/688439529
	${RM} "$@.d"
	$(COMPILE.c) -g -DMEMWATCH -I../../mvd/include -I../shared/include -I../../include -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/688439529/memwatch.o ../shared/src/memwatch.c

${OBJECTDIR}/dom.o: dom.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -g -DMEMWATCH -I../../mvd/include -I../shared/include -I../../include -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/dom.o dom.c

${OBJECTDIR}/mvd_export.o: mvd_export.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -g -DMEMWATCH -I../../mvd/include -I../shared/include -I../../include -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/mvd_export.o mvd_export.c

${OBJECTDIR}/mvd_json.o: mvd_json.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -g -DMEMWATCH -I../../mvd/include -I../shared/include -I../../include -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/mvd_json.o mvd_json.c

# Subprojects
.build-subprojects:
	cd ../../mvd && ${MAKE}  -f Makefile CONF=Test

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_export.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:
	cd ../../mvd && ${MAKE}  -f Makefile CONF=Test clean

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
