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
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/node.o \
	${OBJECTDIR}/_ext/1161271408/serialiser.o \
	${OBJECTDIR}/_ext/1161271408/pair.o \
	${OBJECTDIR}/_ext/1161271408/mvd.o \
	${OBJECTDIR}/_ext/1445274692/hsieh.o \
	${OBJECTDIR}/src/benchmark.o \
	${OBJECTDIR}/_ext/1445274692/dyn_array.o \
	${OBJECTDIR}/_ext/1161271408/version.o \
	${OBJECTDIR}/src/suffixtree.o \
	${OBJECTDIR}/src/path.o \
	${OBJECTDIR}/src/encoding.o \
	${OBJECTDIR}/_ext/1161271408/group.o \
	${OBJECTDIR}/src/test.o \
	${OBJECTDIR}/src/plugin_log.o \
	${OBJECTDIR}/_ext/1445274692/link_node.o \
	${OBJECTDIR}/_ext/1445274692/utils.o \
	${OBJECTDIR}/src/mvd_add.o \
	${OBJECTDIR}/_ext/1445274692/hashmap.o \
	${OBJECTDIR}/_ext/1445274692/bitset.o


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
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_add.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_add.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -dynamiclib -install_name libmvd_add.${CND_DLIB_EXT} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libmvd_add.${CND_DLIB_EXT} -fPIC ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/src/node.o: src/node.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/node.o src/node.c

${OBJECTDIR}/_ext/1161271408/serialiser.o: ../../src/mvd/serialiser.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1161271408
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1161271408/serialiser.o ../../src/mvd/serialiser.c

${OBJECTDIR}/_ext/1161271408/pair.o: ../../src/mvd/pair.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1161271408
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1161271408/pair.o ../../src/mvd/pair.c

${OBJECTDIR}/_ext/1161271408/mvd.o: ../../src/mvd/mvd.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1161271408
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1161271408/mvd.o ../../src/mvd/mvd.c

${OBJECTDIR}/_ext/1445274692/hsieh.o: ../../src/hsieh.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1445274692
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1445274692/hsieh.o ../../src/hsieh.c

${OBJECTDIR}/src/benchmark.o: src/benchmark.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/benchmark.o src/benchmark.c

${OBJECTDIR}/_ext/1445274692/dyn_array.o: ../../src/dyn_array.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1445274692
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1445274692/dyn_array.o ../../src/dyn_array.c

${OBJECTDIR}/_ext/1161271408/version.o: ../../src/mvd/version.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1161271408
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1161271408/version.o ../../src/mvd/version.c

${OBJECTDIR}/src/suffixtree.o: src/suffixtree.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/suffixtree.o src/suffixtree.c

${OBJECTDIR}/src/path.o: src/path.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/path.o src/path.c

${OBJECTDIR}/src/encoding.o: src/encoding.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/encoding.o src/encoding.c

${OBJECTDIR}/_ext/1161271408/group.o: ../../src/mvd/group.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1161271408
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1161271408/group.o ../../src/mvd/group.c

${OBJECTDIR}/src/test.o: src/test.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/test.o src/test.c

${OBJECTDIR}/src/plugin_log.o: src/plugin_log.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/plugin_log.o src/plugin_log.c

${OBJECTDIR}/_ext/1445274692/link_node.o: ../../src/link_node.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1445274692
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1445274692/link_node.o ../../src/link_node.c

${OBJECTDIR}/_ext/1445274692/utils.o: ../../src/utils.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1445274692
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1445274692/utils.o ../../src/utils.c

${OBJECTDIR}/src/mvd_add.o: src/mvd_add.c 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/mvd_add.o src/mvd_add.c

${OBJECTDIR}/_ext/1445274692/hashmap.o: ../../src/hashmap.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1445274692
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1445274692/hashmap.o ../../src/hashmap.c

${OBJECTDIR}/_ext/1445274692/bitset.o: ../../src/bitset.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/1445274692
	${RM} $@.d
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/1445274692/bitset.o ../../src/bitset.c

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
