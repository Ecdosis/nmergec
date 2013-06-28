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
	${OBJECTDIR}/_ext/1810560952/memwatch.o \
	${OBJECTDIR}/src/benchmark.o \
	${OBJECTDIR}/src/bitset.o \
	${OBJECTDIR}/src/dyn_array.o \
	${OBJECTDIR}/src/encoding.o \
	${OBJECTDIR}/src/group.o \
	${OBJECTDIR}/src/hashmap.o \
	${OBJECTDIR}/src/hint.o \
	${OBJECTDIR}/src/hsieh.o \
	${OBJECTDIR}/src/link_node.o \
	${OBJECTDIR}/src/mvd.o \
	${OBJECTDIR}/src/pair.o \
	${OBJECTDIR}/src/serialiser.o \
	${OBJECTDIR}/src/utils.o \
	${OBJECTDIR}/src/verify.o \
	${OBJECTDIR}/src/version.o \
	${OBJECTDIR}/src/vgnode.o


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
LDLIBSOPTIONS=-licuuc

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -dynamiclib -install_name libmvd.${CND_DLIB_EXT} -fPIC

${OBJECTDIR}/_ext/1810560952/memwatch.o: ../plugins/shared/src/memwatch.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1810560952
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1810560952/memwatch.o ../plugins/shared/src/memwatch.c

${OBJECTDIR}/src/benchmark.o: src/benchmark.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/benchmark.o src/benchmark.c

${OBJECTDIR}/src/bitset.o: src/bitset.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/bitset.o src/bitset.c

${OBJECTDIR}/src/dyn_array.o: src/dyn_array.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/dyn_array.o src/dyn_array.c

${OBJECTDIR}/src/encoding.o: src/encoding.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/encoding.o src/encoding.c

${OBJECTDIR}/src/group.o: src/group.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/group.o src/group.c

${OBJECTDIR}/src/hashmap.o: src/hashmap.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/hashmap.o src/hashmap.c

${OBJECTDIR}/src/hint.o: src/hint.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/hint.o src/hint.c

${OBJECTDIR}/src/hsieh.o: src/hsieh.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/hsieh.o src/hsieh.c

${OBJECTDIR}/src/link_node.o: src/link_node.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/link_node.o src/link_node.c

${OBJECTDIR}/src/mvd.o: src/mvd.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvd.o src/mvd.c

${OBJECTDIR}/src/pair.o: src/pair.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/pair.o src/pair.c

${OBJECTDIR}/src/serialiser.o: src/serialiser.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/serialiser.o src/serialiser.c

${OBJECTDIR}/src/utils.o: src/utils.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/utils.o src/utils.c

${OBJECTDIR}/src/verify.o: src/verify.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/verify.o src/verify.c

${OBJECTDIR}/src/version.o: src/version.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/version.o src/version.c

${OBJECTDIR}/src/vgnode.o: src/vgnode.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -g -DMEMWATCH -Iinclude -I/usr/local/include -I../plugins/shared/include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/vgnode.o src/vgnode.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
